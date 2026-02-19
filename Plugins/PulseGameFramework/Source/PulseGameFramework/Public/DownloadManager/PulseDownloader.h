// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseDownloadTask.h"
#include "Core/PulseCoreTypes.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Object.h"
#include "PulseDownloader.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPulseDownloadDelegateEvent, const FGuid&, DownloadId);



/**
 * The Pulse downloader Game Instance Sub-System to download files from the internet using HTTP direct links.
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseDownloader : public UGameInstanceSubsystem, public IIPulseCore
{
	GENERATED_BODY()

protected:
	
	FPulseDownloadSaved SavedDownloads;
	UPROPERTY()
	TMap<FGuid, TObjectPtr<UPulseDownloadTask>> Downloads;
	int32 _maxConcurrentDownloads = 3;
	int32 _maxConcurrentChunks = 3;
	int32 _downloadChunkMBSize = 100;
	int32 _downloadChunkRetries = 3;
	int32 _fileInfosQueryTimeOutSeconds = 5;

	// Make a download Task from an identifier
	bool StartDownload_Internal(const FDownloadIdentifier& DownloadIdentifier);

	// Request beginning of a download task. Will only get file infos if task not ready
	bool RequestDownloadStart(const FGuid& DownloadId);

	// Try to start the actual download task
	bool StartDownloadTask(const FGuid& DownloadId);
	
	static void DownloadTask(const FGuid& DownloadId, int32 MBChunkSize);

	void BroadcastDownloadEvent(const FGuid& DownloadId, EDownloadState EventType);

	static void OnReceiveDownloadInfos(const FGuid& DownloadId, FString FileName, bool Succeeded, int64 FileSize);
	
	static void OnPostReceiveDownloadInfos(const FGuid& DownloadId, FString FileName, int64 FileSize, bool bSupportChunking);

	void BindDownloadTask(UPulseDownloadTask* DownloadTask);
	
	void UnbindDownloadTask(UPulseDownloadTask* DownloadTask) const;
	
	void OnUpdateDownloadTask(UPulseDownloadTask* DownloadTask);
	
	void OnCompletedDownloadTask(UPulseDownloadTask* DownloadTask);
	
	void OnFailedDownloadTask(UPulseDownloadTask* DownloadTask);
	
	void OnCancelledDownloadTask(UPulseDownloadTask* DownloadTask);
	
	void OnPausedDownloadTask(UPulseDownloadTask* DownloadTask);
	
	void SaveRememberFile();
	
	void LoadRememberFile();

	void OnRememberFileLoaded(const FString& slotName, const int32 UserIndex, USaveGame* Save);
	
	void OnRememberFileSaved(const FString& SlotName, int UserIndex, bool Success) const;
	
public:

	// Triggerred when a download is completed or failed. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadComplete;

	// Triggerred when a download is Paused. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadPaused;

	// Triggerred when a download is Ready to be started. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadReadyToStart;

	// Triggerred when a download has been put on Queue, thus not ready to be downloaded. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadQueued;

	// Triggerred when a download has been started or resumed. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadResumedOrStarted;

	// Triggerred when a download receive new bytes from the server. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadOnGoing;

	// Triggerred when a download is Cancelled by the user. It broadcast the UID of the download.
	UPROPERTY(BlueprintAssignable, Category="Pulse Download")
	FPulseDownloadDelegateEvent OnDownloadCancelled;

	
	/**
	 * @brief Try to start a new download from url, and return the download ID 
	 * @param Url the Http/Https direct file download link
	 * @param OutDownloadId The Output Download ID
	 * @param DownloadDirectory The sub-folder in the download folder where to save the file (must exist and be writable) [Optional] 
	 * @param bImmediateStart Start the download as soon as it get ready to be downloaded.
	 * @return True if the download was successfully put in the download Queue.
	 */
	UFUNCTION(BlueprintCallable, Category="Pulse Download", meta=(AdvancedDisplay = 1))
	bool StartDownload(const FString& Url, FGuid& OutDownloadId, const FString& DownloadDirectory = "", bool bImmediateStart = true);

	// Pause an active download by ID
	UFUNCTION(BlueprintCallable, Category="Pulse Download")
	bool PauseDownload(const FGuid& DownloadId);

	// Resume a paused download by ID
	UFUNCTION(BlueprintCallable, Category="Pulse Download")
	bool ResumeDownload(const FGuid& DownloadId);

	// Retry a cancelled or failed download by ID
	UFUNCTION(BlueprintCallable, Category="Pulse Download")
	bool RetryDownload(const FGuid& DownloadId);

	// Cancel an active or paused download by ID
	UFUNCTION(BlueprintCallable, Category="Pulse Download")
	bool CancelDownload(const FGuid& DownloadId);

	// Get all download Ids except for the completed ones
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDownloads(TArray<FGuid>& OutDownloadIds) const;

	// Get all the completed download Ids
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetCompletedDownloads(TArray<FGuid>& OutDownloadIds) const;

	// Get the state of a download by ID
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDownloadState(const FGuid& DownloadId, EDownloadState& OutState) const;

	// Get expected byte size of the file to download or the size of the downloaded file if completed.
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDownloadTotalSize(const FGuid& DownloadId, int64& OutFileSize) const;

	// Get the download file name
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDownloadFileName(const FGuid& DownloadId, FString& OutFileName) const;

	// Get the Identifier (Download meta) of a download by ID
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDownloadIdentifier(const FGuid& DownloadId, FDownloadIdentifier& OutIdentifier) const;

	// Get the overall percentage of the download by ID
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDownloadProgress(const FGuid& DownloadId, float& OutProgress) const;
	
	// Get the per-chunk percentage of the download by ID
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	bool GetDetailedDownloadProgress(const FGuid& DownloadId, TArray<float>& OutProgresses) const;
	
	// Get the total number of download by download state
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	int32 GetPerStateDownloadCount(UPARAM(meta = (Bitmask, BitmaskEnum = EDownloadState)) int32 States) const;
	
	// Get the average download completion of all the download currently in the provided states.
	UFUNCTION(BlueprintPure, Category="Pulse Download")
	float GetPerStateDownloadAverageProgress(UPARAM(meta = (Bitmask, BitmaskEnum = EDownloadState)) int32 States) const;

	// Start the downloads awaiting in Queue
	UFUNCTION(BlueprintCallable, Category="Pulse Download")
	void StartQueueDownload();

	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static void VerifyRangeRequest(const FString& Url, TFunction<void(bool)> OnResult);
	static void GetRemoteFileInfos(const FString& Url, const float TimeOut, TFunction<void(FString, bool, int64)> OnComplete);
	bool DownloadFileCollision(const FString& FileName, const FGuid QueryTaskUID, TObjectPtr<UPulseDownloadTask>& OutOngoingTask) const;
	static bool GetFileNameFromURL(const FString& Url, FString& OutFilename);
	static UPulseDownloader* Get();

	UFUNCTION(BlueprintPure, Category="Pulse Download")
	static FString ToString(const FDownloadIdentifier& Identifier);
};
