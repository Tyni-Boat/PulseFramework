// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseDebugLibrary.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


void UPulseDebugLibrary::DrawDebugTransform(const UObject* WorldContext, const FTransform Transform, FLinearColor Color, float Duration, float Size)
{
	if (!WorldContext)
		return;
	const FVector start = Transform.GetLocation();
	const FVector end = start + Transform.GetRotation().GetForwardVector() * Size * 10;
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end, Size * 10, Color, Duration, Size);
	UKismetSystemLibrary::DrawDebugPoint(WorldContext, start, Size * 5, Color, Duration);
}

void UPulseDebugLibrary::DrawDebugCircle(const UObject* WorldContext, const FVector Location, float Radius, FVector Normal, FLinearColor Color, float Duration, float Size)
{
	if (!WorldContext)
		return;
	const auto rot = UKismetMathLibrary::MakeRotFromZ(Normal).Quaternion();
	UKismetSystemLibrary::DrawDebugCircle(WorldContext, Location, Radius, Radius * 12, Color, Duration, Size, rot.GetRightVector(), rot.GetForwardVector());
}

void UPulseDebugLibrary::DrawDebugArcCircle(const UObject* WorldContext, const FVector Location, float Radius, float DegreeAngle, FVector Axis, FVector HeadingVector,
	FLinearColor Color, float Duration, float Size, float StartArrowSize, float EndArrowSize, int32 SegmentFor360, bool bUseCentralHeading)
{
	if (!WorldContext)
		return;
	if (DegreeAngle == 0)
		return;
	if (HeadingVector.Length() <= 0)
		return;
	if (Axis.Length() <= 0)
		return;
	// Gathering parameters
	const int32 segmentFor360 = FMath::Abs(SegmentFor360);
	float degAngle = DegreeAngle > 0 ? FMath::Min(DegreeAngle, 360) : FMath::Min(DegreeAngle, -360);
	const float radAngle = FMath::DegreesToRadians(degAngle);
	const float perSegmentAngle = 360.0f / static_cast<float>(segmentFor360);
	const float perSegmentRad = FMath::DegreesToRadians(perSegmentAngle);
	const int32 segmentCount = FMath::CeilToInt(static_cast<float>(segmentFor360) * (FMath::Abs(degAngle) / 360.0f)) + 1;
	FVector newAxis = Axis.GetSafeNormal();
	FQuat newRotation = UKismetMathLibrary::MakeRotFromXY(HeadingVector, newAxis).Quaternion();
	if (bUseCentralHeading)
	{
		FQuat offset = FQuat(-newRotation.GetAxisX(), radAngle * 0.5f);
		newRotation = newRotation * offset;
	}
	const FVector u = newRotation.GetAxisX();
	const FVector v = newRotation.GetAxisY();
	// Placing points
	TArray<FVector> points = {};
	float cumulatedAngle = 0;
	for (int i = 0; i < segmentCount; i++)
	{
		FVector point = (u * FMath::Cos(perSegmentRad * i) + v * FMath::Sin(perSegmentRad * i)) * Radius;
		points.Add(point);
		cumulatedAngle += perSegmentAngle;
		if (cumulatedAngle >= FMath::Abs(degAngle))
		{
			point = (u * FMath::Cos(radAngle) + v * FMath::Sin(radAngle)) * Radius;
			points[points.Num() - 1] = point;
			break;
		}
	}
	// Transformation
	for (int i = 0; i < points.Num(); ++i)
	{
		points[i] += Location;
	}
	//Draw
	for (int i = 1; i < points.Num(); ++i)
	{
		FVector lastPoint = points[i - 1];
		if (i == 1 && StartArrowSize > 0)
		{
			UKismetSystemLibrary::DrawDebugArrow(WorldContext, points[i], lastPoint, StartArrowSize, Color, Duration, Size);
			continue;
		}
		if (i == (points.Num() - 1) && EndArrowSize > 0)
		{
			UKismetSystemLibrary::DrawDebugArrow(WorldContext, lastPoint, points[i], EndArrowSize, Color, Duration, Size);
			continue;
		}
		UKismetSystemLibrary::DrawDebugLine(WorldContext, lastPoint, points[i], Color, Duration, Size);
	}
}

