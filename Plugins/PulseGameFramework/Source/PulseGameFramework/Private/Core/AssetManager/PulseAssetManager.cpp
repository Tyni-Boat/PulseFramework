// Copyright � by Tyni Boat. All Rights Reserved.


#include "Core/AssetManager/PulseAssetManager.h"

#include "IPlatformFilePak.h"
#include "HAL/PlatformFilemanager.h"
#include "Core/PulseSystemLibrary.h"
#include "Core/AssetManager/PulseContentProvider.h"
#include "Engine/AssetManager.h"
#include "Core/AssetManager/Types/IIdentifiableActor.h"
#include "Core/AssetManager/Types/BasePulseAsset.h"
#include "GameFeaturesSubsystem.h"
#include "PulseGameFramework.h"
#include "IO/IoDispatcher.h"


bool UPulseAssetManager::MountPak(const FString& PakPath, const int32 Version)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	static FPakPlatformFile* PakPlatformFile = nullptr;

	if (!PakPlatformFile)
	{
		PakPlatformFile = new FPakPlatformFile();
		PakPlatformFile->Initialize(&PlatformFile, TEXT(""));
		FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
	}

	const int32 PakOrder = Version; // Higher overrides lower
	FString MountPoint = ""; // Uses the one baked in the pak
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Mounting Pak= %s , Version(PAkOrder)= %d"), *PakPath, PakOrder);
	return PakPlatformFile->Mount(*PakPath, PakOrder, *MountPoint);
}

bool UPulseAssetManager::UnmountPak(const FString& PakPath)
{
	IPlatformFile* PlatformFile =
		&FPlatformFileManager::Get().GetPlatformFile();

	FPakPlatformFile* PakPlatformFile =
		static_cast<FPakPlatformFile*>(PlatformFile);

	if (!PakPlatformFile || !UPulseSystemLibrary::FileExist(PakPath))
	{
		UE_LOG(LogPulseAssetManagement, Error, TEXT("Unable to Umount Pak= %s"), *PakPath);
		return false;
	}

	UE_LOG(LogPulseAssetManagement, Log, TEXT("UnMounting Pak= %s"), *PakPath);
	return PakPlatformFile->Unmount(*PakPath);
}


FString UPulseAssetManager::GetGameContentSavePath()
{
	return FString::Printf(TEXT("%s/GameContent"), *FPaths::ProjectContentDir());
}

FString UPulseAssetManager::GetDLCSavePath()
{
	return FString::Printf(TEXT("%s/GameContent/DLC"), *FPaths::ProjectContentDir());
}

FString UPulseAssetManager::GetDLCDownloadPath()
{
	return FString::Printf(TEXT("%s/Paks/DLC"), *FPaths::ProjectPersistentDownloadDir());
}

void UPulseAssetManager::OnBundleReadyToMount(TSubclassOf<UPulseContentProvider> Class, const FString& FilePath, bool bSucess)
{
	if (!Class)
		return;
	if (!ContentProviders.Contains(Class))
		return;
	if (FilePath.IsEmpty())
		return;
	if (!bSucess)
		return;
	FPAkMountRequest MountRequest;
	if (!ContentProviders[Class]->GetLocalBundleDescriptor(FilePath, MountRequest.BundleDescriptor))
		return;
	if (MountRequest.BundleDescriptor.FilePath != FilePath)
		return;
	MountRequest.ProviderClass = Class;
	ContentMountRequests.Insert(MountRequest, 0);
}

void UPulseAssetManager::TryToAttributePostLoadedAssetToCurrentBundle(const FAssetData& AssetData)
{
	if (!CurrentMountRequest.ProviderClass)
		return;
	FPAkAssetReferenceKey key;
	key.Class = CurrentMountRequest.ProviderClass;
	key.BundleName = CurrentMountRequest.BundleDescriptor.BundleName;
	if (PakAssetReferences.Contains(key))
	{
		PakAssetReferences[key].AssetReferences.Add(AssetData);
	}
	else
	{
		FPAkAssetReferenceEntry Entry;
		Entry.AssetReferences.Add(AssetData);
		Entry.ContentPaths = CurrentMountRequest.BundleDescriptor.MountScanPaths;
		Entry.ModularGameplayPlugins = CurrentMountRequest.BundleDescriptor.ModularGameplayPlugins;
		PakAssetReferences.Add(key, Entry);
	}
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Discovered Asset %s Belong to PAK %s"), *AssetData.GetFullName(), *CurrentMountRequest.BundleDescriptor.FilePath);
}

