// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseCoreTypes.h"
#include "Types/GameAssetTypes.h"
#include "UObject/Object.h"
#include "PulseContentProvider.generated.h"


UENUM(BlueprintType)
enum class EPulseContentBundleQueryResponse: uint8
{
	NotFound,
	AlreadyMounted,
	AvailableRemotely,
	AvailableLocally,
};

UENUM(BlueprintType)
enum class EPulseContentBundleQueryState: uint8
{
	NotFound,
	UpdateAvailable,
	ReadyToDownload,
	Downloading,
	AvailableLocally,
	AlreadyMounted,
};

USTRUCT(BlueprintType)
struct FPulseBundleDescriptor
{
	GENERATED_BODY()

public:
	// The name of the bundle file.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	FString BundleName = "";

	// The version of the pak file. This will be used for pakOrder when mounting.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	int32 Version = 0;

	// Is the bundle file a .IoStore? else it's a .Pak
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	bool bIsIOStoreBundle = false;

	// The hash of the bundle file.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	FString Hash = "";

	// The address/path of the bundle file in remote location.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	FString Url = "";

	// The path of the bundle file in local. Only available if the file download is completed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	FString FilePath = "";

	// The last modification date of the bundle file.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	FDateTime LastModification;

	// The unique identifier of the bundle download process in the download manager. Only valid if using the Pulse download manager to download the bundle
	// and the download is pending, in progress or paused.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	FGuid DownloadHandleUID;

	// The paths to scan after mounting the Pak file. The path scan will be recursive.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	TArray<FString> MountScanPaths = { "Game/GameContent/DLC" };

	// The modular gameplay features to enable upon Pak mounting 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	TArray<FString> ModularGameplayPlugins;

	// The size in byte of the manifest file.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content", SkipSerialization)
	EPulseContentBundleQueryState BundleState = EPulseContentBundleQueryState::NotFound;

	inline bool CanOverride(const FPulseBundleDescriptor& Other) const
	{
		if (BundleName != Other.BundleName)
			return false;
		return Version > Other.Version;
	}
};

USTRUCT(BlueprintType)
struct FPulseProviderContentManifest
{
	GENERATED_BODY()

public:
	// The version of the manifest. This will be used when ever the structure of the struct change over time.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Asset Content")
	int32 Version = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content")
	TArray<FPulseBundleDescriptor> Bundles;
};

class UPulseContentProvider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FContenDownloadCompleteEventDelegate, TSubclassOf<UPulseContentProvider>, ContentProvider, const FString&, FilePath, bool, Sucess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FContenBundleUpdateAvailableEventDelegate, TSubclassOf<UPulseContentProvider>, ContentProvider, int32, UpdateCount);


/**
 * The Base class for any asset provider 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class PULSEGAMEFRAMEWORK_API UPulseContentProvider : public UObject
{
	GENERATED_BODY()

private:

	FString PendingRemoteManifestLoad = "";
	TMap<FString, EPulseContentBundleQueryState> _beforeFetchBundleStates;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content", meta = (AllowPrivateAccess = "true"))
	TMap<FString, FPulseBundleDescriptor> LocalBundlesMap;

	// The manifest fetched from remote repository
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content", meta = (AllowPrivateAccess = "true"))
	FPulseProviderContentManifest UpToDateManifest;

	// The size of the buffer when checking bundle file md5. default at 1MB (1024*1024) 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content", meta = (AllowPrivateAccess = "true"))
	int32 BundleFileMd5StreamBufferSize = 1048576;

	// The version of the manifest to use when saving the manifest locally
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content", meta = (AllowPrivateAccess = "true"))
	int32 ManifestVersion = 0;

	// Used to lock the provider to prevent fetching or loading manifests during certain operations
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Asset Content", meta = (AllowPrivateAccess = "true"))
	int32 LockManifests = 0;

	UFUNCTION()
	void OnManifestDownloaded_Internal(TSubclassOf<UPulseContentProvider> Provider, const FString& FilePath);

	UFUNCTION()
	void OnBundleDownloaded_Internal(TSubclassOf<UPulseContentProvider> Provider, const FString& BundleName, const FString& FilePath);

	// Must be called when a manifest download operation ends
	UFUNCTION(BlueprintCallable)
	void EndManifestDownload(const FString& InStorageFilePath);

	// Must be called when a bundle download operation ends
	UFUNCTION(BlueprintCallable)
	void EndBundleDownload(const FString& BundleName, const FString& InStorageFilePath);

	// Must be called when a bundle download is cancelled. 
	UFUNCTION(BlueprintCallable)
	void OnBundleDownloadCancelled(const FGuid& DownloadUID);

	// Set a download UID to the corresponding bundle descriptor if none is yet set.
	UFUNCTION(BlueprintCallable)
	bool SetBundleDownloadUID(const FString& BundleUrl, const FGuid& DownloadUID);

public:
	UPROPERTY(BlueprintAssignable, Category="Asset Content")
	FContenDownloadCompleteEventDelegate OnBundleDownloadCompleted;

	UPROPERTY(BlueprintAssignable, Category="Asset Content")
	FContenDownloadCompleteEventDelegate OnBundleReadyToMount;

	UPROPERTY(BlueprintAssignable, Category="Asset Content")
	FContenDownloadCompleteEventDelegate OnContentManifestDownloadCompleted;

	UPROPERTY(BlueprintAssignable, Category="Asset Content")
	FContenBundleUpdateAvailableEventDelegate OnBundleUpdateAvailable;


	void Initialize(const UCoreProjectSetting* projectSettings);
	bool LoadLocalManifest();
	bool SaveLocalManifest();
	int GetBundleAvailableUpdatesCount() const;
	int GetBundleAvailableUpdates(TArray<FPulseBundleDescriptor>& OutBundleDescriptors) const;
	void UpdateBundles();
	bool DoBundleExistLocally(const FString& BundleLocalPath) const;
	bool GetLocalBundleDescriptor(const FString& BundleLocalPath, FPulseBundleDescriptor& OutDescriptor) const;
	EPulseContentBundleQueryState GetBundleState(const FString& BundleName);
	bool QueryBundleToMount(const FString& BundleName, FString& OutBundlePath, EPulseContentBundleQueryResponse& OutQueryResponse);
	bool DeleteBundle(const FString& BundleName);
	void BeginFetchRemoteManifest();
	void BeginFetchRemoteBundle(const FString& BundleName);
	void OnBundleMounted(const FString& BundleFilePath);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Asset Content")
	void FetchRemoteBundle(const FPulseBundleDescriptor& BundleDescriptor);
	virtual void FetchRemoteBundle_Implementation(const FPulseBundleDescriptor& BundleDescriptor);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Asset Content")
	void FetchRemoteManifest();
	virtual void FetchRemoteManifest_Implementation();
};
