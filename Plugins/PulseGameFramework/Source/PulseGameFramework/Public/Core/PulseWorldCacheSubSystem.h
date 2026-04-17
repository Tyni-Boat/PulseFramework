// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PulseWorldCacheSubSystem.generated.h"

/**
 * A cache tied to the active world. 
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseWorldCacheSubSystem : public UWorldSubsystem
{
	GENERATED_BODY()

private:
	static UPulseWorldCacheSubSystem* GetInstance(const UObject* WorldContext);
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

#pragma region Convex Physic Shapes
protected:

	TArray<Chaos::FImplicitObjectPtr> _convexPhysicShapeCache;
	TMap<int32, TSet<FVector>> _convexPhysicsShapePointToIdx;
public:
	static Chaos::FImplicitObjectPtr GetPhysicConvexShapeFromPointCloud(const UObject* WorldContext, const TSet<FVector>& PointCloud);
	static bool CachePhysicConvexShapeFromPointCloud(const UObject* WorldContext, const TSet<FVector>& PointCloud, Chaos::FImplicitObjectPtr InShape);

#pragma endregion 
};
