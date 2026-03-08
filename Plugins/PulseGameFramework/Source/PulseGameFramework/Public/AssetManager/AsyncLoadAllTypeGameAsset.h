// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/GameAssetTypes.h"
#include "UObject/Object.h"
#include "AsyncLoadAllTypeGameAsset.generated.h"



// Async task to all game assets of the same type
UCLASS()
class PULSEGAMEFRAMEWORK_API UAsyncLoadAllTypeGameAsset : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Blueprint node exposed function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AdvancedDisplay = 3), Category = "PulseCore|AssetManager|Game Assets Queries")
	static UAsyncLoadAllTypeGameAsset* LoadGameAssetAllOfType(UObject* WorldContextObject,UPARAM(meta=(AllowAbstract=false))
	                                                      const TSubclassOf<UBasePulseAsset> Type,
	                                                      UPARAM(meta = (Bitmask, BitmaskEnum = EPulseDataBundleType))
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
	TArray<FName> _assetBundles;
};