void UPulseAssetManager::TryToRemovePostLoadedAssetFromCurrentBundle(const FAssetData& AssetData)
{
	if (!CurrentMountRequest.ProviderClass)
		return;
	FPAkAssetReferenceKey key;
	key.Class = CurrentMountRequest.ProviderClass;
	key.BundleName = CurrentMountRequest.BundleDescriptor.BundleName;
	if (PakAssetReferences.Contains(key) && PakAssetReferences[key].AssetReferences.Contains(AssetData))
	{
		UE_LOG(LogPulseAssetManagement, Log, TEXT("UnDiscovered Asset %s Belong to PAK %s"), *AssetData.GetFullName(), *CurrentMountRequest.BundleDescriptor.FilePath);
		PakAssetReferences[key].AssetReferences.Remove(AssetData);
	}
}

void UPulseAssetManager::OnPostAssetAdded(const FAssetData& AssetData)
{
	TryToAttributePostLoadedAssetToCurrentBundle(AssetData);
	FPrimaryAssetId id = AssetData.GetPrimaryAssetId();
	if (!id.IsValid())
		return;
	AssetCheckList.Add(id);
	if (!AssetData.GetClass())
		return;
	if (!AssetData.GetClass()->IsChildOf(UBasePulseAsset::StaticClass()))
		return;
	if (const auto asset = Cast<UBasePulseAsset>(AssetData.GetAsset()))
	{
		FPulseAssetId assetID;
		assetID.AssetClassFName = AssetData.GetClass()->GetFName();
		assetID.Id = asset->AssetId;
		FPulseAssetsManifestEntry entry;
		entry.PrimaryAssetId = id;
		entry.Version = asset->Version;
		entry.bIsPlaceHolderAsset = asset->bIsPlaceHolder;
#if WITH_EDITOR
		if (AssetsManifest.AssetsRegistry.Contains(assetID))
		{
			// Compare
			if (AssetsManifest.AssetsRegistry[assetID].Compare(entry))
			{
				AssetsManifest.AssetsRegistry[assetID] = entry;
			}
		}
		else
		{
			AssetsManifest.AssetsRegistry.Add(assetID, entry);
		}
#else
		if (DLCAssetsManifest.AssetsRegistry.Contains(assetID))
		{
			// Compare
			if (DLCAssetsManifest.AssetsRegistry[assetID].Compare(entry))
			{
				if (!DLCVersioningEntries.Contains(assetID))
					DLCVersioningEntries.Add(assetID, {});
				DLCVersioningEntries[assetID].EntryPack.Add(DLCAssetsManifest.AssetsRegistry[assetID]);
				DLCAssetsManifest.AssetsRegistry[assetID] = entry;
			}
		}
		else
		{
			DLCAssetsManifest.AssetsRegistry.Add(assetID, entry);
		}
#endif
	}
	UnloadPrimaryAsset(id);
}

