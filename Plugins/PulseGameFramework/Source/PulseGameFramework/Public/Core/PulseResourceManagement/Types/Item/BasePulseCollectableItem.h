// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BasePulseBaseItemAsset.h"
#include "BasePulseCollectableItem.generated.h"




/**
 * Serve as a base class for all collectable item (skills, Buffs, etc..) game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseCollectableItem : public UBasePulseBaseItemAsset
{
	GENERATED_BODY()
};
