// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
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
	
	// Get chaos representation of a primitive component
	static Chaos::FRigidBodyHandle_Internal* GetInternalHandle(UPrimitiveComponent* Component, FName BoneName = NAME_None);

};
