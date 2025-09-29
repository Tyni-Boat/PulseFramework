// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BasePulseBaseItemAsset.h"
#include "BasePulseCharacterItem.generated.h"




/**
 * Serve as a base class for all character item (weapon, armor, etc..) game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseCharacterItem : public UBasePulseBaseItemAsset
{
	GENERATED_BODY()
};
