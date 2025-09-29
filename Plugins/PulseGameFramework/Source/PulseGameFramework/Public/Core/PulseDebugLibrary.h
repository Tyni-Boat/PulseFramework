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
	UFUNCTION(BlueprintCallable, Category = "PulseDebug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugTransform(const UObject* WorldContext, const FTransform Transform, FLinearColor Color, float Duration = 1, float Size = 3);
};
