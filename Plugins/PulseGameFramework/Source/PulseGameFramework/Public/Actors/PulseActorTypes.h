// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseActorTypes.generated.h"




// Represent the balance state of a pulse pawn.
UENUM(BlueprintType)
enum class EPulseLocomotionBalanceState: uint8
{
	PerfectlyBalanced = 0 UMETA(DisplayName = "Perfect Balance"),
	Balanced = 1 UMETA(DisplayName = "Mostly Balanced"),
	CriticalBalance = 2 UMETA(DisplayName = "Critical Balance"),
	Unbalanced = 3 UMETA(DisplayName = "Completely Unbalanced"),
};


// Represent a bone segment
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FPulseBoneSegment
{
	GENERATED_BODY()

	FPulseBoneSegment(){}

	FPulseBoneSegment(FName Head, FName Tail, uint8 Shape = 0, FVector2D Extent = {10, 10})
		: HeadBone(Head)
		  , TailBone(Tail)
		  , ShapeType(Shape)
		  , ShapeExtents(Extent)
	{
	}

	// The bone representing the head of the segment
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	FName HeadBone = "";

	// The bone representing the head of the segment
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	FName TailBone = "";

	// The FCollision shape type to use. 0-line, 1-Capsule, 2-Box, 3-Sphere
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	uint8 ShapeType = 0;

	// The bone space head bone dependant segment orientation vector, used to determine the segment roll 
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	FVector SegmentNormal = {0, 0, 1};

	// The size offset from the head bone (X) and tail bone (Y).
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	FVector2D ShapeSizeOffset = {};

	// The location offset in the segment normal direction (X) and its orthogonal direction (Y).
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	FVector2D ShapeLocationOffset = {};

	// The extent of the shape. Box use X for the Segment normal direction and Y for its orthogonal direction. other shape type use only X for the radius.
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="Bones")
	FVector2D ShapeExtents = {10, 10};


	bool ValidBoneSegment() const;

	bool ValidBoneSegmentFor(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh) const;

	bool GetBoneShape(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh, FCollisionShape& OutShape, FTransform& OutWorldSpaceTransform, float& outSegmentLenght) const;

	void DebugDrawBoneSegment(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh, FLinearColor Color = FLinearColor(1, 0.5, 0.4, 1), float Thickness = 1,
	                          bool ShowAxis = true) const;
};


// Represent a creature limb
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FPulseLimbDefinition
{
	GENERATED_BODY()

	FPulseLimbDefinition(){}
	
	FPulseLimbDefinition(FName Name, TArray<FName> BoneList): LimbName(Name)
	{
		for (int i = 1; i < BoneList.Num(); i++)
		{
			LimbBoneChain.Add(FPulseBoneSegment(BoneList[i - 1], BoneList[i]));
		}
	}

	// The name of the limb
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="PulseLimb")
	FName LimbName = "";

	// The limb offset from the center of mass, in component space
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="PulseLimb")
	FVector LimbAnchorOffset = {};

	// Define whether this limb must be used for locomotion or not.
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="PulseLimb")
	bool bLocomotionLimb = false;

	// the chain of bones segments representing the limb. 
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="PulseLimb")
	TArray<FPulseBoneSegment> LimbBoneChain = {};

	// The segment indexes to use fo limb collision 
	UPROPERTY(editAnywhere, BlueprintReadWrite, Category="PulseLimb")
	TArray<int32> LimbCollisionBoneSegmentIndexes = {};
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPulsePawnBalanceEvent, APawn*, Pawn, EPulseLocomotionBalanceState, oldState, EPulseLocomotionBalanceState, newState);
