// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Chaos/ImplicitObject.h"
#include "Chaos/ShapeInstance.h"
#include "Chaos/Framework/PhysicsProxy.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PhysicsInterfaceDeclaresCore.h"
#include "PulsePhysicChaosLibrary.generated.h"


/**
 * Library of chaos Physic Functions
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulsePhysicChaosLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Add force directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddForce(UPrimitiveComponent* Component, FVector Force, bool bAccelChange, FName BoneName);

	// Add force at location directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force,
	                                    FName BoneName);

	// Add torque directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddTorque(UPrimitiveComponent* Component, FVector Torque, bool bAccelChange,
	                           FName BoneName);

	// Add Impulse directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddImpulse(UPrimitiveComponent* Component, FVector Impulse, bool bVelChange,
	                            FName BoneName);

	// Add impulse at position directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddImpulseAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Impulse,
	                                      FName BoneName);

	// Add angular impulse directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddAngularImpulseInRadians(UPrimitiveComponent* Component, FVector Torque,
	                                            bool bVelChange, FName BoneName);

	// Add force directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosAddAngularImpulseInDegrees(UPrimitiveComponent* Component, FVector Torque,
	                                            bool bVelChange, FName BoneName);

	// Get transform directly from a chaos body. Works in async ticks.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Physic")
	static FTransform ChaosGetTransform(UPrimitiveComponent* Component);

	// Get linear velocity directly from a chaos body. Works in async ticks.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Physic")
	static FVector ChaosGetLinearVelocity(UPrimitiveComponent* Component, FName BoneName);

	// Get Center of Mass location directly from a chaos body. Works in async ticks.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Physic")
	static FVector ChaosGetCoMPosition(UPrimitiveComponent* Component);


	// Get angular velocity directly from a chaos body. Works in async ticks.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Physic")
	static FVector ChaosGetAngularVelocity(UPrimitiveComponent* Component, FName BoneName);

	// Set linear velocity directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosSetLinearVelocity(UPrimitiveComponent* Component, FVector Velocity, bool bAddToCurrent,
	                                   FName BoneName);

	// Set angular velocity in Radian directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosSetAngularVelocityInRadians(UPrimitiveComponent* Component, FVector AngVelocity,
	                                             bool bAddToCurrent, FName BoneName);

	// Set angular velocity in Degrees directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosSetAngularVelocityInDegrees(UPrimitiveComponent* Component, FVector AngVelocity,
	                                             bool bAddToCurrent, FName BoneName);

	// Set world location directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosSetWorldLocation(USceneComponent* Component, FVector Location);

	// Set world rotation directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosSetWorldRotation(UPrimitiveComponent* Component, FRotator Rotation);

	// Set world location and rotation directly to a chaos body. Works in async ticks.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic")
	static void ChaosSetWorldLocationAndRotation(UPrimitiveComponent* Component, FVector Location,
	                                             FRotator Rotation);

	// Get Velocity directly from a chaos body. Works in async ticks.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Physic")
	static FVector ChaosGetLinearVelocityAtPoint(UPrimitiveComponent* Component, FVector Point, FName BoneName);

	// Trace against a cone
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic",
		Meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Cone Trace By Channel", AdvancedDisplay =
			"TraceColor, TraceHitColor, DrawTime", Keywords = "sweep"))
	static bool ConeTraceMulti(const UObject* WorldContextObject, FVector Start, FRotator Direction, float ConeHeight, float ConeRadius,
	                           TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	                           TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);

	// Trace against a disc or partial disc
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic",
		Meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore", DisplayName = "Multi Disc Trace By Channel", AdvancedDisplay =
			"TraceColor, TraceHitColor, DrawTime", Keywords = "sweep"))
	static bool DiscTraceMulti(const UObject* WorldContextObject, FVector Start, FRotator Direction, FVector2D Radiuses, float ArcDegAngle, float Thickness,
	                           TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType,
	                           TArray<FHitResult>& OutHits, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime);

	/**
	 * Sweep a convex hull (defined by a list of points) through the world
	 * @param WorldContextObject - Context object to get the world
	 * @param Points - Array of points defining the convex hull
	 * @param Start - Start position of the sweep
	 * @param End - End position of the sweep
	 * @param Rotation - Rotation of the convex hull
	 * @param TraceChannel - Collision channel to use for the query
	 * @param bTraceComplex - Whether to trace against complex collision
	 * @param ActorsToIgnore - Actors to ignore during the sweep
	 * @param OutHits - Output array of hit results
	 * @param DrawDebugType - The Draw debug type
	 * @param TraceColor - Color of an unsuccessful trace query.
	 * @param TraceHitColor - Color of a successful trace query.
	 * @param DrawTime - Debug draw time when using for duration mode.
	 * @param bCacheGeometry - Cache the convex geometry for later use
	 * @return True if a blocking hit was found
	 */
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic",
		Meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Points, Start, End, Rotation, ActorsToIgnore", DisplayName = "Sweep Convex Hull Multi"
			, AdvancedDisplay = "DrawDebugType, TraceColor, TraceHitColor, DrawTime", Keywords = "sweep, convex"))
	static bool SweepConvexHullMulti(const UObject* WorldContextObject, const TArray<FVector>& Points, const FVector& Start, const FVector& End,
	                                 const FRotator& Rotation, TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	                                 TArray<FHitResult>& OutHits, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime
	                                 , bool bCacheGeometry = false);


	/**
	 * Check for overlaps with a convex hull (defined by a list of points) at a location
	 * @param WorldContextObject - Context object to get the world
	 * @param Points - Array of points defining the convex hull
	 * @param Location - Location to test for overlaps
	 * @param Rotation - Rotation of the convex hull
	 * @param TraceChannel - Collision channel to use for the query
	 * @param bTraceComplex - Whether to trace against complex collision
	 * @param ActorsToIgnore - Actors to ignore during the overlap check
	 * @param OutOverlaps - Output array of overlap results
	 * @param DrawDebugType - The Draw debug type
	 * @param TraceColor - Color of a non overlap query.
	 * @param TraceHitColor - Color of an overlap query.
	 * @param DrawTime - Debug draw time when using for duration mode.
	 * @param bCacheGeometry - Cache the convex geometry for later use
	 * @return True if any overlaps were found
	*/
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Physic",
		Meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "Points, Start, End, Rotation, ActorsToIgnore", DisplayName = "Overlap Convex Hull Multi"
		, AdvancedDisplay = "DrawDebugType, TraceColor, TraceHitColor, DrawTime", Keywords = "overlap, convex"))
	static bool OverlapConvexHullMulti(const UObject* WorldContextObject, const TArray<FVector>& Points, const FVector& Location, const FRotator& Rotation,
	                                   TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
	                                   TArray<UPrimitiveComponent*>& OutOverlaps, EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime
	                                 , bool bCacheGeometry = false);


	// Helper to build a Chaos::FConvex from a set of points
	static Chaos::FImplicitObjectPtr BuildConvexFromPoints(const TArray<FVector>& PointCloud, const UWorld* CacheContext = nullptr, bool bDebug = false);

	// Get chaos representation of a primitive component
	static Chaos::FRigidBodyHandle_Internal* GetInternalHandle(UPrimitiveComponent* Component, FName BoneName = NAME_None);
};