void UPulseAssetManager::OnPostAssetRemoved(const FAssetData& AssetData)
{
	TryToRemovePostLoadedAssetFromCurrentBundle(AssetData);
	FPrimaryAssetId id = AssetData.GetPrimaryAssetId();
	if (!id.IsValid())
		return;
	AssetCheckList.Remove(id);
	if (!AssetData.GetClass())
		return;
	if (!AssetData.GetClass()->IsChildOf(UBasePulseAsset::StaticClass()))
		return;
	if (const auto asset = Cast<UBasePulseAsset>(AssetData.GetAsset()))
	{
		FPulseAssetId assetID;
		assetID.AssetClassFName = AssetData.GetClass()->GetFName();
		assetID.Id = asset->AssetId;
#if WITH_EDITOR
		if (AssetsManifest.AssetsRegistry.Contains(assetID))
			AssetsManifest.AssetsRegistry.Remove(assetID);
#else
		if (DLCAssetsManifest.AssetsRegistry.Contains(assetID))
		{
			if (DLCAssetsManifest.AssetsRegistry[assetID].PrimaryAssetId == id)
			{
				if (DLCVersioningEntries.Contains(assetID) && !DLCVersioningEntries[assetID].EntryPack.IsEmpty())
				{
					DLCAssetsManifest.AssetsRegistry[assetID] = DLCVersioningEntries[assetID].EntryPack.Pop();
				}
				else
				{
					DLCAssetsManifest.AssetsRegistry.Remove(assetID);
				}
			}
			if (DLCVersioningEntries.Contains(assetID))
			{
				const int32 index = DLCVersioningEntries[assetID].EntryPack.IndexOfByPredicate([id](const FPulseAssetsManifestEntry& entry)-> bool
				{
					return entry.PrimaryAssetId == id;
				});
				if (index >= 0)
					DLCVersioningEntries[assetID].EntryPack.RemoveAt(index);
			}
		}
#endif
	}
	UnloadPrimaryAsset(id);
}

void UPulseAssetManager::OnPostAssetUpdated(TArrayView<const FAssetData> AssetDatas)
{
	for (const auto& AssetData : AssetDatas)
	{
		TryToAttributePostLoadedAssetToCurrentBundle(AssetData);
		FPrimaryAssetId id = AssetData.GetPrimaryAssetId();
		if (!id.IsValid())
			return;
		AssetCheckList.Add(id);
		if (const auto asset = Cast<UBasePulseAsset>(AssetData.GetAsset()))
		{
			FPulseAssetId assetID;
			assetID.AssetClassFName = AssetData.GetClass()->GetFName();
			assetID.Id = asset->AssetId;
			FPulseAssetsManifestEntry entry;
			entry.PrimaryAssetId = id;
			entry.Version = asset->Version;
			entry.bIsPlaceHolderAsset = asset->bIsPlaceHolder;
#if WITH_EDITOR
			if (AssetsManifest.AssetsRegistry.Contains(assetID))
				AssetsManifest.AssetsRegistry[assetID] = entry;
			else
				AssetsManifest.AssetsRegistry.Add(assetID, entry);
#else
			if (DLCAssetsManifest.AssetsRegistry.Contains(assetID))
			{
				// Compare
				if (DLCAssetsManifest.AssetsRegistry[assetID].Compare(entry))
				{
					if (!DLCVersioningEntries.Contains(assetID))
						DLCVersioningEntries.Add(assetID, {});
					DLCVersioningEntries[assetID].EntryPack.Add(DLCAssetsManifest.AssetsRegistry[assetID]);
					DLCAssetsManifest.AssetsRegistry[assetID] = entry;
				}
			}
			else
			{
				DLCAssetsManifest.AssetsRegistry.Add(assetID, entry);
			}
#endif
		}
	}
}

void UPulseAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Pulse Asset Manager Initialization Started"));

	// Load Additional content providers
	if (const auto config = GetProjectSettings())
	{
		for (int i = 0; i < config->AdditionalContentProviders.Num(); i++)
		{
			if (!config->AdditionalContentProviders[i]->IsChildOf(UPulseContentProvider::StaticClass()))
				continue;
			auto provider = NewObject<UPulseContentProvider>(this, config->AdditionalContentProviders[i]);
			if (!provider)
				continue;
			UE_LOG(LogPulseAssetManagement, Log, TEXT("Asset Provider %s Created"), *provider->GetName());
			provider->OnBundleReadyToMount.AddDynamic(this, &UPulseAssetManager::OnBundleReadyToMount);
			provider->Initialize(config);
			ContentProviders.Add(provider->GetClass(), provider);
		}
	}
	// Mount packages
	MountPendingPakPackages();

	// Fill the asset check set
	TMap<FPrimaryAssetType, TArray<FPrimaryAssetId>> AllAssetsMap;
	TArray<FPrimaryAssetTypeInfo> AssetTypes;
	GetPrimaryAssetTypeInfoList(AssetTypes);
	TArray<FPrimaryAssetId> AssetIds;
	for (const FPrimaryAssetTypeInfo& AssetType : AssetTypes)
	{
		const auto Type = FPrimaryAssetType(AssetType.PrimaryAssetType);
		if (!Type.IsValid())
			continue;
		AssetIds.Reset();
		if (GetPrimaryAssetIdList(Type, AssetIds))
		{
			for (const auto& Id : AssetIds)
			{
				AssetCheckList.Add(Id);
				if (AllAssetsMap.Contains(Type))
					AllAssetsMap[Type].Add(Id);
				else
					AllAssetsMap.Add(Type, {Id});
			}
		}
	}
	// Load manifest
	LoadAssetsManifest();
	if (!AssetsManifest.AssetsRegistry.IsEmpty())
	{
		UE_LOG(LogPulseAssetManagement, Log, TEXT("Asset Registry already exist"));
		// verify manifest
		TArray<FPulseAssetId> Keys;
		const int32 count = AssetsManifest.AssetsRegistry.GetKeys(Keys);
		if (count > 0)
		{
			for (const auto& key : Keys)
			{
				if (!AssetExist(AssetsManifest.AssetsRegistry[key].PrimaryAssetId))
				{
					AssetsManifest.AssetsRegistry.Remove(key);
					UE_LOG(LogPulseAssetManagement, Log, TEXT("Removing pulse Asset %s from asset registry: Asset doesn't exist"), *key.ToString());
					continue;
				}
			}
		}
	}
	else
	{
		// Construct manifest
		UE_LOG(LogPulseAssetManagement, Log, TEXT("Construct new Asset Registry"));
		for (const auto& pair : AllAssetsMap)
		{
			FPrimaryAssetTypeInfo TypeInfos;
			if (!GetPrimaryAssetTypeInfo(pair.Key, TypeInfos))
				continue;
			bool isValid = false;
			bool hasLoadedClass = false;
			TypeInfos.FillRuntimeData(isValid, hasLoadedClass);
			if (!hasLoadedClass)
				continue;
			if (!TypeInfos.GetAssetBaseClass().Get())
				continue;
			if (!TypeInfos.GetAssetBaseClass().Get()->IsChildOf(UBasePulseAsset::StaticClass()))
				continue;
			for (int i = 0; i < pair.Value.Num(); ++i)
			{
				FPrimaryAssetId assetId = pair.Value[i];
				if (!assetId.IsValid())
					continue;
				FStreamableDelegate streamableDelegate;
				streamableDelegate.BindLambda([this, assetId]()
				{
					auto Loaded = GetPrimaryAssetObject(assetId);
					if (const auto asset = Cast<UBasePulseAsset>(Loaded))
					{
						FPulseAssetId _assetID;
						_assetID.AssetClassFName = Loaded->GetClass()->GetFName();
						_assetID.Id = asset->AssetId;
						FPulseAssetsManifestEntry entry;
						entry.PrimaryAssetId = assetId;
						entry.Version = asset->Version;
						entry.bIsPlaceHolderAsset = asset->bIsPlaceHolder;
						UE_LOG(LogPulseAssetManagement, Log, TEXT("Adding pulse Asset %s to asset registry"), *_assetID.ToString());
						AssetsManifest.AssetsRegistry.Add(_assetID, entry);
					}
					if (Loaded)
					{
						UnloadPrimaryAsset(assetId);
					}
				});
				LoadPrimaryAsset(assetId, {}, streamableDelegate, FStreamableManager::AsyncLoadHighPriority);
			}
		}
	}
	// Save manifest
	SaveAssetsManifest();

	// Register for asset registry changes
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().OnAssetAdded().AddUObject(this, &UPulseAssetManager::OnPostAssetAdded);
	ARM.Get().OnAssetRemoved().AddUObject(this, &UPulseAssetManager::OnPostAssetRemoved);
	ARM.Get().OnAssetsUpdated().AddUObject(this, &UPulseAssetManager::OnPostAssetUpdated);
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Pulse Asset Manager Initialization Completed"));
}

bool UPulseAssetManager::AssetExist(const FPrimaryAssetId Id) const
{
	if (AssetCheckList.Contains(Id))
		return true;
	FAssetData data;
	return GetPrimaryAssetData(Id, data);
}

