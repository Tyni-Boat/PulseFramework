// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/AssetManager/Types/GameAssetTypes.h"
#include "Core/AssetManager/Types/BasePulseAsset.h"
#include "BasePulseWorldAsset.generated.h"




/**
 * Serve as a base class for all world game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseWorldAsset: public UBasePulseAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_SPAWN, meta = (AssetBundles = BUNDLE_SPAWN))
	TSoftObjectPtr<UWorld> Map; // The world asset to load for this game asset
};
