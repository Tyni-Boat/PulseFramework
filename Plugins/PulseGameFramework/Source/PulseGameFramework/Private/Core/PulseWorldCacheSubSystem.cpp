// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseWorldCacheSubSystem.h"

#include "PulseGameFramework.h"


UPulseWorldCacheSubSystem* UPulseWorldCacheSubSystem::GetInstance(const UObject* WorldContext)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
		return nullptr;
	return World->GetSubsystem<UPulseWorldCacheSubSystem>();
}

void UPulseWorldCacheSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPulseWorldCacheSubSystem::Deinitialize()
{
	Super::Deinitialize();
	// Physic Shapes
	for (int i = _convexPhysicShapeCache.Num() - 1; i >= 0; i--)
	{
		if (_convexPhysicShapeCache[i].IsValid())
		{
			_convexPhysicShapeCache[i].SafeRelease();
		}
		_convexPhysicShapeCache.RemoveAt(i);
	}
	_convexPhysicShapeCache.Empty();
	_convexPhysicsShapePointToIdx.Empty();
	//
}


Chaos::FImplicitObjectPtr UPulseWorldCacheSubSystem::GetPhysicConvexShapeFromPointCloud(const UObject* WorldContext, const TSet<FVector>& PointCloud)
{
	auto cache = GetInstance(WorldContext);
	if (!cache)
		return nullptr;
	TArray<int32> keys;
	cache->_convexPhysicsShapePointToIdx.GetKeys(keys);
	TSet<FVector> diffSet = {};
	int32 index = -1;
	for (const int32& key : keys)
	{
		diffSet.Empty();
		diffSet = cache->_convexPhysicsShapePointToIdx[key].Difference(PointCloud);
		if (diffSet.IsEmpty())
		{
			index = key;
			break;
		}
	}
	if (index < 0)
		return nullptr;
	if (!cache->_convexPhysicShapeCache[index].IsValid())
		return nullptr;
	return cache->_convexPhysicShapeCache[index];
}

bool UPulseWorldCacheSubSystem::CachePhysicConvexShapeFromPointCloud(const UObject* WorldContext, const TSet<FVector>& PointCloud, Chaos::FImplicitObjectPtr InShape)
{
	if (!InShape.IsValid())
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("Impossible to cache physic convex hull from cloud points: Invalid In Shape"));
		return false;
	}
	auto cache = GetInstance(WorldContext);
	if (!cache)
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("Impossible to cache physic convex hull from cloud points: Cache subSystem is not available"));
		return false;
	}
	TArray<int32> keys;
	cache->_convexPhysicsShapePointToIdx.GetKeys(keys);
	TSet<FVector> diffSet = {};
	int32 index = -1;
	for (const int32& key : keys)
	{
		diffSet.Empty();
		diffSet = cache->_convexPhysicsShapePointToIdx[key].Difference(PointCloud);
		if (diffSet.IsEmpty())
		{
			index = key;
			break;
		}
	}
	if (index >= 0)
	{
		if (!cache->_convexPhysicShapeCache[index].IsValid())
			cache->_convexPhysicShapeCache[index] = InShape;
		return true;
	}
	UE_LOG(LogPulsePhysics, Log, TEXT("Successfully cached physic convex hull from cloud points"));
	index = cache->_convexPhysicShapeCache.Add(InShape);
	cache->_convexPhysicsShapePointToIdx.Add(index, PointCloud);
	return true;
}
