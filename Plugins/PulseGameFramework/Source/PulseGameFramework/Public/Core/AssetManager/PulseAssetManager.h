// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "PulseContentProvider.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/GameAssetTypes.h"
#include "Core/PulseCoreTypes.h"
#include "Engine/AssetManager.h"
#include "PulseAssetManager.generated.h"


USTRUCT()
struct FPAkAssetReferenceKey
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TSubclassOf<UObject> Class = nullptr;
	UPROPERTY()
	FString BundleName = "";

	bool operator==(const FPAkAssetReferenceKey& Other) const
	{
		return Class == Other.Class && BundleName == Other.BundleName;
	}
};

FORCEINLINE uint32 GetTypeHash(const FPAkAssetReferenceKey& Key)
{
	return HashCombine(GetTypeHash(Key.Class), GetTypeHash(Key.BundleName));
}

USTRUCT()
struct FPAkAssetReferenceEntry
{
	GENERATED_BODY()
public:
	TArray<FString> ContentPaths = {};
	TArray<FString> ModularGameplayPlugins = {};
	TArray<FAssetData> AssetReferences = {};
};

USTRUCT()
struct FPAkMountRequest
{
	GENERATED_BODY()
public:
	TSubclassOf<UPulseContentProvider> ProviderClass = nullptr;
	FPulseBundleDescriptor BundleDescriptor = {};
};


class UBasePulseAsset;

UCLASS()
class UPulseAssetManager : public UAssetManager, public IIPulseCore
{
	GENERATED_BODY()

private:

	bool MountPak(const FString& PakPath, const int32 Version);
	bool UnmountPak(const FString& PakPath);

public:
	UPROPERTY()
	TMap<TSubclassOf<UPulseContentProvider>, TObjectPtr<UPulseContentProvider>> ContentProviders;
	UPROPERTY()
	FPulseAssetsManifest AssetsManifest;
	UPROPERTY()
	FPulseAssetsManifest DLCAssetsManifest;
	UPROPERTY()
	TSet<FPrimaryAssetId> AssetCheckList;
	UPROPERTY()
	TMap<FPulseAssetId, FPulseAssetsManifestEntryPack> DLCVersioningEntries;
	UPROPERTY()
	TMap<FPAkAssetReferenceKey, FPAkAssetReferenceEntry> PakAssetReferences;
	UPROPERTY()
	TArray<FPAkMountRequest> ContentMountRequests;
	UPROPERTY()
	FPAkMountRequest CurrentMountRequest;
	
	static UPulseAssetManager* Get()
	{
		return Cast<UPulseAssetManager>(UAssetManager::GetIfInitialized());
	}
	
	// Get the save path of game content.
	static FString GetGameContentSavePath();

	// Get the save path of additional contents
	static FString GetDLCSavePath();

	// Get the download path of additional contents
	static FString GetDLCDownloadPath();
	
	UFUNCTION()
	void OnBundleReadyToMount(TSubclassOf<UPulseContentProvider> Class, const FString& FilePath, bool bSucess);
	void TryToAttributePostLoadedAssetToCurrentBundle(const FAssetData& AssetData);
	void TryToRemovePostLoadedAssetFromCurrentBundle(const FAssetData& AssetData);

	void OnPostAssetAdded(const FAssetData& AssetData);
	void OnPostAssetRemoved(const FAssetData& AssetData);
	void OnPostAssetUpdated(TArrayView<const FAssetData> AssetDatas);

	virtual void StartInitialLoading() override;
	bool AssetExist(const FPrimaryAssetId Id) const;
	void LoadAssetsManifest();
	void SaveAssetsManifest() const;
	bool QueryPrimaryAssetID(const FName& ClassFName, const int32& Id, FPrimaryAssetId& OutAssetID, bool bIncludePlaceholders = false);
	bool QueryPulseAssetID(const FPrimaryAssetId& PrimaryId, FPulseAssetId& OutAssetID, bool bIncludePlaceholders = false);
	void MountPendingPakPackages();
	void UnMountContentPackage(const FString& BundleName, bool bDeleteBundle = false);

