// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Units/RigUnit.h"
#include "PulseProceduralWalkRigNode.generated.h"


/**
 * @brief Control rig node to procedurally walk and run using gait parameters and Idle Pose.
 */
USTRUCT(meta = (DisplayName = "Pulse Procedural Walk", Category = "Procedural"))
struct PULSELOCOMOTION_API FPulseProceduralWalkRigNode: public FRigUnitMutable
{
	GENERATED_BODY()
	
public:
	FPulseProceduralWalkRigNode();

	// The RIGVM_METHOD macro is required for the Execute function
	RIGVM_METHOD()
	virtual void Execute() override;
	
	virtual void Initialize() override;

	// --- Input Parameters ---
	// You can add your gait parameters here as UPROPERTY(meta = (Input))
	UPROPERTY(meta = (Input))
	float Speed;

	// Example of another parameter you might use later
	UPROPERTY(meta = (Input))
	float CycleProgress;

	// --- Output Data ---
	// Data to be passed to other nodes
	UPROPERTY(meta = (Output))
	float ModifiedCycleProgress;
};
