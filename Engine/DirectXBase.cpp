#include "pch.h"
#include "DirectXBase.h"
#include "Constants.h"
#include "GeometryGenerator.h"
#include "vertex.h"
#include "Effects.h"

using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Windows::Graphics::Display;
using namespace D2D1;

// Constructor.
DirectXBase::DirectXBase() :
	m_windowSizeChangeInProgress(false),
	m_dpi(-1.0f)
{
	mScreenQuadVB = NULL;
	mScreenQuadIB = NULL;
}

// Initialize the DirectX resources required to run.
void DirectXBase::Initialize(CoreWindow^ window, float dpi)
{
	m_window = window;

	CreateDeviceIndependentResources();
	CreateDeviceResources();
	SetDpi(dpi);
//	Effects::InitAll(m_d3dDevice.Get());

	BuildScreenQuadGeometryBuffers();
}

// Recreate all device resources and set them back to the current state.
void DirectXBase::HandleDeviceLost()
{
	// Reset these member variables to ensure that SetDpi recreates all resources.
	float dpi = m_dpi;
	m_dpi = -1.0f;
	m_windowBounds.Width = 0;
	m_windowBounds.Height = 0;
	m_swapChain = nullptr;

	CreateDeviceResources();
	SetDpi(dpi);
}

// These are the resources required independent of the device.
void DirectXBase::CreateDeviceIndependentResources()
{
	D2D1_FACTORY_OPTIONS options;
	ZeroMemory(&options, sizeof(D2D1_FACTORY_OPTIONS));

#if defined(_DEBUG)
	// If the project is in a debug build, enable Direct2D debugging via SDK Layers.
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif

	DX::ThrowIfFailed(
		D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory2),
			&options,
			&m_d2dFactory
			)
		);

	DX::ThrowIfFailed(
		DWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory2),
			&m_dwriteFactory
			)
		);

	DX::ThrowIfFailed(
		CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_wicFactory)
			)
		);
}

// These are the resources that depend on the device.
void DirectXBase::CreateDeviceResources()
{
	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	ComPtr<IDXGIDevice> dxgiDevice;

#if defined(_DEBUG)
	if (DX::SdkLayersAvailable())
	{
		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the Direct3D 11 API device object and a corresponding context.
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;
	DX::ThrowIfFailed(
		D3D11CreateDevice(
			nullptr,                    // Specify nullptr to use the default adapter.
			D3D_DRIVER_TYPE_HARDWARE,
			0,
			creationFlags,              // Set debug and Direct2D compatibility flags.
			featureLevels,              // List of feature levels this app can support.
			ARRAYSIZE(featureLevels),
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			&device,                    // Returns the Direct3D device created.
			&m_featureLevel,            // Returns feature level of device created.
			&context                    // Returns the device immediate context.
			)
		);

	// Get the Direct3D 11.1 API device and context interfaces.
	DX::ThrowIfFailed(
		device.As(&m_d3dDevice)
		);

	DX::ThrowIfFailed(
		context.As(&m_d3dContext)
		);

	// Get the underlying DXGI device of the Direct3D device.
	DX::ThrowIfFailed(
		m_d3dDevice.As(&dxgiDevice)
		);

	// Create the Direct2D device object and a corresponding context.
	DX::ThrowIfFailed(
		m_d2dFactory->CreateDevice(dxgiDevice.Get(), &m_d2dDevice)
		);

	DX::ThrowIfFailed(
		m_d2dDevice->CreateDeviceContext(
			D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
			&m_d2dContext
			)
		);
}

// Helps track the DPI in the helper class.
// This is called in the dpiChanged event handler in the view class.
void DirectXBase::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		// Save the DPI of this display in our class.
		m_dpi = dpi;

		// Update Direct2D's stored DPI.
		m_d2dContext->SetDpi(m_dpi, m_dpi);

		// Often a DPI change implies a window size change. In some cases Windows will issue
		// both a size changed event and a DPI changed event. In this case, the resulting bounds
		// will not change, and the window resize code will only be executed once.
		UpdateForWindowSizeChange();
	}
}

