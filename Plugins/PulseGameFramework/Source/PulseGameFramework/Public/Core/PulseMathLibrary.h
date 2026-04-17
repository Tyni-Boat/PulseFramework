// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PulseMathLibrary.generated.h"

/**
 * Library of tools for math operations
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Transform an input alpha [0-1] into a [0-1] output evaluated from a curve type.
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="CurveAlpha", AdvancedDisplay = 1))
	static float AlphaByCurve(const float InAlpha, EAlphaBlendOption CurveType = EAlphaBlendOption::Linear, UCurveFloat* FloatCurve = nullptr);

	// Transform from local space to world space. Use pivot transform as reference.
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="TransformLocalToWorldBy"))
	static FTransform TransformByTransform(const FTransform& PivotTransform, const FTransform& TransformToConvert);

	// Transform from world space to local space. Use pivot transform as reference.
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="TransformWorldToLocalBy"))
	static FTransform InverseTransformByTransform(const FTransform& PivotTransform, const FTransform& TransformToConvert);

	// Check if a transform is valid
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="IsValidTransform"))
	static bool IsTransformValid(const FTransform& Transform);

	// Make an orthonormal basis from a vector
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="OrthoBasis", AdvancedDisplay = 1))
	static void MakeOrthoBasis(const FVector& vector, FVector& OutRight, FVector& OutUp, float vectorRollInDegrees = 0);

	// Get the lenght of a circle arc
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="ArcLenght", AdvancedDisplay = 1))
	static float CircleArcLenght(const float Radius, const float radAngle);

	/**
	 * @brief Return a list of points on a circle arc
	 * @param Location - The circle center world offset
	 * @param Radius - Radius of the circle
	 * @param DegreeAngle - The arc angle in degrees
	 * @param Axis - The vector orthogonal to the circle. may be changed to be orthogonal to the heading vector
	 * @param HeadingVector - The vector representing the angle zero
	 * @param OutPoints - World space point on the arc. The array is not reset.
	 * @param PiRadSegmentCount - How many segments shall have been used if the arc is a half circle
	 * @param bUseCentralHeading - The heading vector will no longer point to zero but RadAngle / 2
	 * @return true if the circle got at least on point added 
	 */
	UFUNCTION(BlueprintPure, Category="PulseCore|Math", meta=(DisplayName="ArcPoints", AdvancedDisplay = 1, AutoCreateRefTerm = "Location,Axis,HeadingVector"))
	static bool CircleArcPoints(const FVector& Location, float Radius, float DegreeAngle, const FVector& Axis, const FVector& HeadingVector, TArray<FVector>& OutPoints,
	                            int32 PiRadSegmentCount = 18, bool bUseCentralHeading = false);
};