	// Return on success an array of the same size of Id array
	void AsyncLoadPulseAssets(const TSubclassOf<UBasePulseAsset> Class, const TArray<int32>& Ids, const TArray<FName>& LoadBundles = TArray<FName>(), TFunction<void(TArray<UPrimaryDataAsset*>&)> OnSuccess = nullptr, TFunction<void()> OnFailed = nullptr, bool bIncludePlaceholderAssets = false);

	virtual void BeginDestroy() override;
};



UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseBPAssetManager : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "BundleFlags", Keywords = "bundle, flag"))
	static TArray<FName> GetDataBundleFromFlags(UPARAM(meta = (Bitmask, BitmaskEnum = EDataBundleType))
		int32 flag);

	// Try to get an asset object's ID
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ObjectID", Keywords = "id, object"))
	static bool TryGetObjectAssetID(UObject* Object, int32& OutID);

	// Try to get the primary Type of a class
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ClassToAssetType", Keywords = "class, asset"))
	static bool TryGetPrimaryAssetType(UPARAM(meta=(AllowAbstract=false))
	                                   const TSubclassOf<UBasePulseAsset> Type, FPrimaryAssetType& OutAssetType);

	// Try to get the Type of a Primary asset Type
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "AssetTypeToClass", Keywords = "asset, class"))
	static bool TryGetClassAssetType(const FPrimaryAssetType& AssetType, TSoftClassPtr<UObject>& OutClass);

	// Try to get the primary asset id of this type and Id
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "ToPrimaryID", Keywords = "id, primary"))
	static bool TryGetPrimaryAssetID(UPARAM(meta=(AllowAbstract=false))
	                                 const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FPrimaryAssetId& OutAssetID);

	// Try to get the asset type, name and ID from primary Asset ID
	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Tools", meta = (CompactNodeTitle = "FromPrimaryID", Keywords = "id, primary"))
	static bool TryGetPrimaryAssetInfos(const FPrimaryAssetId& AssetID, TSoftClassPtr<UObject>& OutClass, FName& OutAssetName, int32& OutAssetId);

	UFUNCTION(BlueprintPure, Category = "PulseCore|AssetManager|Game Assets Queries", meta = (CompactNodeTitle = "AllAssets", Keywords = "asset, all"))
	static TArray<FPrimaryAssetId> GetAllAssetsOfType(UPARAM(meta=(AllowAbstract=false))
		const TSubclassOf<UObject> Type);

	// Load A Game Asset
	static bool LoadPulseAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FOnAssetLoaded& CallBack, const TArray<FName>& Bundles = {BUNDLE_INFOS});

	// Load All game Assets of type T
	static bool LoadAllPulseAssets(const TSubclassOf<UBasePulseAsset> Type, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles = {BUNDLE_INFOS});

	// Load batch of game Assets of type T
	static bool LoadMultipleAssets(const TSubclassOf<UBasePulseAsset> Type, TArray<int32> Ids, FOnMultipleAssetsLoaded& CallBack,
	                               const TArray<FName>& Bundles = {BUNDLE_INFOS});

	UFUNCTION(BlueprintCallable, Category = "PulseCore|AssetManager|Game Assets Actions")
	static void TryUnloadGameAsset(UPARAM(meta=(AllowAbstract=false))
	                               const TSubclassOf<UBasePulseAsset> Type, const int32 Id);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|AssetManager|Game Assets Actions")
	static void TryUnloadAllGameAssets(UPARAM(meta=(AllowAbstract=false))
		const TSubclassOf<UBasePulseAsset> Type);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|AssetManager|Game Assets Queries")
	static bool TryGetControlledCharacterID(const AController* Controller, FPrimaryAssetId& OutCharacterID);
};