void UPulseAssetManager::LoadAssetsManifest()
{
	FString manifestJson = "";
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Loading Local Asset manifest"));
	if (!UPulseSystemLibrary::LoadJsonFromLocal("LocalAssetManifest", manifestJson))
	{
		UE_LOG(LogPulseAssetManagement, Warning, TEXT("Unable to Load Local Asset Manifest file. Fallback to Native one"));
		if (const auto projectSettings = GetProjectSettings())
		{
			auto nativeManifest = FSoftObjectPtr(projectSettings->LocalAssetManifest).Get();
			if (!nativeManifest)
				return;
			if (const auto bakedLocalManifest = Cast<UPulseLocalBakedAssetManifest>(nativeManifest))
			{
				AssetsManifest = bakedLocalManifest->AssetsManifest;
				return;
			}
		}
	}
	if (manifestJson.IsEmpty())
	{
		UE_LOG(LogPulseAssetManagement, Error, TEXT("Unable to Load Local Asset Manifest. manifest Json is empty"));
		return;
	}
	if (!UPulseSystemLibrary::JsonStringToUStruct(manifestJson, AssetsManifest))
	{
		UE_LOG(LogPulseAssetManagement, Error, TEXT("Unable to Load Local Asset Manifest. manifest types mismatch. Json: %s"), *manifestJson);
	}
}

void UPulseAssetManager::SaveAssetsManifest() const
{
#if !WITH_EDITOR
	FString manifestJson = UPulseSystemLibrary::UStructToJsonString(AssetsManifest);
	if (manifestJson.IsEmpty())
		return;
	UPulseSystemLibrary::SaveJsonToLocal("LocalAssetManifest", manifestJson);
#endif
}

bool UPulseAssetManager::QueryPrimaryAssetID(const FName& ClassFName, const int32& Id, FPrimaryAssetId& OutAssetID, bool bIncludePlaceholders)
{
	FPulseAssetId assetID;
	assetID.AssetClassFName = ClassFName;
	assetID.Id = Id;
	// Look in the Additional content
	if (DLCAssetsManifest.AssetsRegistry.Contains(assetID))
	{
		const auto entry = DLCAssetsManifest.AssetsRegistry[assetID];
		if (((entry.bIsPlaceHolderAsset && bIncludePlaceholders) || !entry.bIsPlaceHolderAsset) && entry.PrimaryAssetId.IsValid())
		{
			OutAssetID = entry.PrimaryAssetId;
			return true;
		}
	}
	// Look in the Native content
	if (AssetsManifest.AssetsRegistry.Contains(assetID))
	{
		const auto entry = AssetsManifest.AssetsRegistry[assetID];
		if (((entry.bIsPlaceHolderAsset && bIncludePlaceholders) || !entry.bIsPlaceHolderAsset) && entry.PrimaryAssetId.IsValid())
		{
			OutAssetID = entry.PrimaryAssetId;
			return true;
		}
	}
	return false;
}

bool UPulseAssetManager::QueryPulseAssetID(const FPrimaryAssetId& PrimaryId, FPulseAssetId& OutAssetID, bool bIncludePlaceholders)
{
	// Look in the Additional content
	for (const auto& pair : DLCAssetsManifest.AssetsRegistry)
	{
		if (pair.Value.bIsPlaceHolderAsset && !bIncludePlaceholders)
			continue;
		if (pair.Value.PrimaryAssetId != PrimaryId)
			continue;
		OutAssetID = pair.Key;
		return true;
	}
	// Look in the native content
	for (const auto& pair : AssetsManifest.AssetsRegistry)
	{
		if (pair.Value.bIsPlaceHolderAsset && !bIncludePlaceholders)
			continue;
		if (pair.Value.PrimaryAssetId != PrimaryId)
			continue;
		OutAssetID = pair.Key;
		return true;
	}
	return false;
}

