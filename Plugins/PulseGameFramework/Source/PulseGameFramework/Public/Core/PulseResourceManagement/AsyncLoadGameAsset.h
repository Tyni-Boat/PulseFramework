// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Types/GameAssetTypes.h"
#include "AsyncLoadGameAsset.generated.h"


// Async task to a load game asset
UCLASS()
class PULSEGAMEFRAMEWORK_API UAsyncLoadGameAsset : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	// Blueprint node exposed function
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", AdvancedDisplay = 3), Category = "PulseAssetManager|Game Assets Queries")
	static UAsyncLoadGameAsset* LoadGameAsset(UObject* WorldContextObject,UPARAM(meta=(AllowAbstract=false))
	                                          const TSubclassOf<UBasePulseAsset> Type, const int32 Id,
	                                          UPARAM(meta = (Bitmask, BitmaskEnum = EDataBundleType))
	                                          int32 flag);

	// Delegate to notify when the task is complete
	UPROPERTY(BlueprintAssignable)
	FOnAssetLoaded OnAssetLoaded;

protected:
	virtual void Activate() override;

	UFUNCTION()
	void OnAssetLoaded_Internal(UPrimaryDataAsset* Asset);

private:
	UPROPERTY()
	UObject* _worldContext;
	UPROPERTY()
	UClass* _Type;
	int32 _ID;
	TArray<FName> _assetBundles;
};