// This routine is called in the event handler for the view SizeChanged event.
void DirectXBase::UpdateForWindowSizeChange()
{
	// Only handle window size changed if there is no pending DPI change.
	if (m_dpi != DisplayInformation::GetForCurrentView()->LogicalDpi)
	{
		return;
	}

	if (m_window->Bounds.Width != m_windowBounds.Width ||
		m_window->Bounds.Height != m_windowBounds.Height)
	{
		m_d2dContext->SetTarget(nullptr);
		m_d2dTargetBitmap = nullptr;

		//		for (int i = 0; i < NUM_RENDER_TARGETS; i++)
		{
			m_d3dRenderTargetView = nullptr;
			m_d3dOffscreenRenderTargetView = nullptr;
		}

		m_d3dDepthStencilView = nullptr;
		m_windowSizeChangeInProgress = true;
		CreateWindowSizeDependentResources();
	}
}

//******************************* KNOW THIS FUNCTION **********/
// Allocate all memory resources that change on a window SizeChanged event.
void DirectXBase::CreateWindowSizeDependentResources()
{
	// Store the window bounds so the next time we get a SizeChanged event we can
	// avoid rebuilding everything if the size is identical.
	m_windowBounds = m_window->Bounds;

	if (m_swapChain != nullptr)
	{
		// If the swap chain already exists, resize it.
		HRESULT hr = m_swapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			// If the device was removed for any reason, a new device and swapchain will need to be created.
			HandleDeviceLost();

			// Everything is set up now. Do not continue execution of this method.
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// Otherwise, create a new one using the same adapter as the existing Direct3D device.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
		swapChainDesc.Width = 0;                                     // Use automatic sizing.
		swapChainDesc.Height = 0;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;           // This is the most common swap chain format.
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;                          // Don't use multi-sampling.
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

		// Isn't m_d3dRenderTargetView the back buffer?
		//	How do m_d3dRenderTargetView and the back buffer get associated?
		swapChainDesc.BufferCount = 2;                               // Use double-buffering to minimize latency.
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
		swapChainDesc.Flags = 0;

		// Create the device...

		// First, convert our ID3D11Device1 into an IDXGIDevice1.
		ComPtr<IDXGIDevice1> dxgiDevice;
		DX::ThrowIfFailed(
			m_d3dDevice.As(&dxgiDevice)
			);

		// Second, use the IDXGIDevice1 interface to get access to the adapter.
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(
			dxgiDevice->GetAdapter(&dxgiAdapter)
			);

		// Third, use the IDXGIAdapter interface to get access to the factory.
		// DXGI Factory is an object that can create other DXGI objects.
		ComPtr<IDXGIFactory2> dxgiFactory;
		DX::ThrowIfFailed(
			dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
			);

		CoreWindow^ window = m_window.Get();
		DX::ThrowIfFailed(
			dxgiFactory->CreateSwapChainForCoreWindow(
				m_d3dDevice.Get(),
				reinterpret_cast<IUnknown*>(window),
				&swapChainDesc,
				nullptr,
				&m_swapChain
				)
			);

		// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
		// ensures that the application will only render after each VSync, minimizing power consumption.
		DX::ThrowIfFailed(
			dxgiDevice->SetMaximumFrameLatency(1)
			);
	}

	D3D11_TEXTURE2D_DESC textureDesc;
	HRESULT result;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	ComPtr<ID3D11Texture2D> buffer1;












	// RMB: Create a Direct3D render target view of the swap chain back buffer.

	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(
			0,	// Number of the back buffer to obtain.
			IID_PPV_ARGS(&buffer1))
		);



	// We now have the address of the back buffer.

	// A view is a representation of a model.
	DX::ThrowIfFailed(
		m_d3dDevice->CreateRenderTargetView(
			buffer1.Get(),	// Points to a texture.
			nullptr,
			&m_d3dRenderTargetView	// This associates m_d3dRenderTargetView[0] with the back buffer.
			)
		);

	// RMB: m_d3dRenderTargetView[DEFAULT_BACK_BUFFER] now represents the back buffer.

	// Cache the rendertarget dimensions in our helper class for convenient use.
	D3D11_TEXTURE2D_DESC backBufferDesc = { 0 };
	buffer1->GetDesc(&backBufferDesc);
	m_renderTargetSize.Width = static_cast<float>(backBufferDesc.Width);
	m_renderTargetSize.Height = static_cast<float>(backBufferDesc.Height);





	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = m_renderTargetSize.Width;
	textureDesc.Height = m_renderTargetSize.Height;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags =
		D3D11_BIND_RENDER_TARGET | // Want to render to an off-screen texture
		D3D11_BIND_SHADER_RESOURCE |
		D3D11_BIND_UNORDERED_ACCESS;

	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	ID3D11Texture2D * offscreenTex = NULL;

	// Create the render target texture.
	result = m_d3dDevice->CreateTexture2D(
		&textureDesc,
		NULL,
		&m_renderTargetTexture);

	// Setup the description of the render target view.
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	m_d3dDevice->CreateShaderResourceView(
		m_renderTargetTexture, 
		nullptr, 
		&mOutputTexture);

	// Create the render target view.
	m_d3dDevice->CreateRenderTargetView(
		m_renderTargetTexture,
		&renderTargetViewDesc,
		&m_d3dOffscreenRenderTargetView);


















	// Create a depth stencil view for use with 3D rendering if needed.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
	    DXGI_FORMAT_D24_UNORM_S8_UINT,
	    backBufferDesc.Width,
	    backBufferDesc.Height,
	    1,
	    1,
	    D3D11_BIND_DEPTH_STENCIL
	    );

	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(
	    m_d3dDevice->CreateTexture2D(
	        &depthStencilDesc,
	        nullptr,
	        &depthStencil
	        )
	    );

	auto viewDesc = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
	    m_d3dDevice->CreateDepthStencilView(
	        depthStencil.Get(),
	        &viewDesc,
	        &m_d3dDepthStencilView
	        )
	    );

	// Set the 3D rendering viewport to target the entire window.
	CD3D11_VIEWPORT viewport(
		0.0f, // 500.0f,
		0.0f,
		static_cast<float>(backBufferDesc.Width /* * 0.5f */),
		static_cast<float>(backBufferDesc.Height /* * 0.5f */)
		);

	//D3D11_VIEWPORT vp;
	//vp.TopLeftX = 0.0f;
	//vp.TopLeftY = 500.0f;
	//vp.Width = static_cast<float>(200.f);
	//vp.Height = static_cast<float>(200.f);
	//vp.MinDepth = 0.0f;
	//vp.MaxDepth = 1.0f;

	m_d3dContext->RSSetViewports(1, &viewport);

	// *******************************************************
	// "Take a 2D picture of the 3D image in the back buffer?"
	// *******************************************************

	// Create a Direct2D target bitmap associated with the
	// swap chain back buffer and set it as the current target.
	D2D1_BITMAP_PROPERTIES1 bitmapProperties =
		BitmapProperties1(
			D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			m_dpi,
			m_dpi
			);

	ComPtr<IDXGISurface> dxgiBackBuffer;	// vs ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer))
		);

	DX::ThrowIfFailed(
		m_d2dContext->CreateBitmapFromDxgiSurface(
			dxgiBackBuffer.Get(),
			&bitmapProperties,
			&m_d2dTargetBitmap
			)
		);

	m_d2dContext->SetTarget(m_d2dTargetBitmap.Get());

	// Grayscale text anti-aliasing is recommended for all Windows Store apps.
	m_d2dContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

}

