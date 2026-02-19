// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/AssetManager/PulseContentProvider.h"

#include "PulseGameFramework.h"
#include "Core/PulseSystemLibrary.h"


void UPulseContentProvider::OnManifestDownloaded_Internal(TSubclassOf<UPulseContentProvider> Provider, const FString& FilePath)
{
	if (GetClass() != Provider)
		return;
	LockManifests--;
	if (LockManifests > 0)
	{
		UE_LOG(LogPulseAssetManagement, Warning,
		       TEXT("[Fetch Remote Manifest] - Manifest locked (%d): Unable to load the remote manifest '%s'. Provider: %s"), LockManifests, *FilePath, *GetClass()->GetName());
		PendingRemoteManifestLoad = FilePath;
		OnContentManifestDownloadCompleted.Broadcast(Provider, FilePath, false);
		return;
	}
	FString json;
	if (!UPulseSystemLibrary::LoadJsonFromLocalPath(FilePath, json))
	{
		OnContentManifestDownloadCompleted.Broadcast(Provider, FilePath, false);
		return;
	}
	if (!UPulseSystemLibrary::JsonStringToUStruct(json, UpToDateManifest))
	{
		OnContentManifestDownloadCompleted.Broadcast(Provider, FilePath, false);
		return;
	}
	// Set the descriptor status
	for (int i = UpToDateManifest.Bundles.Num() - 1; i >= 0; i--)
	{
		if (LocalBundlesMap.Contains(UpToDateManifest.Bundles[i].BundleName))
		{
			if (!UpToDateManifest.Bundles[i].CanOverride(LocalBundlesMap[UpToDateManifest.Bundles[i].BundleName]))
			{
				UpToDateManifest.Bundles.RemoveAt(i);
				continue;
			}
			UpToDateManifest.Bundles[i].BundleState = EPulseContentBundleQueryState::UpdateAvailable;
		}
		UpToDateManifest.Bundles[i].BundleState = EPulseContentBundleQueryState::ReadyToDownload;
	}

	OnContentManifestDownloadCompleted.Broadcast(Provider, FilePath, true);
	const int32 updateCount = GetBundleAvailableUpdatesCount();
	if (updateCount > 0)
	{
		OnBundleUpdateAvailable.Broadcast(Provider, updateCount);
	}
}

