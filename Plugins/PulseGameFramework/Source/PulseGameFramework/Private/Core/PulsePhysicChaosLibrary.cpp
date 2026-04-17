// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulsePhysicChaosLibrary.h"

#include "PulseGameFramework.h"
#include "PhysicsProxy/SingleParticlePhysicsProxy.h"
#include "Core/PulseDebugLibrary.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "CompGeom/ConvexHull3.h"

// Chaos includes
#include "Chaos/Convex.h"           // Chaos::FConvex  (FVec3 / FPlane types)
#include "Chaos/AABB.h"             // Chaos::FAABB3

// ---- GeometryCollection ----
#include "GeometryCollection/GeometryCollection.h"
#include "GeometryCollection/GeometryCollectionObject.h"


// Physics interface includes
#include "Core/PulseMathLibrary.h"
#include "Core/PulseWorldCacheSubSystem.h"
#include "Engine/OverlapResult.h"
#include "Kismet/KismetMathLibrary.h"
#include "PhysicsEngine/BodyInstance.h"


void UPulsePhysicChaosLibrary::ChaosAddForce(UPrimitiveComponent* Component, FVector Force, bool bAccelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAccelChange)
		{
			const float RigidMass = RigidHandle->M();
			const Chaos::FVec3 Acceleration = Force * RigidMass;
			RigidHandle->AddForce(Acceleration, false);
		}
		else
		{
			RigidHandle->AddForce(Force, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddForceAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Force, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		const Chaos::FVec3 WorldCOM = Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle);
		const Chaos::FVec3 WorldTorque = Chaos::FVec3::CrossProduct(Position - WorldCOM, Force);
		RigidHandle->AddForce(Force, false);
		RigidHandle->AddTorque(WorldTorque, false);
	}
}

void UPulsePhysicChaosLibrary::ChaosAddTorque(UPrimitiveComponent* Component, FVector Torque, bool bAccelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAccelChange)
		{
			RigidHandle->AddTorque(Chaos::FParticleUtilitiesXR::GetWorldInertia(RigidHandle) * Torque, false);
		}
		else
		{
			RigidHandle->AddTorque(Torque, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddImpulse(UPrimitiveComponent* Component, FVector Impulse, bool bVelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bVelChange)
		{
			RigidHandle->SetLinearImpulse(RigidHandle->LinearImpulse() + RigidHandle->M() * Impulse, false);
		}
		else
		{
			RigidHandle->SetLinearImpulse(RigidHandle->LinearImpulse() + Impulse, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddImpulseAtPosition(UPrimitiveComponent* Component, FVector Position, FVector Impulse, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		const Chaos::FVec3 WorldCOM = Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle);
		const Chaos::FVec3 AngularImpulse = Chaos::FVec3::CrossProduct(Position - WorldCOM, Impulse);
		RigidHandle->SetLinearImpulse(RigidHandle->LinearImpulse() + Impulse, false);
		RigidHandle->SetAngularImpulse(RigidHandle->AngularImpulse() + AngularImpulse, false);
	}
}

void UPulsePhysicChaosLibrary::ChaosAddAngularImpulseInRadians(UPrimitiveComponent* Component, FVector Torque, bool bVelChange, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bVelChange)
		{
			const Chaos::FMatrix33 WorldI = Chaos::FParticleUtilitiesXR::GetWorldInertia(RigidHandle);
			RigidHandle->SetAngularImpulse(RigidHandle->AngularImpulse() + (WorldI * Torque), false);
		}
		else
		{
			RigidHandle->SetAngularImpulse(RigidHandle->AngularImpulse() + Torque, false);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosAddAngularImpulseInDegrees(UPrimitiveComponent* Component, FVector Torque, bool bVelChange, FName BoneName)
{
	ChaosAddAngularImpulseInRadians(Component, FMath::DegreesToRadians(Torque), bVelChange, BoneName);
}

FTransform UPulsePhysicChaosLibrary::ChaosGetTransform(UPrimitiveComponent* Component)
{
	if (const Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		const Chaos::FRigidTransform3 WorldCOM = Chaos::FParticleUtilitiesGT::GetActorWorldTransform(RigidHandle);
		return WorldCOM;
	}
	return Component ? Component->GetComponentTransform() : FTransform();
}

FVector UPulsePhysicChaosLibrary::ChaosGetLinearVelocity(UPrimitiveComponent* Component, FName BoneName)
{
	if (const Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		return RigidHandle->V();
	}
	return FVector::ZeroVector;
}

FVector UPulsePhysicChaosLibrary::ChaosGetCoMPosition(UPrimitiveComponent* Component)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		if (ensure(RigidHandle->CanTreatAsKinematic()))
		{
			const bool bIsRigid = RigidHandle->CanTreatAsRigid();
			return bIsRigid
				       ? Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle)
				       : static_cast<Chaos::FVec3>(Chaos::FParticleUtilitiesGT::GetActorWorldTransform(RigidHandle).
					       GetTranslation());
		}
	}
	return FVector::ZeroVector;
}

