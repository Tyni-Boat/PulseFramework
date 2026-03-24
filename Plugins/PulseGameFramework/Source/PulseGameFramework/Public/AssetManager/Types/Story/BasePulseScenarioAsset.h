// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AssetManager/Types/BasePulseAsset.h"
#include "BasePulseScenarioAsset.generated.h"




/**
 * Serve as a base class for all Story game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseScenarioAsset: public UBasePulseAsset
{
	GENERATED_BODY()
};