void UPulseContentProvider::OnBundleDownloaded_Internal(TSubclassOf<UPulseContentProvider> Provider, const FString& BundleName, const FString& FilePath)
{
	if (GetClass() != Provider)
		return;
	if (BundleName.IsEmpty())
	{
		UE_LOG(LogPulseAssetManagement, Error, TEXT("[Bundle Download] - Empty bundle Name for file: %s. Provider: %s"), *FilePath, *GetClass()->GetName());
		return;
	}
	if (!UPulseSystemLibrary::FileExist(FilePath))
	{
		UE_LOG(LogPulseAssetManagement, Error, TEXT("[Bundle Download] - Bundle File '%s' does not exist. Provider: %s"), *FilePath, *GetClass()->GetName());
		OnBundleDownloadCompleted.Broadcast(Provider, FilePath, false);
		return;
	}
	OnBundleDownloadCompleted.Broadcast(Provider, FilePath, true);

	// Get the bundle descriptor
	FPulseBundleDescriptor descriptor;
	LockManifests++;
	const int32 updateIndex = UpToDateManifest.Bundles.IndexOfByPredicate([BundleName](const FPulseBundleDescriptor& bDesc)-> bool { return bDesc.BundleName.Equals(BundleName); });
	if (updateIndex == INDEX_NONE)
	{
		if (!LocalBundlesMap.Contains(BundleName))
		{
			UE_LOG(LogPulseAssetManagement, Error, TEXT("[Bundle Download] - Unknown Bundle: Name(%s), Path(%s). Provider: %s"), *BundleName, *FilePath, *GetClass()->GetName());
			UPulseSystemLibrary::FileDelete(FilePath);
			LockManifests--;
			return;
		}
		descriptor = LocalBundlesMap[BundleName];
	}
	else
	{
		descriptor = UpToDateManifest.Bundles[updateIndex];
	}

	// Check bundle hash
	if (descriptor.BundleState != EPulseContentBundleQueryState::Downloading)
	{
		UE_LOG(LogPulseAssetManagement, Warning, TEXT("[Bundle Download] - unexpected bundle descriptor state: State(%s). Provider: %s"),
		       *UEnum::GetValueAsString(descriptor.BundleState), *GetClass()->GetName());
	}
	const FString ExpectedMD5 = descriptor.Hash;
	UPulseSystemLibrary::
		FileComputeMD5Async(FilePath, FOnMD5Computed::CreateLambda(
			                    [this, ExpectedMD5, FilePath, updateIndex](bool bSuccess, const FString& MD5)
			                    {
				                    if (!bSuccess || !MD5.Equals(ExpectedMD5, ESearchCase::IgnoreCase))
				                    {
					                    UE_LOG(LogPulseAssetManagement, Error, TEXT("[Bundle Download] - MD5 mismatch (Got %s; Expected %s), rejecting pak '%s'. Provider: %s"),
					                           *MD5, *ExpectedMD5, *FilePath, *GetClass()->GetName());
					                    UPulseSystemLibrary::FileDelete(FilePath);
					                    LockManifests--;
					                    return;
				                    }
				                    const int32 hashIndex = UpToDateManifest.Bundles.IndexOfByPredicate([ExpectedMD5](const FPulseBundleDescriptor& bDesc)-> bool
				                    {
					                    return bDesc.Hash.Equals(ExpectedMD5);
				                    });
				                    if (hashIndex != INDEX_NONE)
				                    {
					                    // Update the bundle descriptor
					                    auto bundleDescriptor = UpToDateManifest.Bundles[updateIndex];
					                    bundleDescriptor.FilePath = FilePath;
					                    bundleDescriptor.BundleState = EPulseContentBundleQueryState::AvailableLocally;
					                    bundleDescriptor.DownloadHandleUID = {};
					                    if (UPulseSystemLibrary::MapAddOrUpdateValue(LocalBundlesMap, bundleDescriptor.BundleName, bundleDescriptor))
					                    {
						                    UpToDateManifest.Bundles.RemoveAt(hashIndex);
						                    const int32 updateCount = GetBundleAvailableUpdatesCount();
						                    if (updateCount > 0)
						                    {
							                    OnBundleUpdateAvailable.Broadcast(GetClass(), updateCount);
						                    }
					                    }
				                    }
				                    else
				                    {
					                    TArray<FString> bundleNames;
					                    LocalBundlesMap.GetKeys(bundleNames);
					                    bool bFound = false;
					                    for (const auto& bundleName : bundleNames)
					                    {
						                    if (LocalBundlesMap[bundleName].Hash.Equals(ExpectedMD5))
						                    {
							                    bFound = true;
							                    LocalBundlesMap[bundleName].FilePath = FilePath;
							                    LocalBundlesMap[bundleName].BundleState = EPulseContentBundleQueryState::AvailableLocally;
							                    LocalBundlesMap[bundleName].DownloadHandleUID = {};
							                    break;
						                    }
					                    }
					                    if (!bFound)
					                    {
						                    UE_LOG(LogPulseAssetManagement, Error,
						                           TEXT("[Bundle Download] - No Bundle descriptor was awaiting the bundle file '%s'. Provider: %s"), *FilePath,
						                           *GetClass()->GetName());
						                    UPulseSystemLibrary::FileDelete(FilePath);
						                    LockManifests--;
						                    return;
					                    }
				                    }
				                    SaveLocalManifest();
				                    LockManifests--;
				                    OnBundleReadyToMount.Broadcast(this->GetClass(), FilePath, true);
			                    }), BundleFileMd5StreamBufferSize);
}

void UPulseContentProvider::EndManifestDownload(const FString& InStorageFilePath)
{
	OnManifestDownloaded_Internal(GetClass(), InStorageFilePath);
}

