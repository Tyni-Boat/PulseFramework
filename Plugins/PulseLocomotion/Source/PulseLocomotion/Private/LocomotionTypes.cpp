// Copyright © by Tyni Boat. All Rights Reserved.


#include "LocomotionTypes.h"

#include "Core/PulseDebugLibrary.h"
#include "Core/PulseMathLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "CollisionShape.h"
#include "Kismet/KismetSystemLibrary.h"


bool FPulseBoneSegment::ValidBoneSegment() const
{
	return !HeadBone.IsNone() && !TailBone.IsNone() && HeadBone != TailBone && ShapeType <= 3;
}

bool FPulseBoneSegment::ValidBoneSegmentFor(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh) const
{
	return SkeletalMesh.IsValid() && ValidBoneSegment() && SkeletalMesh->DoesSocketExist(HeadBone) && SkeletalMesh->BoneIsChildOf(TailBone, HeadBone);
}

bool FPulseBoneSegment::GetBoneShape(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh, FCollisionShape& OutShape, FTransform& OutWorldSpaceTransform,
	float& outSegmentLenght) const
{
	if (!ValidBoneSegmentFor(SkeletalMesh))
		return false;
	// Get bone transforms
	const FTransform headTr = SkeletalMesh->GetBoneTransform(HeadBone, RTS_Component);
	const FTransform tailTr = SkeletalMesh->GetBoneTransform(TailBone, RTS_Component);
	const FVector headTailDir = (tailTr.GetLocation() - headTr.GetLocation()).GetSafeNormal();
	const float distance = (tailTr.GetLocation() - headTr.GetLocation()).Length();
	FVector middlePoint = headTr.GetLocation() + headTailDir * distance * 0.5f;
	FVector segmentNormal = SegmentNormal;
	const FRotator compSpaceRot = UKismetMathLibrary::MakeRotFromZY(headTailDir, segmentNormal);
	// Shape
	switch (ShapeType)
	{
	default:
		{
			OutShape = FCollisionShape();
			OutShape.SetShape(ECollisionShape::Line, FVector(0));
		}
		break;
	case 1:
		{
			OutShape = FCollisionShape::MakeCapsule(ShapeExtents.X * 0.5f, (distance - (ShapeSizeOffset.X + ShapeSizeOffset.Y)) * 0.5f);
			middlePoint = middlePoint + headTailDir * (ShapeSizeOffset.X - ShapeSizeOffset.Y)
				+ compSpaceRot.Quaternion().GetRightVector() * ShapeLocationOffset.X
				+ compSpaceRot.Quaternion().GetForwardVector() * ShapeLocationOffset.Y;
		}
		break;
	case 2:
		{
			OutShape = FCollisionShape::MakeBox(FVector(ShapeExtents.Y, ShapeExtents.X, (distance - (ShapeSizeOffset.X + ShapeSizeOffset.Y))) * 0.5f);
			middlePoint = middlePoint + headTailDir * (ShapeSizeOffset.X - ShapeSizeOffset.Y)
				+ compSpaceRot.Quaternion().GetRightVector() * ShapeLocationOffset.X
				+ compSpaceRot.Quaternion().GetForwardVector() * ShapeLocationOffset.Y;
		}
		break;
	case 3:
		{
			OutShape = FCollisionShape::MakeSphere(FMath::Min(ShapeExtents.X, (distance - (ShapeSizeOffset.X + ShapeSizeOffset.Y))) * 0.5f);
			middlePoint = middlePoint + headTailDir * (ShapeSizeOffset.X - ShapeSizeOffset.Y)
				+ compSpaceRot.Quaternion().GetRightVector() * ShapeLocationOffset.X
				+ compSpaceRot.Quaternion().GetForwardVector() * ShapeLocationOffset.Y;
		}
		break;
	}
	FVector ws_Loc = {};
	FQuat ws_Rot = {};
	const FTransform comTr = SkeletalMesh->GetComponentTransform();
	ws_Loc = comTr.TransformPosition(middlePoint);
	ws_Rot = comTr.TransformRotation(compSpaceRot.Quaternion());
	OutWorldSpaceTransform = FTransform(ws_Rot, ws_Loc);
	outSegmentLenght = distance;
	return true;
}

void FPulseBoneSegment::DebugDrawBoneSegment(TWeakObjectPtr<USkeletalMeshComponent> SkeletalMesh, FLinearColor Color, float Thickness, bool ShowAxis) const
{
	FCollisionShape Shape = {};
	FTransform ws_Transform = {};
	float outSegmentLenght = 0.0f;
	if (GetBoneShape(SkeletalMesh, Shape, ws_Transform, outSegmentLenght))
	{
		UPulseDebugLibrary::DrawDebugPrimitiveShape(SkeletalMesh.Get(), ShapeType, ShapeType <= 0? FVector(outSegmentLenght) : Shape.GetExtent(), ws_Transform.GetLocation(), FRotator(ws_Transform.GetRotation()), Color, 0,
		                                            Thickness);
		UKismetSystemLibrary::DrawDebugString(SkeletalMesh.Get(), ws_Transform.GetLocation() - ws_Transform.GetRotation().GetUpVector() * outSegmentLenght * 0.5, HeadBone.ToString(), 0, Color);
		UKismetSystemLibrary::DrawDebugString(SkeletalMesh.Get(), ws_Transform.GetLocation() + ws_Transform.GetRotation().GetUpVector() * outSegmentLenght * 0.5, TailBone.ToString(), 0, Color);
		if (ShowAxis)
		{
			UPulseDebugLibrary::DrawDebugBasis(SkeletalMesh.Get(), ws_Transform.GetLocation(), ws_Transform.GetRotation().GetForwardVector()
				, ws_Transform.GetRotation().GetRightVector(), ws_Transform.GetRotation().GetUpVector(), 0, Thickness);
		}
	}
}
