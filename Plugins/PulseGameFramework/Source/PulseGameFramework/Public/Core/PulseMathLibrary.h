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
	UFUNCTION(BlueprintPure, Category=PulseMath, meta=(DisplayName="CurveAlpha", AdvancedDisplay = 1))
	static float AlphaByCurve(const float InAlpha, EAlphaBlendOption CurveType = EAlphaBlendOption::Linear, UCurveFloat* FloatCurve = nullptr);

	// Transform from local space to world space. Use pivot transform as reference.
	UFUNCTION(BlueprintPure, Category=PulseMath, meta=(DisplayName="TransformLocalToWorldBy"))
	static FTransform TransformByTransform(const FTransform& PivotTransform, const FTransform& TransformToConvert);

	// Transform from world space to local space. Use pivot transform as reference.
	UFUNCTION(BlueprintPure, Category=PulseMath, meta=(DisplayName="TransformWorldToLocalBy"))
	static FTransform InverseTransformByTransform(const FTransform& PivotTransform, const FTransform& TransformToConvert);

	// Check if a transform is valid
	UFUNCTION(BlueprintPure, Category=PulseMath, meta=(DisplayName="IsValidTransform"))
	static bool IsTransformValid(const FTransform& Transform);
};
