// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/GameAssetTypes.h"
#include "Core/CoreTypes.h"
#include "Core/PulseSubModuleBase.h"
#include "PulseAssetManager.generated.h"


UCLASS()
class UPulseAssetManagerSubModule : public UPulseSubModuleBase
{
	GENERATED_BODY()

public:
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
};


class UBasePulseAsset;

UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseAssetManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Tools", meta = (CompactNodeTitle = "BundleFlags", Keywords = "bundle, flag"))
	static TArray<FName> GetDataBundleFromFlags(UPARAM(meta = (Bitmask, BitmaskEnum = EDataBundleType))
		int32 flag);

	// Try to get an asset object's ID
	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ObjectID", Keywords = "id, object"))
	static bool TryGetObjectAssetID(UObject* Object, int32& OutID);

	// Try to get the primary Type of a class
	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ClassToAssetType", Keywords = "class, asset"))
	static bool TryGetPrimaryAssetType(UPARAM(meta=(AllowAbstract=false))
	                                   const TSubclassOf<UBasePulseAsset> Type, FPrimaryAssetType& OutAssetType);

	// Try to get the Type of a Primary asset Type
	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Tools", meta = (CompactNodeTitle = "AssetTypeToClass", Keywords = "asset, class"))
	static bool TryGetClassAssetType(const FPrimaryAssetType& AssetType, TSoftClassPtr<UObject>& OutClass);

	// Try to get the primary asset id of this type and Id
	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ToPrimaryID", Keywords = "id, primary"))
	static bool TryGetPrimaryAssetID(UPARAM(meta=(AllowAbstract=false))
	                                 const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FPrimaryAssetId& OutAssetID);

	// Try to get the asset type, name and ID from primary Asset ID
	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Tools", meta = (CompactNodeTitle = "FromPrimaryID", Keywords = "id, primary"))
	static bool TryGetPrimaryAssetInfos(FPrimaryAssetId AssetID, TSoftClassPtr<UObject>& OutClass, FName& OutAssetName, int32& OutAssetId);

	UFUNCTION(BlueprintPure, Category = "PulseAssetManager|Game Assets Queries", meta = (CompactNodeTitle = "AllAssets", Keywords = "asset, all"))
	static TArray<FPrimaryAssetId> GetAllGameAssets(UPARAM(meta=(AllowAbstract=false))
		const TSubclassOf<UBasePulseAsset> Type);

	// Load A Game Asset
	static bool LoadAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FOnAssetLoaded& CallBack, const TArray<FName>& Bundles = {BUNDLE_INFOS});

	// Load All game Assets of type T
	static bool LoadAllAssets(const TSubclassOf<UBasePulseAsset> Type, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles = {BUNDLE_INFOS});

	// Load All game Assets of type T
	static bool LoadMultipleAssets(const TMap<TSubclassOf<UBasePulseAsset>, TArray<int32>> TypeBatches, FOnMultipleAssetsLoaded& CallBack,
	                               const TArray<FName>& Bundles = {BUNDLE_INFOS});

	UFUNCTION(BlueprintCallable, Category = "PulseAssetManager|Game Assets Actions")
	static void TryUnloadGameAsset(UPARAM(meta=(AllowAbstract=false))
	                               const TSubclassOf<UBasePulseAsset> Type, const int32 Id);

	UFUNCTION(BlueprintCallable, Category = "PulseAssetManager|Game Assets Actions")
	static void TryUnloadAllGameAssets(UPARAM(meta=(AllowAbstract=false))
		const TSubclassOf<UBasePulseAsset> Type);

	UFUNCTION(BlueprintCallable, Category = "PulseAssetManager|Game Assets Queries")
	static bool TryGetControlledCharacterID(const AController* Controller, FPrimaryAssetId& OutCharacterID);
};
