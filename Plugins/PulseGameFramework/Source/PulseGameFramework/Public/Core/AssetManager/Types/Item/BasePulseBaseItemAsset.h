// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/AssetManager/Types/BasePulseAsset.h"
#include "UObject/Object.h"
#include "BasePulseBaseItemAsset.generated.h"

/**
 * Serve as a base class for all item game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseBaseItemAsset : public UBasePulseAsset
{
	GENERATED_BODY()
	
public:
};