void UPulseAssetManager::MountPendingPakPackages()
{
	if (ContentMountRequests.IsEmpty())
	{
		UE_LOG(LogPulseAssetManagement, Warning, TEXT("Unable to Mount Pending Pak Packages: No content mount request"));
		return;
	}
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Starting to Mount Pending Pak Packages"));
	FAssetRegistryModule& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	while (!ContentMountRequests.IsEmpty())
	{
		CurrentMountRequest = ContentMountRequests.Pop();
		if (!ContentProviders.Contains(CurrentMountRequest.ProviderClass))
			return;
		if (MountPak(CurrentMountRequest.BundleDescriptor.FilePath, CurrentMountRequest.BundleDescriptor.Version))
		{
			AssetRegistry.Get().ScanPathsSynchronous(CurrentMountRequest.BundleDescriptor.MountScanPaths, true);
			UE_LOG(LogPulseAssetManagement, Log, TEXT("Successfully Mounted Pak Package: Filepath= %s, Version= %d"), *CurrentMountRequest.BundleDescriptor.FilePath,
			       CurrentMountRequest.BundleDescriptor.Version);
			// Enable modular game play features
			FString PluginURL;
			for (const auto& featureName : CurrentMountRequest.BundleDescriptor.ModularGameplayPlugins)
			{
				// Get the Plugin URL based on the Game Features Name
				if (UGameFeaturesSubsystem::Get().GetPluginURLByName(featureName, PluginURL))
				{
					UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(PluginURL, FGameFeaturePluginLoadComplete());
				}
			}
			// notify the bundle is mounted
			ContentProviders[CurrentMountRequest.ProviderClass]->OnBundleMounted(CurrentMountRequest.BundleDescriptor.FilePath);
		}
	}
	CurrentMountRequest = {};
}

void UPulseAssetManager::UnMountContentPackage(const FString& BundleName, bool bDeleteBundle)
{
	for (const auto& pair : ContentProviders)
	{
		FPAkAssetReferenceKey key;
		key.Class = pair.Key;
		key.BundleName = BundleName;
		if (!PakAssetReferences.Contains(key))
			continue;
		FString bundlePath = "";
		if (!pair.Value->DoBundleExistLocally(bundlePath))
			continue;
		if (bDeleteBundle && !pair.Value->DeleteBundle(BundleName))
			continue;
		//Unload assets
		for (const auto& Asset : PakAssetReferences[key].AssetReferences)
		{
			if (!Asset.IsAssetLoaded())
				continue;
			FPrimaryAssetId AssetId = Asset.GetPrimaryAssetId();
			if (AssetId.IsValid())
				UnloadPrimaryAsset(AssetId);
			else
				GetStreamableManager().Unload(Asset.GetSoftObjectPath());
		}
		//FlushAsyncLoading
		FlushAsyncLoading();

		//Remove AssetRegistry paths
		FAssetRegistryModule& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		for (const auto& path : PakAssetReferences[key].ContentPaths)
		{
			AssetRegistry.Get().RemovePath(path);
		}

		// Disable modular game play features
		FString PluginURL;
		for (const auto& featureName : PakAssetReferences[key].ModularGameplayPlugins)
		{
			// Get the Plugin URL based on the Game Features Name
			if (UGameFeaturesSubsystem::Get().GetPluginURLByName(featureName, PluginURL))
			{
				UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(PluginURL);
				UGameFeaturesSubsystem::Get().UnloadGameFeaturePlugin(PluginURL);
			}
		}

		//Unmount pak
		UnmountPak(bundlePath);
	}
}

