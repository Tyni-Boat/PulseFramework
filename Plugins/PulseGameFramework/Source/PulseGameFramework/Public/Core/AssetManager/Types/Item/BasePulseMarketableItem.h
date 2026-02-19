// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BasePulseBaseItemAsset.h"
#include "BasePulseMarketableItem.generated.h"




/**
 * Serve as a base class for all Marketable item (currency, resource, etc..) game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseMarketableItem : public UBasePulseBaseItemAsset
{
	GENERATED_BODY()
};
