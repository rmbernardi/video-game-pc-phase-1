#include "pch.h"
#include "CircularZoneCollisionStrategy.h"
#include <list>

using namespace std;

CircularZoneCollisionStrategy::CircularZoneCollisionStrategy()
{
}

bool CircularZoneCollisionStrategy::Detect(CollisionDetectionInfo * info)
{
	return false;
}

void CircularZoneCollisionStrategy::Detect(
	list<GridSpace *> * retVal,
	float2 playerSize,
	float2 spriteSize,
	Player * pPlayer,
	vector<BaseSpriteData> * sprites)
{
}