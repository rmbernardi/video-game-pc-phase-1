#pragma once
#include "pch.h"
#include "CollisionDetectionStrategy.h"
#include <list>
#include "GridSpace.h"

class BoundingBoxMidpointCollisionStrategy : public CollisionDetectionStrategy
{
public:
	BoundingBoxMidpointCollisionStrategy();
	bool Detect(CollisionDetectionInfo * info);
	void Detect(
		list<BaseSpriteData> * retVal,
		float2 playerSize,
		float2 spriteSize,
		Player * pPlayer,
		vector<BaseSpriteData> * sprites,
		float fWindowWidth,
		float fWindowHeight);

protected:

private:
};