void UPulseAssetManager::AsyncLoadPulseAssets(const TSubclassOf<UBasePulseAsset> Class, const TArray<int32>& Ids, const TArray<FName>& LoadBundles,
                                              TFunction<void(TArray<UPrimaryDataAsset*>&)> OnSuccess, TFunction<void()> OnFailed, bool bIncludePlaceholderAssets)
{
	if (!Class)
	{
		if (OnFailed != nullptr)
			OnFailed();
		return;
	}
	if (Ids.IsEmpty())
	{
		if (OnFailed != nullptr)
			OnFailed();
		return;
	}
	const FName className = Class->GetFName();
	TMap<FPrimaryAssetId, TArray<int32>> IndexMap;
	TMap<FPrimaryAssetId, TObjectPtr<UBasePulseAsset>> AssetMap;
	// Fill assets ids
	for (int i = 0; i < Ids.Num(); ++i)
	{
		FPrimaryAssetId assetID;
		if (QueryPrimaryAssetID(className, Ids[i], assetID, bIncludePlaceholderAssets))
		{
			if (!IndexMap.Contains(assetID))
				IndexMap.Add(assetID, {i});
			else
				IndexMap[assetID].Add(i);
			if (!AssetMap.Contains(assetID))
				AssetMap.Add(assetID, nullptr);
		}
	}
	// Prepare delegate
	FStreamableDelegate streamableDelegate;
	streamableDelegate.BindLambda([this, IndexMap, AssetMap, Ids, OnSuccess, OnFailed]()-> void
	{
		TArray<UPrimaryDataAsset*> _assets;
		UPulseSystemLibrary::ArrayMatchSize(_assets, Ids.Num());
		TArray<FPrimaryAssetId> _assetIds;
		AssetMap.GetKeys(_assetIds);
		bool atLeastOne = false;
		for (const auto& assetID : _assetIds)
		{
			if (auto asset = Cast<UPrimaryDataAsset>(GetPrimaryAssetObject(assetID)))
			{
				if (IndexMap.Contains(assetID))
				{
					atLeastOne = true;
					for (const auto& index : IndexMap[assetID])
						_assets[index] = asset;
				}
			}
		}
		if (!atLeastOne)
		{
			if (OnFailed != nullptr)
				OnFailed();
			return;
		}
		if (OnSuccess)
			OnSuccess(_assets);
	});

	// Load assets
	TArray<FPrimaryAssetId> assetIds;
	AssetMap.GetKeys(assetIds);
	LoadPrimaryAssets(assetIds, LoadBundles, streamableDelegate);
}

void UPulseAssetManager::BeginDestroy()
{
	SaveAssetsManifest();
	Super::BeginDestroy();
}


TArray<FName> UPulseBPAssetManager::GetDataBundleFromFlags(int32 flag)
{
	TArray<FName> result;
	if (flag & static_cast<int32>(EPulseDataBundleType::Infos))
		result.Add(BUNDLE_INFOS);
	if (flag & static_cast<int32>(EPulseDataBundleType::UI))
		result.Add(BUNDLE_UI);
	if (flag & static_cast<int32>(EPulseDataBundleType::Spawn))
		result.Add(BUNDLE_SPAWN);
	if (flag & static_cast<int32>(EPulseDataBundleType::Accessories))
		result.Add(BUNDLE_ACCESSORY);
	return result;
}

bool UPulseBPAssetManager::TryGetObjectAssetID(UObject* Object, int32& OutID)
{
	if (!Object)
		return false;
	auto AssetManager = UPulseAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	auto assetID = AssetManager->GetPrimaryAssetIdForObject(Object);
	if (!assetID.IsValid())
		return false;
	TSoftClassPtr<> ptr;
	FName name;
	return TryGetPrimaryAssetInfos(assetID, ptr, name, OutID);
}

bool UPulseBPAssetManager::TryGetPrimaryAssetType(const TSubclassOf<UBasePulseAsset> Type, FPrimaryAssetType& OutAssetType)
{
	if (!Type)
		return false;
	auto AssetManager = UPulseAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	FPrimaryAssetType type = FPrimaryAssetType(Type->GetFName());
	FPrimaryAssetTypeInfo typeInfos;
	if (!AssetManager->GetPrimaryAssetTypeInfo(type, typeInfos))
		return false;
	if (typeInfos.GetAssetBaseClass() != TSoftClassPtr(Type))
		return false;
	OutAssetType = type;
	return true;
}

bool UPulseBPAssetManager::TryGetClassAssetType(const FPrimaryAssetType& AssetType, TSoftClassPtr<UObject>& OutClass)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	FPrimaryAssetTypeInfo typeInfos;
	if (!AssetManager->GetPrimaryAssetTypeInfo(AssetType, typeInfos))
		return false;
	OutClass = typeInfos.GetAssetBaseClass();
	return true;
}

bool UPulseBPAssetManager::TryGetPrimaryAssetID(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FPrimaryAssetId& OutAssetID)
{
	if (!Type)
		return false;
	auto AssetManager = UPulseAssetManager::Get();
	if (!AssetManager)
		return false;
	return AssetManager->QueryPrimaryAssetID(Type->GetFName(), Id, OutAssetID, true);
}

