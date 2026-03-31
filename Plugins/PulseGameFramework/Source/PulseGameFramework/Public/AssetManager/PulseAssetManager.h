// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/GameAssetTypes.h"
#include "Core/PulseCoreTypes.h"
#include "Engine/AssetManager.h"
#include "PulseAssetManager.generated.h"




class UBasePulseAsset;

UCLASS()
class UPulseAssetManager : public UAssetManager, public IIPulseCore
{
	GENERATED_BODY()

private:

	TSet<FPulseAssetId> AssetSet = {};

public:	
	static UPulseAssetManager* Get()
	{
		return Cast<UPulseAssetManager>(UAssetManager::GetIfInitialized());
	}

	void OnPostAssetAdded(const FAssetData& AssetData);
	void OnPostAssetRemoved(const FAssetData& AssetData);
	void OnPostAssetUpdated(TArrayView<const FAssetData> AssetDatas);

	virtual void StartInitialLoading() override;
	bool AssetExist(const FPrimaryAssetId Id) const;
	bool QueryPrimaryAssetID(const FPrimaryAssetType& Type, const int32& Id, FPrimaryAssetId& OutAssetID) const;
	bool QueryPulseAssetID(const FPrimaryAssetId& PrimaryId, FPulseAssetId& OutAssetID) const;

	// Return on success an array of the same size of Id array
	void AsyncLoadPulseAssets(TSubclassOf<UBasePulseAsset> Class, const TSet<int32>& Ids, const TArray<FName>& LoadBundles = TArray<FName>(), TFunction<void(TArray<UPrimaryDataAsset*>&)> OnSuccess = nullptr, TFunction
	                          <void()> OnFailed = nullptr);

	virtual void BeginDestroy() override;
};



UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseBPAssetManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "BundleFlags", Keywords = "bundle, flag"))
	static TArray<FName> GetDataBundleFromFlags(UPARAM(meta = (Bitmask, BitmaskEnum = EPulseDataBundleType))
		int32 flag);

	// Try to get an asset object's ID
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ObjectID", Keywords = "id, object"))
	static bool GetAssetPulseID(UObject* Object, int32& OutID);

	// Try to get the primary Type of a class
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ClassToAssetType", Keywords = "class, asset"))
	static bool GetClassPrimaryAssetType(UPARAM(meta=(AllowAbstract=false))
	                                   const TSubclassOf<UBasePulseAsset> Type, FPrimaryAssetType& OutAssetType);

	// Try to get the Type of a Primary asset Type
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "AssetTypeToClass", Keywords = "asset, class"))
	static bool GetPrimaryTypeClass(const FPrimaryAssetType& AssetType, TSoftClassPtr<UObject>& OutClass);

	// Try to get the primary asset id of this type and Id
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ToPrimaryID", Keywords = "id, primary"))
	static bool GetPrimaryAssetID(UPARAM(meta=(AllowAbstract=false))
	                                 const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FPrimaryAssetId& OutAssetID);

	// Try to get the asset type, name and ID from primary Asset ID
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "FromPrimaryID", Keywords = "id, primary"))
	static bool GetPulseAssetInfos(const FPrimaryAssetId& AssetID, TSoftClassPtr<UObject>& OutClass, FName& OutAssetName, int32& OutAssetId);

	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Queries", meta = (CompactNodeTitle = "AllAssets", Keywords = "asset, all"))
	static TArray<FPrimaryAssetId> GetAllAssetsOfType(UPARAM(meta=(AllowAbstract=false))
		const TSubclassOf<UObject> Type);

	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Queries", meta = (CompactNodeTitle = "IsLoaded", Keywords = "asset, Loaded"))
	static bool IsAssetLoaded(const FPrimaryAssetId& AssetID);

	// Load A Game Asset
	static bool LoadPulseAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FOnAssetLoaded& CallBack, const TArray<FName>& Bundles = {BUNDLE_INFOS});

	// Load All game Assets of type T
	static bool LoadAllPulseAssets(const TSubclassOf<UBasePulseAsset> Type, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles = {BUNDLE_INFOS});

	// Load batch of game Assets of type T
	static bool LoadMultipleAssets(const TSubclassOf<UBasePulseAsset> Type, TSet<int32> Ids, FOnMultipleAssetsLoaded& CallBack,
	                               const TArray<FName>& Bundles = {BUNDLE_INFOS});

	UFUNCTION(BlueprintCallable, Category = "PulseCore|AssetManager|Game Assets Actions")
	static void UnloadGameAsset(UPARAM(meta=(AllowAbstract=false))
	                               const TSubclassOf<UBasePulseAsset> Type, const int32 Id);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|AssetManager|Game Assets Actions")
	static void UnloadAllGameAssets(UPARAM(meta=(AllowAbstract=false))
		const TSubclassOf<UBasePulseAsset> Type);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|AssetManager|Game Assets Queries")
	static bool GetActorAssetID(AActor* Actor, FPrimaryAssetId& OutActorID);
};
