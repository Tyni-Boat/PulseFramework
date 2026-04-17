// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseDebugLibrary.h"

#include "CompGeom/ConvexHull3.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"

// Chaos includes
#include "Chaos/ImplicitObject.h"
#include "Chaos/Plane.h"
#include "Chaos/Box.h"

// Physics interface includes
#include "TriangleTypes.h"
#include "Core/PulseMathLibrary.h"
#include "PhysicsEngine/BodyInstance.h"


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
	TArray<FVector> points = {};
	if (!UPulseMathLibrary::CircleArcPoints(Location, Radius, DegreeAngle, Axis, HeadingVector, points, SegmentFor360 / 2, bUseCentralHeading))
		return;
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

void UPulseDebugLibrary::DrawDebugPath(const UObject* WorldContext, const TArray<FVector>& Path, FLinearColor Color, float Duration, float Size)
{
	if (!WorldContext)
		return;
	//Draw
	for (int i = 1; i < Path.Num(); ++i)
	{
		FVector lastPoint = Path[i - 1];
		UKismetSystemLibrary::DrawDebugLine(WorldContext, lastPoint, Path[i], Color, Duration, Size);
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

void UPulseDebugLibrary::DrawDebugConvexHull(const UObject* WorldContext, const TArray<FVector>& Points, const FTransform& Transform, const FLinearColor& Color, float Duration,
                                             float Size)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (!World || Points.Num() < 3)
	{
		return;
	}

	// Use TConvexHull3 to compute hull for drawing
	using RealType = double;
	UE::Geometry::TConvexHull3<RealType> HullSolver;
	if (!HullSolver.Solve(Points.Num(), [&Points](int32 Idx) { return Points[Idx]; }, [](int32 Idx) { return true; }))
	{
		return;
	}

	// Draw each triangle of the hull
	HullSolver.GetTriangles([&](const UE::Geometry::FIndex3i& Triangle)
	{
		FVector V0 = Transform.TransformPosition(FVector(Points[Triangle.A]));
		FVector V1 = Transform.TransformPosition(FVector(Points[Triangle.B]));
		FVector V2 = Transform.TransformPosition(FVector(Points[Triangle.C]));

		UKismetSystemLibrary::DrawDebugLine(World, V0, V1, Color, Duration, Size);
		UKismetSystemLibrary::DrawDebugLine(World, V1, V2, Color, Duration, Size);
		UKismetSystemLibrary::DrawDebugLine(World, V2, V0, Color, Duration, Size);
	});
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