void UPulseContentProvider::EndBundleDownload(const FString& BundleName, const FString& InStorageFilePath)
{
	OnBundleDownloaded_Internal(GetClass(), BundleName, InStorageFilePath);
	const int32 index = UpToDateManifest.Bundles.IndexOfByPredicate([BundleName](const FPulseBundleDescriptor& Descriptor)-> bool
	{
		return Descriptor.BundleName == BundleName;
	});
	if (index != INDEX_NONE)
	{
		UpToDateManifest.Bundles[index].BundleState = _beforeFetchBundleStates.Contains(BundleName)
			                                              ? _beforeFetchBundleStates[BundleName]
			                                              : EPulseContentBundleQueryState::ReadyToDownload;
	}
	if (_beforeFetchBundleStates.Contains(BundleName))
		_beforeFetchBundleStates.Remove(BundleName);
}

void UPulseContentProvider::OnBundleDownloadCancelled(const FGuid& DownloadUID)
{
	const int32 index = UpToDateManifest.Bundles.IndexOfByPredicate([DownloadUID](const FPulseBundleDescriptor& Descriptor)-> bool
	{
		return Descriptor.DownloadHandleUID == DownloadUID;
	});
	if (index != INDEX_NONE)
	{
		const FString BundleName = UpToDateManifest.Bundles[index].BundleName;
		UpToDateManifest.Bundles[index].BundleState = _beforeFetchBundleStates.Contains(BundleName)
			                                              ? _beforeFetchBundleStates[BundleName]
			                                              : EPulseContentBundleQueryState::ReadyToDownload;
		UpToDateManifest.Bundles[index].DownloadHandleUID = {};
		if (_beforeFetchBundleStates.Contains(BundleName))
			_beforeFetchBundleStates.Remove(BundleName);
	}
}

bool UPulseContentProvider::SetBundleDownloadUID(const FString& BundleUrl, const FGuid& DownloadUID)
{
	const int32 index = UpToDateManifest.Bundles.IndexOfByPredicate([BundleUrl](const FPulseBundleDescriptor& Descriptor)-> bool
	{
		return Descriptor.Url == BundleUrl;
	});
	if (index == INDEX_NONE)
		return false;
	if (UpToDateManifest.Bundles[index].DownloadHandleUID.IsValid())
		return false;
	UpToDateManifest.Bundles[index].DownloadHandleUID = DownloadUID;
	return true;
}


void UPulseContentProvider::Initialize(const UCoreProjectSetting* projectSettings)
{
	if (projectSettings)
	{
		BundleFileMd5StreamBufferSize = FMath::FloorToInt(1024 * 1204 * projectSettings->Md5CheckBufferSize);
		ManifestVersion = projectSettings->ManifestTypeVersion;
	}
	LoadLocalManifest();
	BeginFetchRemoteManifest();
}

bool UPulseContentProvider::LoadLocalManifest()
{
	if (LockManifests > 0)
		return false;
	FPulseProviderContentManifest localManifest = {};
	FString localManifestJson;
	FString fileName = FString::Printf(TEXT("%s_ContentManifest.json"), *GetClass()->GetFName().ToString());
	if (!UPulseSystemLibrary::LoadJsonFromLocal(fileName, localManifestJson))
		return false;
	if (!UPulseSystemLibrary::JsonStringToUStruct(localManifestJson, localManifest))
		return false;
	for (int i = 0; i < localManifest.Bundles.Num(); i++)
	{
		if (!UPulseSystemLibrary::FileExist(localManifest.Bundles[i].FilePath))
			continue;
		localManifest.Bundles[i].BundleState = EPulseContentBundleQueryState::AvailableLocally;
		localManifest.Bundles[i].DownloadHandleUID = {};
		UPulseSystemLibrary::MapAddOrUpdateValue(LocalBundlesMap, localManifest.Bundles[i].BundleName, localManifest.Bundles[i]);
		OnBundleReadyToMount.Broadcast(this->GetClass(), localManifest.Bundles[i].FilePath, true);
	}
	return true;
}

