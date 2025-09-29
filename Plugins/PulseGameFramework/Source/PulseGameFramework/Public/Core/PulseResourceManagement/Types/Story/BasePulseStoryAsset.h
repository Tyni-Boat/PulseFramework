// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/PulseResourceManagement/Types/BasePulseAsset.h"
#include "BasePulseStoryAsset.generated.h"




/**
 * Serve as a base class for all Story game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseStoryAsset: public UBasePulseAsset
{
	GENERATED_BODY()
};
