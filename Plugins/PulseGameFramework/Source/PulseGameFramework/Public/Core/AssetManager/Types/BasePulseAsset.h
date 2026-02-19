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
class PULSEGAMEFRAMEWORK_API UBasePulseAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asset Infos")
	int32 AssetId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asset Infos")
	FDateTime LastModificationDate;

	// The version of the asset (X-Major; Y-Minor; Z-Patch)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asset Infos")
	FVector Version;

	// Is this asset a place holder asset, while the developer is still working on the actual content?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Asset Infos")
	bool bIsPlaceHolder;

#if WITH_EDITORONLY_DATA

	UPROPERTY(EditAnywhere)
	FString AssetPathForEditor;

#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_INFOS, meta = (AssetBundles = BUNDLE_INFOS))
	FText Name = FText(); // Name of the asset, used for identification

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_INFOS, meta = (AssetBundles = BUNDLE_INFOS))
	FText Description = FText(); // Description of the asset, used for identification

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_UI, meta = (AssetBundles = BUNDLE_UI))
	TSoftObjectPtr<UTexture2D> Icon; // Icon of the asset, used for identification

	inline virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(
			*FString::Printf(TEXT("%s"), *GetClass()->GetName()),
			*FString::Printf(TEXT("%s_%d"), *GetClass()->GetName(), AssetId)
		);
	}
};