bool UPulseContentProvider::SaveLocalManifest()
{
	FPulseProviderContentManifest localManifest = {};
	localManifest.Version = ManifestVersion;
	FString fileName = FString::Printf(TEXT("%s_ContentManifest.json"), *GetClass()->GetFName().ToString());
	for (const TPair<FString, FPulseBundleDescriptor>& Pair : LocalBundlesMap)
		localManifest.Bundles.Add(Pair.Value);
	const FString json = UPulseSystemLibrary::UStructToJsonString(localManifest);
	if (json.IsEmpty())
		return false;
	return UPulseSystemLibrary::SaveJsonToLocal(fileName, json);
}


int UPulseContentProvider::GetBundleAvailableUpdatesCount() const
{
	TArray<FPulseBundleDescriptor> BundleDescriptors;
	return GetBundleAvailableUpdates(BundleDescriptors);
}

int UPulseContentProvider::GetBundleAvailableUpdates(TArray<FPulseBundleDescriptor>& OutBundleDescriptors) const
{
	int count = 0;
	for (const auto& descriptor : UpToDateManifest.Bundles)
	{
		if (LocalBundlesMap.Contains(descriptor.BundleName) && LocalBundlesMap[descriptor.BundleName].Version >= descriptor.Version)
			continue;
		OutBundleDescriptors.Add(descriptor);
		count++;
	}
	return count;
}

void UPulseContentProvider::UpdateBundles()
{
	TArray<FPulseBundleDescriptor> BundleDescriptors;
	if (GetBundleAvailableUpdates(BundleDescriptors) <= 0)
		return;
	for (const auto& descriptor : BundleDescriptors)
		BeginFetchRemoteBundle(descriptor.BundleName);
}

bool UPulseContentProvider::DoBundleExistLocally(const FString& BundleName) const
{
	if (!LocalBundlesMap.Contains(BundleName))
		return false;
	return LocalBundlesMap[BundleName].BundleState >= EPulseContentBundleQueryState::AvailableLocally;
}

bool UPulseContentProvider::GetLocalBundleDescriptor(const FString& BundleLocalPath, FPulseBundleDescriptor& OutDescriptor) const
{
	TArray<FString> bundleNames;
	LocalBundlesMap.GetKeys(bundleNames);
	for (const auto& bundleName : bundleNames)
	{
		if (LocalBundlesMap[bundleName].FilePath.Equals(BundleLocalPath))
		{
			OutDescriptor = LocalBundlesMap[bundleName];
			return true;
		}
	}
	return false;
}

EPulseContentBundleQueryState UPulseContentProvider::GetBundleState(const FString& BundleName)
{
	const int32 index = UpToDateManifest.Bundles.IndexOfByPredicate([BundleName](const FPulseBundleDescriptor& Descriptor)-> bool
	{
		return Descriptor.BundleName == BundleName;
	});
	if (index != INDEX_NONE)
	{
		return UpToDateManifest.Bundles[index].BundleState;
	}
	else if (LocalBundlesMap.Contains(BundleName))
	{
		return LocalBundlesMap[BundleName].BundleState;
	}
	return EPulseContentBundleQueryState::NotFound;
}

