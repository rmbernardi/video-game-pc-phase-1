#pragma once

#include "pch.h"
#include "Constants.h"
#include "DirectXBase.h"
#include "SampleOverlay.h"
#include "SimpleSprites.h"
#include "DebugOverlay.h"
#include "CollisionDetectionStrategy.h"
#include "ScreenBuilder.h"
#include "Player.h"
#include "KeyboardControllerInput.h"
#include "Grid.h"
#include "NarrowCollisionStrategy.h"
#include "BroadCollisionStrategy.h"
#include <fstream>
#include <DirectXMath.h>

using namespace Microsoft::WRL;
using namespace DirectX;


ref class Engine : public DirectXBase , public Windows::ApplicationModel::Core::IFrameworkView
{
internal:
    Engine();

    // DirectXBase Methods
    virtual void CreateDeviceIndependentResources() override;
    virtual void CreateDeviceResources() override;
    virtual void Render() override;
	void Update(float timeTotal, float timeDelta);
	virtual void CreateWindowSizeDependentResources() override;

public:
    // IFrameworkView Methods
    virtual void Initialize(_In_ Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
    virtual void SetWindow(_In_ Windows::UI::Core::CoreWindow^ window);
    virtual void Load(_In_ Platform::String^ entryPoint);
    virtual void Run();
    virtual void Uninitialize();

private:
    // Event Handlers
    void OnWindowSizeChanged(
        _In_ Windows::UI::Core::CoreWindow^ sender,
        _In_ Windows::UI::Core::WindowSizeChangedEventArgs^ args
        );

    void OnVisibilityChanged(
        _In_ Windows::UI::Core::CoreWindow^ sender,
        _In_ Windows::UI::Core::VisibilityChangedEventArgs^ args
        );

    void OnDpiChanged(_In_ Windows::Graphics::Display::DisplayInformation^ sender, _In_ Platform::Object^ args);

    void OnActivated(
        _In_ Windows::ApplicationModel::Core::CoreApplicationView^ applicationView,
        _In_ Windows::ApplicationModel::Activation::IActivatedEventArgs^ args
        );

    void OnSuspending(
        _In_ Platform::Object^ sender,
        _In_ Windows::ApplicationModel::SuspendingEventArgs^ args
        );

    void OnResuming(
        _In_ Platform::Object^ sender,
        _In_ Platform::Object^ args
        );

    void OnWindowClosed(
        _In_ Windows::UI::Core::CoreWindow^ sender,
        _In_ Windows::UI::Core::CoreWindowEventArgs^ args
        );

	void DrawPlayer();
	void DrawSprites();

	void DrawLeftMargin();
	void DrawRightMargin();

	void RenderControllerInput();

#ifdef SHOW_OVERLAY
    SampleOverlay^                                                  m_sampleOverlay;
	DebugOverlay ^	m_debugOverlay;
#endif // SHOW_OVERLAY

    bool                                                            m_windowClosed;
    bool                                                            m_windowVisible;
    ComPtr<ID2D1SolidColorBrush>                    m_blackBrush;
    ComPtr<IDWriteTextFormat>                       m_textFormat;
    ComPtr<ID2D1PathGeometry>                       m_pathGeometry;
    ComPtr<ID2D1PathGeometry>                       m_objectGeometry;
    ComPtr<ID2D1SolidColorBrush>                    m_whiteBrush;
	ComPtr<ID2D1SolidColorBrush>					m_orangeBrush;
	ComPtr<ID2D1SolidColorBrush>					m_greenBrush;
	ComPtr<ID2D1SolidColorBrush>					m_yellowBrush;
	ComPtr<ID2D1SolidColorBrush>					m_grayBrush;
//	ComPtr<ID2D1SolidColorBrush>					m_beigeBrush;
	ComPtr<ID2D1SolidColorBrush>					m_blueBrush;
	ComPtr<ID2D1SolidColorBrush>					m_redBrush;
	ComPtr<ID2D1SolidColorBrush>					m_purpleBrush;
    float                                                           m_pathLength;
    float                                                           m_elapsedTime;

	void DrawHeader(const wchar_t* pText, const D2D1_RECT_F& loc);
	void DrawText(const wchar_t* pText, const D2D1_RECT_F& loc);
	void DrawText(uint32 value, const D2D1_RECT_F& loc);
	void DrawText(int16 value, const D2D1_RECT_F& loc);
	void DrawText(uint8 value, const D2D1_RECT_F& loc);
	void DrawButtonText(uint16 buttons, const D2D1_RECT_F& loc);

#ifdef SIMPLE_SPRITES
	SimpleSprites ^ m_renderer;
#endif // SIMPLE_SPRITES

	BasicSprites::SpriteBatch ^ m_spriteBatch;

	ComPtr<ID3D11Texture2D> m_tree;
	ComPtr<ID3D11Texture2D> m_rock;
	ComPtr<ID3D11Texture2D> m_water;
	ComPtr<ID3D11Texture2D> m_stoneWall;
	ComPtr<ID3D11Texture2D> m_grass;
	ComPtr<ID3D11Texture2D> m_orchi;

	std::vector<BaseSpriteData *> * m_pTreeData;
	std::vector<BaseSpriteData> m_rockData;
	std::vector<BaseSpriteData> m_waterData;
	std::vector<BaseSpriteData> m_stoneWallData;
	std::vector<BaseSpriteData> m_grassData;

	OrchiData m_orchiData;

	ComPtr<ID3D11Texture2D> m_heart;
	std::vector<BaseSpriteData> m_heartData;

	// Input related members
	bool                    m_isControllerConnected;  // Do we have a controller connected
	XINPUT_CAPABILITIES     m_xinputCaps;             // Capabilites of the controller
	XINPUT_STATE            m_xinputState;            // The current state of the controller
	uint64                  m_lastEnumTime;           // Last time a new controller connection was checked

	Microsoft::WRL::ComPtr<IDWriteTextFormat>       m_headerTextFormat;
	Microsoft::WRL::ComPtr<IDWriteTextFormat>       m_dataTextFormat;
	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>    m_textBrush;

	int m_nCollidedSpriteColumn;
	int m_nCollidedSpriteRow;

	bool m_bSpriteCollisionDetected;

	BroadCollisionStrategy * m_broadCollisionDetectionStrategy;
	NarrowCollisionStrategy * m_pNarrowCollisionDetectionStrategy;

	list<BaseSpriteData *> * m_pCollided;

	ScreenBuilder * m_screenBuilder;



	void SetupScreen();
	void BuildScreen();
	void FetchControllerInput();
	int FetchKeyboardInput();

	void CreateLifeText();
	void CreateMapText();
	void CreateButtonsText();
	void CreateInventoryText();
	void CreatePackText();

	void DrawLifeText();
	void DrawMapText();
	void DrawButtonsText();
	void DrawInventoryText();
	void DrawPackText();

	void MovePlayer(uint16 buttons, short horizontal, short vertical);
	void HandleLeftThumbStick(short horizontal, short vertical);

	void HighlightSprite(int column, int row, ComPtr<ID2D1SolidColorBrush> brush);
	void HighlightSprite(int * pLocation, ComPtr<ID2D1SolidColorBrush> brush);


	

	Player * m_pPlayer;

	Microsoft::WRL::ComPtr<IDWriteTextLayout1> m_textLayoutLife;
	Microsoft::WRL::ComPtr<IDWriteTextLayout1> m_textLayoutButtons;
	Microsoft::WRL::ComPtr<IDWriteTextLayout1> m_textLayoutMap;
	Microsoft::WRL::ComPtr<IDWriteTextLayout1> m_textLayoutInventory;
	Microsoft::WRL::ComPtr<IDWriteTextLayout1> m_textLayoutPack;

	DWRITE_TEXT_RANGE m_textRange;

	KeyboardControllerInput * m_pKeyboardController;

	void OnKeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ args);

	Grid grid;

	int intersectRect[4];
	void DrawSpriteIntersection();
	int m_nCollisionState;

	int count;

	Platform::Array<byte> ^ LoadShaderFile(std::string File);

	ComPtr<ID3D11Buffer> vertexbuffer;              // the vertex buffer interface
	ComPtr<ID3D11VertexShader> vertexshader;        // the vertex shader interface
	ComPtr<ID3D11PixelShader> pixelshader;          // the pixel shader interface
	ComPtr<ID3D11InputLayout> inputlayout;          // the input layout interface

	void DrawWrapper();

	XMFLOAT4X4 mView;
	XMFLOAT4X4 mProj;

	ID3D11Buffer* mBoxVB;
	ID3D11Buffer* mBoxIB;

	XMFLOAT4X4 mBoxWorld;

	void DrawScreenQuad();
};

ref class DirectXAppSource : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
