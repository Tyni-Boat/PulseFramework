// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BasePulseBaseItemAsset.h"
#include "BasePulseConsumableItem.generated.h"




/**
 * Serve as a base class for all Consumable item (potion, food, etc..) game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseConsumableItem : public UBasePulseBaseItemAsset
{
	GENERATED_BODY()
};
