// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/GameAssetTypes.h"
#include "UObject/Object.h"
#include "AsyncLoadBatchGameAsset.generated.h"



// Async task to load multiple game assets in batch
UCLASS()
class PULSEGAMEFRAMEWORK_API UAsyncLoadBatchGameAsset : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Blueprint node exposed function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AdvancedDisplay = 3, AutoCreateRefTerm = "AssetIds"), Category = "PulseAssetManager|Game Assets Queries")
	static UAsyncLoadBatchGameAsset* LoadGameAssetBatch(UObject* WorldContextObject,UPARAM(meta=(AllowAbstract=false))
	                                                      const TSubclassOf<UBasePulseAsset> Type,
	                                                      const TArray<int32>& AssetIds,
	                                                      UPARAM(meta = (Bitmask, BitmaskEnum = EDataBundleType))
	                                                      int32 flag);

	// Delegate to notify when the task is complete
	UPROPERTY(BlueprintAssignable)
	FOnMultipleAssetsLoaded OnAssetsLoaded;

protected:
	virtual void Activate() override;

	UFUNCTION()
	void OnAllAssetLoaded_Internal(FAssetPack Assets);

private:
	UPROPERTY()
	UObject* _worldContext;
	UPROPERTY()
	UClass* _Type;
	TArray<int32> _Ids;
	TArray<FName> _assetBundles;
};