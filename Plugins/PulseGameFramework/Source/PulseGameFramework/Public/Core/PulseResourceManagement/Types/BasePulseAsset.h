// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameAssetTypes.h"
#include "BasePulseAsset.generated.h"




/**
 * Serve as a base class for all game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseAsset: public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_INFOS, meta = (AssetBundles = BUNDLE_INFOS))
	FText Name = FText(); // Name of the asset, used for identification

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_INFOS, meta = (AssetBundles = BUNDLE_INFOS))
	FText Description = FText(); // Description of the asset, used for identification

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_UI, meta = (AssetBundles = BUNDLE_UI))
	TSoftObjectPtr<UTexture2D> Icon; // Icon of the asset, used for identification
};