// Method to deliver the final image to the display.
void DirectXBase::Present()
{
	// The application may optionally specify "dirty" or "scroll" rects to improve efficiency
	// in certain scenarios.  In this sample, however, we do not utilize those features.
	DXGI_PRESENT_PARAMETERS parameters = { 0 };
	parameters.DirtyRectsCount = 0;
	parameters.pDirtyRects = nullptr;
	parameters.pScrollRect = nullptr;
	parameters.pScrollOffset = nullptr;

	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present(0, 0);// &parameters);	// // 

	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	//m_d3dContext->DiscardView(m_d3dRenderTargetView.Get());

	//// Discard the contents of the depth stencil.
 //   m_d3dContext->DiscardView(m_d3dDepthStencilView.Get());

	//// If the device was removed either by a disconnect or a driver upgrade, we
	//// must recreate all device resources.
	//if (hr == DXGI_ERROR_DEVICE_REMOVED)
	//{
	//	HandleDeviceLost();
	//}
	//else
	//{
	//	DX::ThrowIfFailed(hr);
	//}

	//if (m_windowSizeChangeInProgress)
	//{
	//	// A window size change has been initiated and the app has just completed presenting
	//	// the first frame with the new size. Notify the resize manager so we can short
	//	// circuit any resize animation and prevent unnecessary delays.
	//	CoreWindowResizeManager::GetForCurrentView()->NotifyLayoutCompleted();
	//	m_windowSizeChangeInProgress = false;
	//}
}