bool UPulseContentProvider::QueryBundleToMount(const FString& BundleName, FString& OutBundlePath, EPulseContentBundleQueryResponse& OutQueryResponse)
{
	FPulseBundleDescriptor BundleDescriptor;
	const int32 index = UpToDateManifest.Bundles.IndexOfByPredicate([BundleName](const FPulseBundleDescriptor& Descriptor)-> bool
	{
		return Descriptor.BundleName == BundleName;
	});
	if (index != INDEX_NONE)
	{
		BundleDescriptor = UpToDateManifest.Bundles[index];
	}
	else if (LocalBundlesMap.Contains(BundleName))
	{
		BundleDescriptor = LocalBundlesMap[BundleName];
	}

	switch (BundleDescriptor.BundleState)
	{
	default:
		{
			OutQueryResponse = EPulseContentBundleQueryResponse::NotFound;
			return false;
		}
	case EPulseContentBundleQueryState::UpdateAvailable:
		{
			OutQueryResponse = EPulseContentBundleQueryResponse::AvailableRemotely;
			if (LocalBundlesMap.Contains(BundleName))
			{
				OutBundlePath = LocalBundlesMap[BundleName].FilePath;
				return true;
			}
			return false;
		}
	case EPulseContentBundleQueryState::ReadyToDownload:
		{
			OutQueryResponse = EPulseContentBundleQueryResponse::AvailableRemotely;
			return false;
		}
	case EPulseContentBundleQueryState::Downloading:
		{
			OutQueryResponse = EPulseContentBundleQueryResponse::AvailableRemotely;
			return false;
		}
	case EPulseContentBundleQueryState::AvailableLocally:
		{
			OutBundlePath = BundleDescriptor.FilePath;
			OutQueryResponse = EPulseContentBundleQueryResponse::AvailableLocally;
			return UPulseSystemLibrary::FileExist(OutBundlePath);
		}
	case EPulseContentBundleQueryState::AlreadyMounted:
		{
			OutQueryResponse = EPulseContentBundleQueryResponse::AlreadyMounted;
			return false;
		}
	}
}

bool UPulseContentProvider::DeleteBundle(const FString& BundleName)
{
	if (!LocalBundlesMap.Contains(BundleName))
		return false;
	FPulseBundleDescriptor BundleDescriptor = LocalBundlesMap[BundleName];
	if (UPulseSystemLibrary::FileDelete(BundleDescriptor.FilePath))
	{
		LocalBundlesMap.Remove(BundleName);
		return true;
	}
	return false;
}

void UPulseContentProvider::BeginFetchRemoteManifest()
{
	if (LockManifests > 0)
		return;
	LockManifests++;
	FetchRemoteManifest();
}

void UPulseContentProvider::BeginFetchRemoteBundle(const FString& BundleName)
{
	const int32 index = UpToDateManifest.Bundles.IndexOfByPredicate([BundleName](const FPulseBundleDescriptor& Descriptor)-> bool
	{
		return Descriptor.BundleName == BundleName;
	});
	if (index == INDEX_NONE)
	{
		UE_LOG(LogPulseAssetManagement, Error,
		       TEXT("[Fetch Remote Bundle] - No descriptor for the bundle '%s' to download or update. Provider: %s"), *BundleName, *GetClass()->GetName());
		return;
	}
	// Check if the download is already ongoing
	if (UpToDateManifest.Bundles[index].BundleState == EPulseContentBundleQueryState::Downloading)
	{
		UE_LOG(LogPulseAssetManagement, Error,
		       TEXT("[Fetch Remote Bundle] - The bundle '%s' is already downloading. Provider: %s"), *BundleName, *GetClass()->GetName());
		return;
	}
	UPulseSystemLibrary::MapAddOrUpdateValue(_beforeFetchBundleStates, BundleName, UpToDateManifest.Bundles[index].BundleState);
	UpToDateManifest.Bundles[index].BundleState = EPulseContentBundleQueryState::Downloading;
	FetchRemoteBundle(UpToDateManifest.Bundles[index]);
}

void UPulseContentProvider::OnBundleMounted(const FString& BundleFilePath)
{
	TArray<FString> bundleNames;
	LocalBundlesMap.GetKeys(bundleNames);
	bool bFound = false;
	for (const auto& bundleName : bundleNames)
	{
		if (LocalBundlesMap[bundleName].FilePath.Equals(BundleFilePath))
		{
			bFound = true;
			LocalBundlesMap[bundleName].BundleState = EPulseContentBundleQueryState::AlreadyMounted;
			break;
		}
	}
	if (!bFound)
		return;
	SaveLocalManifest();
}


void UPulseContentProvider::FetchRemoteBundle_Implementation(const FPulseBundleDescriptor& BundleDescriptor)
{
}

void UPulseContentProvider::FetchRemoteManifest_Implementation()
{
}