void UPulseDebugLibrary::DrawDebugBasis(const UObject* WorldContext, const FVector DrawLocation, const FVector ForwardVector, const FVector RightVector, const FVector UpVector,
                                        float Duration, float Size)
{
	const FVector start = DrawLocation;
	const FVector end_f = start + ForwardVector.GetSafeNormal() * Size * 10;
	const FVector end_r = start + RightVector.GetSafeNormal() * Size * 10;
	const FVector end_u = start + UpVector.GetSafeNormal() * Size * 10;
	const FLinearColor f_col = FColor::Red;
	const FLinearColor r_col = FMath::IsNearlyEqual(ForwardVector.GetSafeNormal() | RightVector.GetSafeNormal(), 0) ? FColor::Green : FColor::Silver;
	const FLinearColor u_col = FMath::IsNearlyEqual(ForwardVector.GetSafeNormal() | UpVector.GetSafeNormal(), 0) ? FColor::Blue : FColor::Silver;
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end_f, Size * 10, f_col, Duration, Size);
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end_r, Size * 10, r_col, Duration, Size);
	UKismetSystemLibrary::DrawDebugArrow(WorldContext, start, end_u, Size * 10, u_col, Duration, Size);
}

void UPulseDebugLibrary::DrawDebugPrimitiveShape(const UObject* WorldContext, uint8 ShapeType, const FVector Extents, const FVector Location, const FRotator Rotation,
                                                 FLinearColor Color, float Duration, float Size)
{
	if (!WorldContext)
		return;
	if (ShapeType > 3)
		return;
	switch (ShapeType)
	{
	default: //Line
		{
			const FVector start = Location - Rotation.Quaternion().GetUpVector() * Extents.X * 0.5;
			const FVector end = Location + Rotation.Quaternion().GetUpVector() * Extents.X * 0.5;
			UKismetSystemLibrary::DrawDebugLine(WorldContext, start, end, Color, Duration, Size);
		}
		break;
	case 1: //Capsule
		UKismetSystemLibrary::DrawDebugCapsule(WorldContext, Location, Extents.Z, FMath::Max(Extents.X, Extents.Y), Rotation, Color, Duration, Size);
		break;
	case 2: //Box
		UKismetSystemLibrary::DrawDebugBox(WorldContext, Location, Extents, Color, Rotation, Duration, Size);
		break;
	case 3: //Sphere
		UKismetSystemLibrary::DrawDebugSphere(WorldContext, Location, FMath::Max(Extents.X, Extents.Y), 12, Color, Duration, Size);
		break;
	}
}

FString UPulseDebugLibrary::DebugNetLog(const UObject* Obj, const FString& message)
{
	return FString::Printf(TEXT("[%s-{%d}] - %s"), *NetModeStr(Obj), NetClientId(Obj), *message);
}

FString UPulseDebugLibrary::NetModeStr(const UObject* Obj)
{
	if (!Obj) return TEXT("NULL");

	UWorld* W = Obj->GetWorld();
	if (!W) return TEXT("NO_WORLD");

	switch (W->GetNetMode())
	{
	case NM_Standalone: return TEXT("Standalone");
	case NM_Client: return TEXT("Client");
	case NM_ListenServer: return TEXT("ListenServer");
	case NM_DedicatedServer: return TEXT("Server");
	default: return TEXT("Unknown");
	}
}

int32 UPulseDebugLibrary::NetClientId(const UObject* Obj)
{
#if WITH_EDITOR
	const UWorld* W = Obj ? Obj->GetWorld() : nullptr;
	return (W ? W->GetOutermost()->GetPIEInstanceID() : -1);
#else
	return -1;
#endif
}
