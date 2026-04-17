// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PulseDebugLibrary.generated.h"

/**
 * Library of tools made for debugging
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	
	// Draw a point at the location and an Arrows in the forward direction of the rotation
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugTransform(const UObject* WorldContext, const FTransform Transform, FLinearColor Color, float Duration = 1, float Size = 1);
	
	// Draw a circle at a location on a normal axis
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugCircle(const UObject* WorldContext, const FVector Location, float Radius, FVector Normal, FLinearColor Color, float Duration = 1, float Size = 1);
	
	// Draw an arc of a circle at a location on with an axis and heading vector (0 degree vector).
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugArcCircle(const UObject* WorldContext, const FVector Location, float Radius, float DegreeAngle, FVector Axis, FVector HeadingVector
		, FLinearColor Color, float Duration = 1, float Size = 1, float StartArrowSize = 0, float EndArrowSize = 0, int32 SegmentFor360 = 36, bool bUseCentralHeading = false);
	
	// Draw a path make of ordered world space points
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugPath(const UObject* WorldContext, const TArray<FVector>& Path, FLinearColor Color, float Duration = 1, float Size = 1);
	
	// Check if 3 vector are orthonormal, and draw them in color if they are and gray if they are not.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugBasis(const UObject* WorldContext, const FVector DrawLocation, const FVector ForwardVector, const FVector RightVector, const FVector UpVector, float Duration = 1, float Size = 1);

	// Draw a primitive shape with extents at location with rotation.
	// Shape type: 0-line, 1-Capsule, 2-Box, 3-Sphere
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugPrimitiveShape(const UObject* WorldContext, uint8 ShapeType, const FVector Extents, const FVector Location, const FRotator Rotation, FLinearColor Color, float Duration = 1, float Size = 1);
	

	// Helper to draw debug visualization for a convex hull
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext", AutoCreateRefTerm = "Color"))
	static void DrawDebugConvexHull(const UObject* WorldContext, const TArray<FVector>& Points, const FTransform& Transform, const FLinearColor& Color, float Duration = 1, float Size = 1);
	
	static FString DebugNetLog(const UObject* Obj, const FString& message = "LogMessage");
	
	static FString NetModeStr(const UObject* Obj);

	static int32 NetClientId(const UObject* Obj);
};