FVector UPulsePhysicChaosLibrary::ChaosGetAngularVelocity(UPrimitiveComponent* Component, FName BoneName)
{
	if (const Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		return RigidHandle->W();
	}
	return FVector::ZeroVector;
}

void UPulsePhysicChaosLibrary::ChaosSetLinearVelocity(UPrimitiveComponent* Component, FVector Velocity, bool bAddToCurrent, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAddToCurrent)
		{
			RigidHandle->SetV(RigidHandle->V() + Velocity);
		}
		else
		{
			RigidHandle->SetV(Velocity);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosSetAngularVelocityInRadians(UPrimitiveComponent* Component, FVector AngVelocity, bool bAddToCurrent, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (bAddToCurrent)
		{
			RigidHandle->SetW(RigidHandle->W() + AngVelocity);
		}
		else
		{
			RigidHandle->SetW(AngVelocity);
		}
	}
}

void UPulsePhysicChaosLibrary::ChaosSetAngularVelocityInDegrees(UPrimitiveComponent* Component, FVector AngVelocity, bool bAddToCurrent, FName BoneName)
{
	ChaosSetAngularVelocityInRadians(Component, FMath::DegreesToRadians(AngVelocity), bAddToCurrent, BoneName);
}

void UPulsePhysicChaosLibrary::ChaosSetWorldLocation(USceneComponent* Component, FVector Location)
{
	if (!Component)
	{
		return;
	}

	if (UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(Component))
	{
		if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(PrimitiveComponent))
		{
			const Chaos::FVec3 P = Location - RigidHandle->R().RotateVector(RigidHandle->CenterOfMass());
			RigidHandle->SetX(P);
		}
	}
	else
	{
		Component->SetWorldLocation(Location);
	}
}

void UPulsePhysicChaosLibrary::ChaosSetWorldRotation(UPrimitiveComponent* Component, FRotator Rotation)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		const Chaos::FRotation3 Q = Rotation.Quaternion() * RigidHandle->RotationOfMass().Inverse();
		RigidHandle->SetR(Q);
	}
}

void UPulsePhysicChaosLibrary::ChaosSetWorldLocationAndRotation(UPrimitiveComponent* Component, FVector Location, FRotator Rotation)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component))
	{
		const Chaos::FRotation3 Q = Rotation.Quaternion() * RigidHandle->RotationOfMass().Inverse();
		const Chaos::FVec3 P = Location - Q.RotateVector(RigidHandle->CenterOfMass());
		RigidHandle->SetR(Q);
		RigidHandle->SetX(P);
	}
}