bool UPulseBPAssetManager::TryGetPrimaryAssetInfos(const FPrimaryAssetId& AssetID, TSoftClassPtr<UObject>& OutClass, FName& OutAssetName, int32& OutAssetId)
{
	if (!AssetID.IsValid())
		return false;
	auto AssetManager = UPulseAssetManager::Get();
	if (!AssetManager)
		return false;
	FPulseAssetId PulseAssetID = {};
	if (!AssetManager->QueryPulseAssetID(AssetID, PulseAssetID, true))
		return false;
	FAssetData AssetData;
	if (!AssetManager->GetPrimaryAssetData(AssetID, AssetData))
		return false;
	OutClass = AssetData.GetClass(EResolveClass::Yes);
	OutAssetName = AssetData.AssetName;
	OutAssetId = PulseAssetID.Id;
	return true;
}

TArray<FPrimaryAssetId> UPulseBPAssetManager::GetAllAssetsOfType(const TSubclassOf<UObject> Type)
{
	if (!Type)
		return {};
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> assets;
		FPrimaryAssetType type = FPrimaryAssetType(Type->GetFName());
		mgr->GetPrimaryAssetIdList(type, assets);
		return assets;
	}
	return {};
}

bool UPulseBPAssetManager::LoadPulseAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FOnAssetLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UPulseAssetManager::Get();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(nullptr);
		return false;
	}
	mgr->AsyncLoadPulseAssets(Type, {Id}, Bundles, [CallBack](TArray<UPrimaryDataAsset*>& Result)-> void
	                          {
		                          //On success
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(!Result.IsEmpty() ? Result[0] : nullptr);
	                          }, [CallBack]()-> void
	                          {
		                          // On failure
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(nullptr);
	                          });
	return true;
}

bool UPulseBPAssetManager::LoadAllPulseAssets(const TSubclassOf<UBasePulseAsset> Type, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UAssetManager::GetIfInitialized();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	auto allAssets = GetAllAssetsOfType(Type);
	if (allAssets.IsEmpty())
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	auto strCallBack = FStreamableDelegate::CreateLambda([CallBack, allAssets]()-> void
	{
		if (auto mgr = UAssetManager::GetIfInitialized())
		{
			TArray<UPrimaryDataAsset*> assets;
			for (const auto& assetId : allAssets)
			{
				if (auto assetData = mgr->GetPrimaryAssetObject<UBasePulseAsset>(assetId))
					assets.Insert(assetData, 0);
			}
			if (CallBack.IsBound())
				CallBack.Broadcast(FAssetPack(assets));
			return;
		}
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
	});
	mgr->LoadPrimaryAssets(allAssets, Bundles, strCallBack);
	return true;
}

bool UPulseBPAssetManager::LoadMultipleAssets(const TSubclassOf<UBasePulseAsset> Type, TArray<int32> Ids, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UPulseAssetManager::Get();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	mgr->AsyncLoadPulseAssets(Type, Ids, Bundles, [CallBack](TArray<UPrimaryDataAsset*>& Result)-> void
	                          {
		                          //On success
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(FAssetPack(Result));
	                          }, [CallBack]()-> void
	                          {
		                          // On failure
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(FAssetPack());
	                          });
	return true;
}


void UPulseBPAssetManager::TryUnloadGameAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id)
{
	FPrimaryAssetId assetID;
	if (!TryGetPrimaryAssetID(Type, Id, assetID))
		return;
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		mgr->UnloadPrimaryAsset(assetID);
	}
}

void UPulseBPAssetManager::TryUnloadAllGameAssets(const TSubclassOf<UBasePulseAsset> Type)
{
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		FPrimaryAssetType type;
		if (!TryGetPrimaryAssetType(Type, type))
			return;
		mgr->UnloadPrimaryAssetsWithType(type);
	}
}

bool UPulseBPAssetManager::TryGetControlledCharacterID(const AController* Controller, FPrimaryAssetId& OutCharacterID)
{
	if (!Controller)
		return false;
	auto pawn = Controller->GetPawn();
	if (!pawn)
		return false;
	if (!pawn->Implements<UIIdentifiableActor>())
		return false;
	OutCharacterID = IIIdentifiableActor::Execute_GetID(pawn);
	return true;
}