void DirectXBase::ValidateDevice()
{
	// The D3D Device is no longer valid if the default adapter changes or if
	// the device has been removed.

	// First, get the information for the adapter related to the current device.

	ComPtr<IDXGIDevice1> dxgiDevice;
	ComPtr<IDXGIAdapter> deviceAdapter;
	DXGI_ADAPTER_DESC deviceDesc;
	DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));
	DX::ThrowIfFailed(deviceAdapter->GetDesc(&deviceDesc));

	// Next, get the information for the default adapter.

	ComPtr<IDXGIFactory2> dxgiFactory;
	ComPtr<IDXGIAdapter1> currentAdapter;
	DXGI_ADAPTER_DESC currentDesc;
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)));
	DX::ThrowIfFailed(dxgiFactory->EnumAdapters1(0, &currentAdapter));
	DX::ThrowIfFailed(currentAdapter->GetDesc(&currentDesc));

	// If the adapter LUIDs don't match, or if the device reports that it has been removed,
	// a new D3D device must be created.

	if ((deviceDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart) ||
		(deviceDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart) ||
		FAILED(m_d3dDevice->GetDeviceRemovedReason()))
	{
		// Release references to resources related to the old device.
		dxgiDevice = nullptr;
		deviceAdapter = nullptr;

		// Create a new device and swap chain.
		HandleDeviceLost();
	}
}

// Call this method when the app suspends to hint to the driver that the app is entering an idle state
// and that its memory can be used temporarily for other apps.
void DirectXBase::Trim()
{
	ComPtr<IDXGIDevice3> dxgiDevice;
	m_d3dDevice.As(&dxgiDevice);

	dxgiDevice->Trim();
}

void DirectXBase::BuildScreenQuadGeometryBuffers()
{

	GeometryGenerator::MeshData quad;

	GeometryGenerator geoGen;
	geoGen.CreateFullscreenQuad(quad);
	//geoGen.CreateBox(100.f, 100.f, 1, 100, quad);

	std::vector<Vertex::Basic32> vertices(quad.Vertices.size());

	for (UINT i = 0; i < quad.Vertices.size(); ++i)
	{
		vertices[i].Pos = quad.Vertices[i].Position;
		vertices[i].Normal = quad.Vertices[i].Normal;
		vertices[i].Tex = quad.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * quad.Vertices.size();
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &vertices[0];

	if (FAILED(m_d3dDevice->CreateBuffer(&vbd, &vinitData, &mScreenQuadVB)))
	{
		OutputDebugStringA("Failed to create buffer");
	}

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * quad.Indices.size();
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &quad.Indices[0];

	if (FAILED(m_d3dDevice->CreateBuffer(&ibd, &iinitData, &mScreenQuadIB)))
	{
		OutputDebugStringA("Failed to create buffer");
	}
}