FVector UPulsePhysicChaosLibrary::ChaosGetLinearVelocityAtPoint(UPrimitiveComponent* Component, FVector Point, FName BoneName)
{
	if (Chaos::FRigidBodyHandle_Internal* RigidHandle = GetInternalHandle(Component, BoneName))
	{
		if (ensure(RigidHandle->CanTreatAsKinematic()))
		{
			const bool bIsRigid = RigidHandle->CanTreatAsRigid();
			const Chaos::FVec3 COM = bIsRigid
				                         ? Chaos::FParticleUtilitiesGT::GetCoMWorldPosition(RigidHandle)
				                         : static_cast<Chaos::FVec3>(
					                         Chaos::FParticleUtilitiesGT::GetActorWorldTransform(RigidHandle).
					                         GetTranslation());
			const Chaos::FVec3 Diff = Point - COM;
			return RigidHandle->V() - Chaos::FVec3::CrossProduct(Diff, RigidHandle->W());
		}
	}
	return FVector::ZeroVector;
}

bool UPulsePhysicChaosLibrary::ConeTraceMulti(const UObject* WorldContextObject, const FVector Start, const FRotator Direction, float ConeHeight, float ConeRadius,
                                              TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
                                              EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
                                              FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const FVector v = FVector::ForwardVector;
	const FVector n = FVector::UpVector;
	TArray<FVector> pointCloud = {};
	// Base
	UPulseMathLibrary::CircleArcPoints(Start + v * ConeHeight, ConeRadius, 360, v, n, pointCloud, 18, true);
	// Central
	pointCloud.Add(Start);
	// Remove world offset
	for (int i = 0; i < pointCloud.Num(); ++i)
		pointCloud[i] -= Start;
	return SweepConvexHullMulti(WorldContextObject, pointCloud, Start, Start, Direction, TraceChannel, bTraceComplex, ActorsToIgnore, OutHits, DrawDebugType, TraceColor,
	                            TraceHitColor,
	                            DrawTime, true);
}

bool UPulsePhysicChaosLibrary::DiscTraceMulti(const UObject* WorldContextObject, const FVector Start, const FRotator Direction, FVector2D Radiuses, float ArcDegAngle,
                                              float Thickness, TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
                                              EDrawDebugTrace::Type DrawDebugType, TArray<FHitResult>& OutHits,
                                              FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
{
	const float r_min = FMath::Min(Radiuses.Y, Radiuses.X);
	const float r_max = FMath::Max(Radiuses.Y, Radiuses.X);
	if (r_min == r_max)
		return false;
	if (r_max <= 0)
		return false;
	if (Thickness <= 0)
		return false;
	const FVector v = FVector::ForwardVector;
	const FVector n = FVector::UpVector;
	TArray<FVector> pointCloud = {};
	// Upper out
	if (!UPulseMathLibrary::CircleArcPoints(Start + n * Thickness, r_max, ArcDegAngle, n, v, pointCloud, 18, true))
		return false;
	// lower out
	UPulseMathLibrary::CircleArcPoints(Start - n * Thickness, r_max, ArcDegAngle, n, v, pointCloud, 18, true);
	if (r_min > 0)
	{
		// upper in
		UPulseMathLibrary::CircleArcPoints(Start + n * Thickness, r_min, ArcDegAngle, n, v, pointCloud, 18, true);
		// lower in
		UPulseMathLibrary::CircleArcPoints(Start - n * Thickness, r_min, ArcDegAngle, n, v, pointCloud, 18, true);
	}
	else
	{
		// upper Central
		pointCloud.Add(Start + n * Thickness);
		// lower Central
		pointCloud.Add(Start - n * Thickness);
	}
	// Remove world offset
	for (int i = 0; i < pointCloud.Num(); ++i)
		pointCloud[i] -= Start;
	return SweepConvexHullMulti(WorldContextObject, pointCloud, Start, Start, Direction, TraceChannel, bTraceComplex, ActorsToIgnore, OutHits, DrawDebugType, TraceColor,
	                            TraceHitColor,
	                            DrawTime, true);
}

bool UPulsePhysicChaosLibrary::SweepConvexHullMulti(const UObject* WorldContextObject, const TArray<FVector>& Points, const FVector& Start, const FVector& End,
                                                    const FRotator& Rotation, TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex,
                                                    const TArray<AActor*>& ActorsToIgnore, TArray<FHitResult>& OutHits,
                                                    EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime, bool bCacheGeometry)
{
	OutHits.Empty();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	// Validate input points
	if (Points.Num() < 4)
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("SweepConvexHullMulti: At least 4 points are required to form a 3D convex hull"));
		return false;
	}


	// Build convex shape from points
	auto ConvexShape = BuildConvexFromPoints(Points, bCacheGeometry ? World : nullptr, DrawDebugType != EDrawDebugTrace::None);
	if (!ConvexShape.IsValid())
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("SweepConvexHullMulti: Failed to build convex hull from points"));
		return false;
	}

	// Build query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = bTraceComplex;
	QueryParams.AddIgnoredActors(ActorsToIgnore);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(TraceChannel, ECR_Overlap);

	FCollisionObjectQueryParams ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;

	FGenericPhysicsInterface::GeomSweepMulti(World, FChaosEngineInterface::GetGeometryCollection(*ConvexShape), Rotation.Quaternion()
	                                         , OutHits, Start, End, TraceChannel, QueryParams, ResponseParams, ObjectParams);
	bool result = OutHits.Num() > 0;

