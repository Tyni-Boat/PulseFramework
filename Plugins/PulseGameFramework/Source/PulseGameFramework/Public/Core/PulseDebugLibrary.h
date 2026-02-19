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
	static void DrawDebugTransform(const UObject* WorldContext, const FTransform Transform, FLinearColor Color, float Duration = 1, float Size = 3);
	
	// Check if 3 vector are orthonormal, and draw them in color if they are and gray if they are not.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Debug|SceneDrawing", meta = (WorldContext = "WorldContext"))
	static void DrawDebugBasis(const UObject* WorldContext, const FVector DrawLocation, const FVector ForwardVector, const FVector RightVector, const FVector UpVector, float Duration = 1, float Size = 3);

	static FString DebugNetLog(const UObject* Obj, const FString& message = "LogMessage");
	
	static FString NetModeStr(const UObject* Obj);

	static int32 NetClientId(const UObject* Obj);
};