#if ENABLE_DRAW_DEBUG
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		FLinearColor debugColor = result ? TraceHitColor : TraceColor;
		float duration = DrawDebugType == EDrawDebugTrace::Persistent ? TNumericLimits<float>::Max() : (DrawDebugType == EDrawDebugTrace::ForDuration ? DrawTime : 0);
		UPulseDebugLibrary::DrawDebugConvexHull(WorldContextObject, Points, FTransform(Rotation, Start), debugColor, duration);
		UPulseDebugLibrary::DrawDebugConvexHull(WorldContextObject, Points, FTransform(Rotation, End), debugColor, duration);
		UKismetSystemLibrary::DrawDebugLine(WorldContextObject, Start, End, debugColor, duration, 1);
		for (const auto& hitResult : OutHits)
		{
			const auto quat = UKismetMathLibrary::MakeRotFromZX(hitResult.ImpactNormal, -hitResult.Normal).Quaternion();
			UPulseDebugLibrary::DrawDebugBasis(WorldContextObject, hitResult.ImpactPoint, quat.GetForwardVector(), quat.GetRightVector()
			                                   , quat.GetUpVector(), duration, 1);
		}
	}
#endif
	if (bCacheGeometry)
	{
		if (!UPulseWorldCacheSubSystem::CachePhysicConvexShapeFromPointCloud(WorldContextObject, TSet(Points), ConvexShape))
			UE_LOG(LogPulsePhysics, Warning, TEXT("OverlapConvexHullMulti: Failed to cache convex hull"));
	}
	return result;
}

bool UPulsePhysicChaosLibrary::OverlapConvexHullMulti(const UObject* WorldContextObject, const TArray<FVector>& Points, const FVector& Location, const FRotator& Rotation,
                                                      TEnumAsByte<ECollisionChannel> TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore,
                                                      TArray<UPrimitiveComponent*>& OutOverlaps,
                                                      EDrawDebugTrace::Type DrawDebugType, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime, bool bCacheGeometry)
{
	OutOverlaps.Empty();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	// Validate input points
	if (Points.Num() < 4)
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("OverlapConvexHullMulti: At least 4 points are required to form a 3D convex hull"));
		return false;
	}


	// Build convex shape from points
	auto ConvexShape = BuildConvexFromPoints(Points, bCacheGeometry ? World : nullptr, DrawDebugType != EDrawDebugTrace::None);
	if (!ConvexShape.IsValid())
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("OverlapConvexHullMulti: Failed to build convex hull from points"));
		return false;
	}

	// Build query parameters
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = bTraceComplex;
	QueryParams.AddIgnoredActors(ActorsToIgnore);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(TraceChannel, ECR_Overlap);

	FCollisionObjectQueryParams ObjectParams = FCollisionObjectQueryParams::DefaultObjectQueryParam;
	TArray<FOverlapResult> Overlaps;

	FGenericPhysicsInterface::GeomOverlapMulti(World, FChaosEngineInterface::GetGeometryCollection(*ConvexShape), Location, Rotation.Quaternion()
	                                           , Overlaps, TraceChannel, QueryParams, ResponseParams, ObjectParams);
	bool result = Overlaps.Num() > 0;
	for (const auto& OverlapResult : Overlaps)
	{
		OutOverlaps.Add(OverlapResult.GetComponent());
	}

#if ENABLE_DRAW_DEBUG
	if (DrawDebugType != EDrawDebugTrace::None)
	{
		FLinearColor debugColor = result ? TraceHitColor : TraceColor;
		float duration = DrawDebugType == EDrawDebugTrace::Persistent ? TNumericLimits<float>::Max() : (DrawDebugType == EDrawDebugTrace::ForDuration ? DrawTime : 0);
		UPulseDebugLibrary::DrawDebugConvexHull(WorldContextObject, Points, FTransform(Rotation, Location), debugColor, duration);
	}
#endif
	if (bCacheGeometry)
	{
		if (!UPulseWorldCacheSubSystem::CachePhysicConvexShapeFromPointCloud(WorldContextObject, TSet(Points), ConvexShape))
			UE_LOG(LogPulsePhysics, Warning, TEXT("OverlapConvexHullMulti: Failed to cache convex hull"));
	}
	return result;
}


Chaos::FImplicitObjectPtr UPulsePhysicChaosLibrary::BuildConvexFromPoints(const TArray<FVector>& PointCloud, const UWorld* CacheContext, bool bDebug)
{
	if (PointCloud.Num() < 4)
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("Need at least 4 non-coplanar points for a 3-D convex hull."));
		return nullptr;
	}

	if (CacheContext != nullptr)
	{
		auto cached = UPulseWorldCacheSubSystem::GetPhysicConvexShapeFromPointCloud(CacheContext, TSet(PointCloud));
		if (cached.IsValid())
			return cached;
	}

	using FVec3Type = Chaos::FConvexBuilder::FVec3Type;
	using FPlaneType = Chaos::FConvex::FPlaneType; // TPlaneConcrete<FReal,3>
	using FAABB3Type = Chaos::FConvexBuilder::FAABB3Type;

	// ==================================================================
	// STEP 1 — Convert FVector array to Chaos::FVec3 (double-precision)
	TArray<FVec3Type> ChaosVerts;
	ChaosVerts.Reserve(PointCloud.Num());
	for (const FVector& V : PointCloud)
	{
		ChaosVerts.Emplace(static_cast<Chaos::FReal>(V.X), static_cast<Chaos::FReal>(V.Y), static_cast<Chaos::FReal>(V.Z));
	}


	// ==================================================================
	// STEP 2 — Run FConvexBuilder::Build()
	TArray<FPlaneType> OutPlanes; // One plane per hull face
	// polygon faces — need fan-tri later. TArray<TArray<int32>>, one inner array per face; each inner array is the ordered CCW polygon of hull-vertex indices (3+ verts per polygon face)
	TArray<TArray<int32>> OutFaceIndices;
	TArray<FVec3Type> OutVertices; // Deduplicated hull vertex positions
	FAABB3Type OutLocalBounds; // axis-aligned bounding box of the hull
	Chaos::FConvexBuilder::Build(ChaosVerts, OutPlanes, OutFaceIndices, OutVertices, OutLocalBounds);
	if (OutVertices.Num() < 4 || OutFaceIndices.Num() < 4)
	{
		UE_LOG(LogPulsePhysics, Warning, TEXT("Convex hull degenerate: %d verts, %d faces."), OutVertices.Num(), OutFaceIndices.Num());
		return nullptr;
	}

	// ==================================================================
	// STEP 3 — Pack hull vertices into the flat float array expected by NewGeometryCollection: [X0,Y0,Z0, X1,Y1,Z1, ...]
	TArray<float> RawVerts;
	RawVerts.Reserve(OutVertices.Num() * 3);
	for (const FVec3Type& HV : OutVertices)
	{
		RawVerts.Add(static_cast<float>(HV.X));
		RawVerts.Add(static_cast<float>(HV.Y));
		RawVerts.Add(static_cast<float>(HV.Z));
	}

	// ==================================================================
	// STEP 4 — Fan-triangulate every polygon face → flat int32 index buffer
	//  OutFaceIndices[f] = { i0, i1, i2, i3, … }  (CCW polygon)
	//  Fan from i0:
	//    triangle 0: i0, i1, i2
	//    triangle 1: i0, i2, i3
	//    ...
	//    triangle k: i0, i(k+1), i(k+2)     for k in [0, N-3)
	//  This correctly handles triangles, quads, and n-gon hull faces.
	TArray<int32> RawIndices;
	int32 TotalTris = 0;
	for (const TArray<int32>& Face : OutFaceIndices)
	{
		TotalTris += FMath::Max(0, Face.Num() - 2);
	}
	RawIndices.Reserve(TotalTris * 3);
	for (const TArray<int32>& Face : OutFaceIndices)
	{
		// Need at least 3 vertices to form a triangle
		if (Face.Num() < 3)
		{
			continue;
		}
		// Fan triangulation: anchor at Face[0]
		const int32 Anchor = Face[0];
		for (int32 k = 1; k < Face.Num() - 1; ++k)
		{
			RawIndices.Add(Anchor);
			RawIndices.Add(Face[k]);
			RawIndices.Add(Face[k + 1]);
		}
	}

	// ==================================================================
	// STEP 5 — Build the FGeometryCollectionDefaults
	FGeometryCollectionDefaults GCDefaults; // GCDefaults.InitialTransform = FTransform::Identity; // GCDefaults.MaterialIndex = 0; .already defaults

	// ==================================================================
	// STEP 6 — Call the static factory
	//  bReverseVertexOrder = false  →  keep CCW winding from Chaos hull
	//  The hull faces are already outward-facing CCW, which matches UE's
	//  default front-face convention.  Pass true only if normals are inward.
	FGeometryCollection* RawGC = FGeometryCollection::NewGeometryCollection(RawVerts, RawIndices, /*bReverseVertexOrder=*/ false, GCDefaults);

	if (!RawGC)
	{
		UE_LOG(LogPulsePhysics, Error, TEXT("FGeometryCollection::NewGeometryCollection returned null. Triangulated hull: %d triangles from %d polygon faces."),
		       RawIndices.Num() / 3, OutFaceIndices.Num());
		return nullptr;
	}

	if (bDebug)
	{
		UE_LOG(LogPulsePhysics, Log, TEXT("Hull: %d hull-verts, %d polygon faces. Remember to cache the hull for performance reasons"), OutVertices.Num(), OutFaceIndices.Num());
	}

	return FGeometryCollectionConvexUtility::GetConvexHull(RawGC, 0).GetReference();
}

Chaos::FRigidBodyHandle_Internal* UPulsePhysicChaosLibrary::GetInternalHandle(UPrimitiveComponent* Component, FName BoneName)
{
	if (IsValid(Component))
	{
		if (const FBodyInstance* BodyInstance = Component->GetBodyInstance(BoneName))
		{
			if (const auto Handle = BodyInstance->ActorHandle)
			{
				if (Chaos::FRigidBodyHandle_Internal* RigidHandle = Handle->GetPhysicsThreadAPI())
				{
					return RigidHandle;
				}
			}
		}
	}
	return nullptr;
}
