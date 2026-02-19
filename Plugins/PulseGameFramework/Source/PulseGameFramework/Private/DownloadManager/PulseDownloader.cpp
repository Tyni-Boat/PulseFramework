// Copyright © by Tyni Boat. All Rights Reserved.


#include "DownloadManager/PulseDownloader.h"

#include <execution>

#include "HttpModule.h"
#include "PulseGameFramework.h"
#include "Algo/Count.h"
#include "Core/PulseSystemLibrary.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/GameplayStatics.h"


bool UPulseDownloader::StartDownload_Internal(const FDownloadIdentifier& DownloadIdentifier)
{
	if (!DownloadIdentifier.IsValid())
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Download Failed: Invalid Download Identifier-> %s"), *DownloadIdentifier.ToString());
		return false;
	}
	UPulseDownloadTask* DownloadTask = NewObject<UPulseDownloadTask>();
	DownloadTask->Identifier = DownloadIdentifier;
	DownloadTask->DownloadState = EDownloadState::Queued;
	Downloads.Add(DownloadIdentifier.Id, DownloadTask);
	AsyncTask(ENamedThreads::GameThread, [this, DownloadIdentifier]()-> void { OnDownloadQueued.Broadcast(DownloadIdentifier.Id); });
	return RequestDownloadStart(DownloadIdentifier.Id);
}

bool UPulseDownloader::RequestDownloadStart(const FGuid& DownloadId)
{
	int32 downloadingCount = 0;
	for (const auto& pair : Downloads)
	{
		if (!pair.Value)
			continue;
		if (pair.Value->DownloadState == EDownloadState::Downloading)
			downloadingCount++;
	}
	if (downloadingCount >= _maxConcurrentDownloads)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Request Failed: Too many active downloads (%d) (ID:%s)"), downloadingCount, *DownloadId.ToString());
		return false;
	}
	if (!Downloads.Contains(DownloadId))
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Request Failed: Unexpected Download ID (ID:%s)"), *DownloadId.ToString());
		return false;
	}
	auto Task = Downloads[DownloadId];
	if (!Task)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Request Failed: Null Download Task (ID:%s)"), *DownloadId.ToString());
		return false;
	}
	if (!Task->Identifier.IsValid())
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Request Failed: (Paused) Invalid task identifier (Task:%s)"), *Task->Identifier.ToString());
		return false;
	}

	switch (Task->DownloadState)
	{
	default:
		break;
	case EDownloadState::Queued:
	{
		// Handle download attempts
		if (!Task->IsInitialized())
		{
			if (Task->bIsInfosRequestOngoing)
				return false;
			Task->bIsInfosRequestOngoing = true;
			UE_LOG(LogPulseDownloader, Log, TEXT("Start Request: (Queue) task is not initialized. Attempting to query download infos (Task:%s)"),
				*Task->Identifier.ToString());
			// Get content size
			GetRemoteFileInfos(Task->Identifier.Url, _fileInfosQueryTimeOutSeconds, [DownloadId](FString FileName, bool Succeeded, int64 FileSize) -> void
				{
					OnReceiveDownloadInfos(DownloadId, FileName, Succeeded, FileSize);
				});
			return true;
		}
		else
		{
			UE_LOG(LogPulseDownloader, Warning, TEXT("Start Request: (Queue) task already initialized. Will be transferred to Ready (Task:%s)"),
				*Task->Identifier.ToString());
			EDownloadState savedState = Task->Identifier.SavedState;
			Task->DownloadState = EDownloadState::ReadyToDownload;
			BroadcastDownloadEvent(DownloadId, EDownloadState::ReadyToDownload);
			if (savedState == EDownloadState::Downloading || savedState == EDownloadState::Paused)
				RequestDownloadStart(DownloadId);
			return true;
		}
	}
	break;
	case EDownloadState::ReadyToDownload:
	{
		// Handle Ready downloads
		if (!Task->IsInitialized())
		{
			UE_LOG(LogPulseDownloader, Warning, TEXT("Start Request Failed: (Ready) task is not initialized. Will be transferred to attempts (Task:%s)"),
				*Task->Identifier.ToString());
			Task->Identifier.SavedState = EDownloadState::Downloading;
			Task->DownloadState = EDownloadState::Queued;
			BroadcastDownloadEvent(DownloadId, EDownloadState::Queued);
			return RequestDownloadStart(DownloadId);
		}
		auto savedState = Task->Identifier.SavedState;
		if (StartDownloadTask(DownloadId))
		{
			if (savedState == EDownloadState::Paused)
				PauseDownload(DownloadId);
			return true;
		}
		return false;
	}
	break;
	case EDownloadState::Paused:
	{
		// Handle paused downloads
		if (!Task->IsInitialized())
		{
			UE_LOG(LogPulseDownloader, Warning, TEXT("Start Request Failed: (Paused) task is not initialized. Will be transferred to attempts (Task:%s)"),
				*Task->Identifier.ToString());
			Task->Identifier.SavedState = EDownloadState::Downloading;
			Task->DownloadState = EDownloadState::Queued;
			BroadcastDownloadEvent(DownloadId, EDownloadState::Queued);
			return RequestDownloadStart(DownloadId);
		}

		Task->Identifier.SavedState = EDownloadState::Downloading;
		Task->DownloadState = EDownloadState::ReadyToDownload;
		RequestDownloadStart(DownloadId);
		return false;
	}
	break;
	}
	return false;
}

bool UPulseDownloader::StartDownloadTask(const FGuid& DownloadId)
{
	if (!Downloads.Contains(DownloadId))
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Download Failed: Id Not found (%s)"), *DownloadId.ToString());
		return false;
	}
	auto Task = Downloads[DownloadId];
	if (!Task)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Download Failed: Invalid task Ptr"));
		return false;
	}
	if (!Task->Identifier.IsValid())
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Download Failed: Invalid task identifier (Task:%s)"), *Task->Identifier.ToString());
		return false;
	}
	if (!Task->IsInitialized())
	{
		UE_LOG(LogPulseDownloader, Warning, TEXT("Start Download Failed: task is not initialized. (Task:%s)"), *Task->Identifier.ToString());
		return false;
	}
	if (Task->DownloadState == EDownloadState::Downloading)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Instance Download Failed: Task already downloading (Task:%s)"), *Task->Identifier.ToString());
		return true;
	}
	TArray<FGuid> DownloadIds;
	Downloads.GetKeys(DownloadIds);
	int32 samePathCount = 0;
	for (const auto& Download : Downloads)
	{
		if (Download.Value->IsComplete())
			continue;
		if (Download.Value->DownloadState != EDownloadState::Downloading)
			continue;
		if (Download.Value->DownloadState != EDownloadState::Paused)
			continue;
		if (Download.Value->DownloadState != EDownloadState::Failed)
			continue;
		if (Download.Value->Identifier.GetFilePath() == Task->Identifier.GetFilePath())
			samePathCount++;
	}
	if (samePathCount > 0)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Instance Download Failed: Cannot start a new download for the same file at the same path (Task:%s)"),
			*Task->Identifier.ToString());
		return false;
	}
	Task->Identifier.SavedState = EDownloadState::None;
	Task->DownloadState = EDownloadState::Downloading;
	Task->Identifier.StartDate = FDateTime::Now();
	int32 finalChunkSizeMb = _downloadChunkMBSize;
	if (!Task->bDoServerSupportRange)
		finalChunkSizeMb = FMath::CeilToInt((double)Task->TotalSize / 1024 / 1024);
	DownloadTask(DownloadId, finalChunkSizeMb);
	BroadcastDownloadEvent(DownloadId, EDownloadState::None);
	return true;
}

void UPulseDownloader::DownloadTask(const FGuid& DownloadId, int32 MBChunkSize)
{
	AsyncTask(ENamedThreads::GameThread, [DownloadId, MBChunkSize]()-> void
		{
			auto dm = UPulseDownloader::Get();
			if (!dm)
				return;
			if (!dm->Downloads.Contains(DownloadId))
			{
				UE_LOG(LogPulseDownloader, Error, TEXT("Download task Failed: Id Not found (%s)"), *DownloadId.ToString());
				return;
			}
			auto Task = dm->Downloads[DownloadId];
			if (!Task || (Task && (!Task->Identifier.IsValid() || !Task->IsInitialized())))
			{
				UE_LOG(LogPulseDownloader, Error, TEXT("Failed to Chunk download: Invalid task (Task:%s)"), *(Task ? Task->Identifier.ToString() : "NULL"));
				dm->OnFailedDownloadTask(Task);
				return;
			}
			const int64 byteChunkSize = MBChunkSize > 0 ? MBChunkSize * 1048576 : 1048576; // 1048576 bytes = 1 MB as default chunk size.
			const int32 chunkCount = Task->GenerateChunks(byteChunkSize);
			if (chunkCount <= 0)
			{
				UE_LOG(LogPulseDownloader, Error, TEXT("Failed to Chunk download: No chunk had been generated (Task:%s)"), *Task->Identifier.ToString());
				dm->OnFailedDownloadTask(Task);
				return;
			}
			dm->BindDownloadTask(Task);
			Task->StartChunksDownload(dm->_maxConcurrentChunks, dm->_fileInfosQueryTimeOutSeconds);
			UE_LOG(LogPulseDownloader, Log, TEXT("Starting to download Task %s"), *Task->Identifier.ToString());
		});
}

void UPulseDownloader::BroadcastDownloadEvent(const FGuid& DownloadId, EDownloadState EventType)
{
	AsyncTask(ENamedThreads::GameThread, [DownloadId, EventType]()-> void
		{
			auto dm = UPulseDownloader::Get();
			if (!dm)
				return;
			if (EventType != EDownloadState::Downloading)
				dm->SaveRememberFile();
			switch (EventType)
			{
			case EDownloadState::None:
				dm->OnDownloadResumedOrStarted.Broadcast(DownloadId);
				break;
			case EDownloadState::Queued:
				dm->OnDownloadQueued.Broadcast(DownloadId);
				break;
			case EDownloadState::ReadyToDownload:
				dm->OnDownloadReadyToStart.Broadcast(DownloadId);
				break;
			case EDownloadState::Downloading:
				dm->OnDownloadOnGoing.Broadcast(DownloadId);
				break;
			case EDownloadState::Paused:
				dm->OnDownloadPaused.Broadcast(DownloadId);
				break;
			case EDownloadState::Completed:
				dm->OnDownloadComplete.Broadcast(DownloadId);
				break;
			case EDownloadState::Failed:
				dm->OnDownloadComplete.Broadcast(DownloadId);
				break;
			case EDownloadState::Cancelled:
				dm->OnDownloadCancelled.Broadcast(DownloadId);
				break;
			}
		});
}

void UPulseDownloader::OnReceiveDownloadInfos(const FGuid& DownloadId, FString FileName, bool Succeeded, int64 FileSize)
{
	auto DownloadManager = UPulseDownloader::Get();
	if (!DownloadManager)
	{
		UE_LOG(LogPulseDownloader, Log, TEXT("Query Download Infos Error: Null DownloadManager"));
		return;
	}
	if (!DownloadManager->Downloads.Contains(DownloadId))
	{
		UE_LOG(LogPulseDownloader, Log, TEXT("Query Download Infos Error: DownloadManager doesn't contains download ID: %s"), *DownloadId.ToString());
		return;
	}
	TObjectPtr<UPulseDownloadTask> CollisionTask = nullptr;
	if (DownloadManager->DownloadFileCollision(FileName, DownloadId, CollisionTask))
	{
		DownloadManager->Downloads.Remove(DownloadId);
		UE_LOG(LogPulseDownloader, Warning, TEXT("Query Download Infos Update: File name collision detected. File name: %s (Download ID:%s) Collided with Task:%s"),
			*FileName, *DownloadId.ToString(), *(CollisionTask ? CollisionTask->Identifier.ToString() : "NULL"));
		if (CollisionTask && CollisionTask->DownloadState == EDownloadState::Paused)
		{
			DownloadManager->ResumeDownload(CollisionTask->Identifier.Id);
		}
		return;
	}
	auto DownloadTask = DownloadManager->Downloads[DownloadId];
	if (!DownloadTask)
	{
		UE_LOG(LogPulseDownloader, Log, TEXT("Query Download Infos Error: Null Download Task. download ID: %s"), *DownloadId.ToString());
		return;
	}
	if (!Succeeded)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Query Download Infos Failed: Failed to get file size. (Task:%s)"), *DownloadTask->Identifier.ToString());
		DownloadTask->bIsInfosRequestOngoing = false;
		DownloadTask->DownloadState = EDownloadState::Failed;
		DownloadManager->BroadcastDownloadEvent(DownloadId, EDownloadState::Failed);
		return;
	}
	DownloadTask->Identifier.FileName = FileName;
	if (FileSize <= 0)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Query Download Infos Failed: File Size is 0. (Task:%s)"), *DownloadTask->Identifier.ToString());
		DownloadTask->bIsInfosRequestOngoing = false;
		DownloadTask->DownloadState = EDownloadState::Failed;
		DownloadManager->BroadcastDownloadEvent(DownloadId, EDownloadState::Failed);
		return;
	}
	if (FileSize >= (DownloadManager->_downloadChunkMBSize * 1024 * 1024))
	{
		UE_LOG(LogPulseDownloader, Warning, TEXT("Query Download Infos: File Size (%s) is larger than a chunk(%s). Verifying Range Capability. (Task:%s)"),
			*UPulseSystemLibrary::FileSizeToString(FileSize), *UPulseSystemLibrary::FileSizeToString(DownloadManager->_downloadChunkMBSize * 1024 * 1024),
			*DownloadTask->Identifier.ToString());
		// The file size is larger than a chunk. checking server capability to download in ranges
		VerifyRangeRequest(DownloadTask->Identifier.Url, [DownloadId, FileName, Succeeded, FileSize](bool bDoSupportRange)-> void
			{
				OnPostReceiveDownloadInfos(DownloadId, FileName, FileSize, bDoSupportRange);
			});
		return;
	}
	OnPostReceiveDownloadInfos(DownloadId, FileName, FileSize, false);
}

void UPulseDownloader::OnPostReceiveDownloadInfos(const FGuid& DownloadId, FString FileName, int64 FileSize, bool bSupportChunking)
{
	auto DownloadManager = UPulseDownloader::Get();
	if (!DownloadManager)
		return;
	if (!DownloadManager->Downloads.Contains(DownloadId))
		return;
	auto DownloadTask = DownloadManager->Downloads[DownloadId];
	if (!DownloadTask)
		return;
	DownloadTask->bIsInfosRequestOngoing = false;
	DownloadTask->Identifier.FileName = FileName;
	DownloadTask->TotalSize = FileSize;
	if (!bSupportChunking && FileSize >= 2147483647)
	{
		UE_LOG(LogPulseDownloader, Error,
			TEXT("Query Download Infos Failed: File size exceed limit but server url doesn't support Ranged downloads. File Size (%s / %s) (Task:%s)"),
			*UPulseSystemLibrary::FileSizeToString(FileSize), *UPulseSystemLibrary::FileSizeToString(2147483647.0), *DownloadTask->Identifier.ToString());
		return;
	}
	UE_LOG(LogPulseDownloader, Log, TEXT("Query Download Infos: Successfully initialized. File size = %s (Task:%s)"), *UPulseSystemLibrary::FileSizeToString(FileSize),
		*DownloadTask->Identifier.ToString());
	DownloadTask->bDoServerSupportRange = bSupportChunking;
	DownloadTask->DownloadState = EDownloadState::ReadyToDownload;
	EDownloadState savedState = DownloadTask->Identifier.SavedState;
	DownloadManager->BroadcastDownloadEvent(DownloadId, EDownloadState::ReadyToDownload);
	if (savedState == EDownloadState::Downloading || savedState == EDownloadState::Paused)
		DownloadManager->RequestDownloadStart(DownloadId);
}

void UPulseDownloader::BindDownloadTask(UPulseDownloadTask* DownloadTask)
{
	if (!DownloadTask)
		return;
	if (DownloadTask->bIsBound)
		return;
	if (!DownloadTask->OnTaskCancelled.IsBoundToObject(this))
		DownloadTask->OnTaskCancelled.AddUObject(this, &UPulseDownloader::OnCancelledDownloadTask);
	if (!DownloadTask->OnTaskCompleted.IsBoundToObject(this))
		DownloadTask->OnTaskCompleted.AddUObject(this, &UPulseDownloader::OnCompletedDownloadTask);
	if (!DownloadTask->OnTaskFailed.IsBoundToObject(this))
		DownloadTask->OnTaskFailed.AddUObject(this, &UPulseDownloader::OnFailedDownloadTask);
	if (!DownloadTask->OnTaskPaused.IsBoundToObject(this))
		DownloadTask->OnTaskPaused.AddUObject(this, &UPulseDownloader::OnPausedDownloadTask);
	if (!DownloadTask->OnTaskUpdate.IsBoundToObject(this))
		DownloadTask->OnTaskUpdate.AddUObject(this, &UPulseDownloader::OnUpdateDownloadTask);
	DownloadTask->bIsBound = true;
}

void UPulseDownloader::UnbindDownloadTask(UPulseDownloadTask* DownloadTask) const
{
	if (!DownloadTask)
		return;
	DownloadTask->OnTaskCancelled.RemoveAll(this);
	DownloadTask->OnTaskCompleted.RemoveAll(this);
	DownloadTask->OnTaskFailed.RemoveAll(this);
	DownloadTask->OnTaskPaused.RemoveAll(this);
	DownloadTask->OnTaskUpdate.RemoveAll(this);
	DownloadTask->bIsBound = false;
}

void UPulseDownloader::OnUpdateDownloadTask(UPulseDownloadTask* DownloadTask)
{
	if (!DownloadTask)
		return;
	BroadcastDownloadEvent(DownloadTask->Identifier.Id, EDownloadState::Downloading);
}

void UPulseDownloader::OnCompletedDownloadTask(UPulseDownloadTask* DownloadTask)
{
	if (!DownloadTask)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Failed to Download task Id %s: Task is now Null"), *DownloadTask->Identifier.ToString());
		return;
	}
	int64 FinalSize = IFileManager::Get().FileSize(*DownloadTask->Identifier.GetFilePath());
	UE_LOG(LogPulseDownloader, Log, TEXT("Download Completed: %s/%s (Task:%s)"), *UPulseSystemLibrary::FileSizeToString(FinalSize),
		*UPulseSystemLibrary::FileSizeToString(DownloadTask->GetTotalSize()), *DownloadTask->Identifier.ToString());
	DownloadTask->DownloadState = EDownloadState::Completed;
	DownloadTask->Identifier.SavedState = EDownloadState::Completed;
	UnbindDownloadTask(DownloadTask);
	const int32 index = SavedDownloads.DownloadHistory.IndexOfByKey(DownloadTask->Identifier);
	if (SavedDownloads.DownloadHistory.IsValidIndex(index))
		SavedDownloads.DownloadHistory[index] = DownloadTask->Identifier;
	else
		SavedDownloads.DownloadHistory.Add(DownloadTask->Identifier);
	Downloads.Remove(DownloadTask->Identifier.Id);
	BroadcastDownloadEvent(DownloadTask->Identifier.Id, EDownloadState::Completed);
	StartQueueDownload();
}

void UPulseDownloader::OnFailedDownloadTask(UPulseDownloadTask* DownloadTask)
{
	if (!DownloadTask)
		return;
	DownloadTask->DownloadState = EDownloadState::Failed;
	UnbindDownloadTask(DownloadTask);
	DownloadTask->DeleteChunkFiles();
	BroadcastDownloadEvent(DownloadTask->Identifier.Id, EDownloadState::Failed);
	StartQueueDownload();
}

void UPulseDownloader::OnCancelledDownloadTask(UPulseDownloadTask* DownloadTask)
{
	if (!DownloadTask)
		return;
	UnbindDownloadTask(DownloadTask);
	DownloadTask->DeleteChunkFiles();
	IFileManager::Get().Delete(*DownloadTask->Identifier.GetFilePath());
	BroadcastDownloadEvent(DownloadTask->Identifier.Id, EDownloadState::Cancelled);
	StartQueueDownload();
}

void UPulseDownloader::OnPausedDownloadTask(UPulseDownloadTask* DownloadTask)
{
	if (!DownloadTask)
		return;
	UnbindDownloadTask(DownloadTask);
	BroadcastDownloadEvent(DownloadTask->Identifier.Id, EDownloadState::Paused);
	StartQueueDownload();
}

void UPulseDownloader::SaveRememberFile()
{
	for (const auto& entry : Downloads)
	{
		if (!entry.Value)
			continue;
		auto identifier = entry.Value->Identifier;
		identifier.SavedState = entry.Value->DownloadState;
		identifier.SavedTotalSize = entry.Value->TotalSize;
		identifier.SavedDownloadedSize = entry.Value->GetDownloadedSize();
		const int32 index = SavedDownloads.DownloadHistory.IndexOfByKey(identifier);
		if (SavedDownloads.DownloadHistory.IsValidIndex(index))
			SavedDownloads.DownloadHistory[index] = identifier;
		else
			SavedDownloads.DownloadHistory.Add(identifier);
	}
	UE_LOG(LogPulseDownloader, Log, TEXT("Save Meta file started"));
	auto saveObj = UGameplayStatics::CreateSaveGameObject(USavedDownload::StaticClass());
	if (!saveObj)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Unable to Save Meta file: Invalid save object"));
		return;
	}
	auto dSaveObj = Cast<USavedDownload>(saveObj);
	if (!dSaveObj)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Unable to Save Meta file: Types somehow mismatch"));
		return;
	}
	dSaveObj->PulseDownloadSaved = SavedDownloads;
	FAsyncSaveGameToSlotDelegate CallBack;
	CallBack.BindUObject(this, &UPulseDownloader::OnRememberFileSaved);
	UGameplayStatics::AsyncSaveGameToSlot(dSaveObj, "DownloadMetaDatas", 0, CallBack);
}

void UPulseDownloader::LoadRememberFile()
{
	UE_LOG(LogPulseDownloader, Log, TEXT("Load Meta file Started"));
	FAsyncLoadGameFromSlotDelegate Callback;
	Callback.BindUObject(this, &UPulseDownloader::OnRememberFileLoaded);
	UGameplayStatics::AsyncLoadGameFromSlot("DownloadMetaDatas", 0, Callback);
}

void UPulseDownloader::OnRememberFileLoaded(const FString&, const int32, USaveGame* Save)
{
	if (!Save)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Load Meta file completed with error: Invalid Save Data"));
		return;
	}
	const auto asDSave = Cast<USavedDownload>(Save);
	if (!asDSave)
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Load Meta file completed with error: Unable to cast to the right type"));
		return;
	}
	UE_LOG(LogPulseDownloader, Log, TEXT("Load Meta file completed successfully"));
	SavedDownloads = asDSave->PulseDownloadSaved;
	auto MakeTaskLambda = [](const FDownloadIdentifier& DownloadId, const EDownloadState InitialState)-> UPulseDownloadTask*
		{
			UPulseDownloadTask* DownloadTask = NewObject<UPulseDownloadTask>();
			DownloadTask->Identifier = DownloadId;
			DownloadTask->DownloadState = InitialState;
			return DownloadTask;
		};
	for (const auto& entry : SavedDownloads.DownloadHistory)
	{
		switch (entry.SavedState)
		{
		case EDownloadState::None:
			break;
		case EDownloadState::Queued:
			Downloads.Add(entry.Id, MakeTaskLambda(entry, EDownloadState::Queued));
			OnDownloadQueued.Broadcast(entry.Id);
			break;
		case EDownloadState::ReadyToDownload:
			StartDownload_Internal(entry);
			break;
		case EDownloadState::Downloading:
			StartDownload_Internal(entry);
			break;
		case EDownloadState::Paused:
			Downloads.Add(entry.Id, MakeTaskLambda(entry, EDownloadState::Paused));
			OnDownloadPaused.Broadcast(entry.Id);
			break;
		case EDownloadState::Completed:
			OnDownloadComplete.Broadcast(entry.Id);
			break;
		case EDownloadState::Failed:
			Downloads.Add(entry.Id, MakeTaskLambda(entry, EDownloadState::Failed));
			OnDownloadComplete.Broadcast(entry.Id);
			break;
		case EDownloadState::Cancelled:
			Downloads.Add(entry.Id, MakeTaskLambda(entry, EDownloadState::Cancelled));
			OnDownloadCancelled.Broadcast(entry.Id);
			break;
		}
	}
	StartQueueDownload();
}

void UPulseDownloader::OnRememberFileSaved(const FString& SlotName, int UserIndex, bool Success) const
{
	if (Success)
	{
		UE_LOG(LogPulseDownloader, Log, TEXT("Save Meta file completed successfully"));
	}
	else
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Save Meta file completed with error"));
	}
}


bool UPulseDownloader::StartDownload(const FString& Url, FGuid& OutDownloadId, const FString& DownloadDirectory, bool bImmediateStart)
{
	if (Url.IsEmpty())
		return false;
	FDownloadIdentifier Identifier;
	Identifier.Id = FGuid::NewGuid();
	Identifier.Url = Url;
	Identifier.Directory = DownloadDirectory;
	if (Identifier.Directory.IsEmpty())
		Identifier.Directory = FPaths::ProjectPersistentDownloadDir();
	if (!UPulseSystemLibrary::FileIsPathWritable(Identifier.Directory))
	{
		UE_LOG(LogPulseDownloader, Error, TEXT("Start Download Failed: Directory %s is not writable. Url: %s"), *Identifier.Directory, *Url);
		return false;
	}
	Identifier.SavedState = bImmediateStart ? EDownloadState::Downloading : EDownloadState::None;
	GetFileNameFromURL(Url, Identifier.FileName);
	if (StartDownload_Internal(Identifier))
	{
		OutDownloadId = Identifier.Id;
		return true;
	}
	return false;
}

bool UPulseDownloader::PauseDownload(const FGuid& DownloadId)
{
	if (!Downloads.Contains(DownloadId))
		return false;
	if (!Downloads[DownloadId])
		return false;
	auto DownloadTask = Downloads[DownloadId];
	if (DownloadTask->DownloadState != EDownloadState::Downloading)
		return false;
	if (DownloadTask->bIsActionRequestOngoing)
		return false;
	auto pastState = DownloadTask->DownloadState;
	DownloadTask->DownloadState = EDownloadState::Paused;
	if (!DownloadTask->PauseTask())
	{
		DownloadTask->DownloadState = pastState;
		return false;
	}
	return true;
}

bool UPulseDownloader::ResumeDownload(const FGuid& DownloadId)
{
	if (!Downloads.Contains(DownloadId))
		return false;
	if (!Downloads[DownloadId])
		return false;
	auto DownloadTask = Downloads[DownloadId];
	if (DownloadTask->DownloadState != EDownloadState::Paused && DownloadTask->DownloadState != EDownloadState::ReadyToDownload)
		return false;
	if (DownloadTask->DownloadState == EDownloadState::Downloading)
		return false;
	return RequestDownloadStart(DownloadId);
}

bool UPulseDownloader::RetryDownload(const FGuid& DownloadId)
{
	if (!Downloads.Contains(DownloadId))
		return false;
	if (!Downloads[DownloadId])
		return false;
	auto Task = Downloads[DownloadId];
	Task->Identifier.SavedState = EDownloadState::Downloading;
	Task->DownloadState = EDownloadState::Queued;
	BroadcastDownloadEvent(Task->Identifier.Id, EDownloadState::Queued);
	return RequestDownloadStart(DownloadId);
}

bool UPulseDownloader::CancelDownload(const FGuid& DownloadId)
{
	if (!Downloads.Contains(DownloadId))
		return false;
	if (!Downloads[DownloadId])
		return false;
	auto DownloadTask = Downloads[DownloadId];
	if (DownloadTask->DownloadState == EDownloadState::Cancelled)
		return false;
	if (DownloadTask->DownloadState != EDownloadState::Paused && DownloadTask->DownloadState != EDownloadState::Downloading)
		return false;
	if (DownloadTask->bIsActionRequestOngoing)
		return false;
	auto pastState = DownloadTask->DownloadState;
	DownloadTask->DownloadState = EDownloadState::Cancelled;
	if (!DownloadTask->bIsBound && pastState != EDownloadState::Downloading)
	{
		OnCancelledDownloadTask(DownloadTask);
		return true;
	}
	if (!DownloadTask->CancelTask())
	{
		DownloadTask->DownloadState = pastState;
		return false;
	}
	return true;
}

bool UPulseDownloader::GetDownloads(TArray<FGuid>& OutDownloadIds) const
{
	OutDownloadIds.Empty();
	for (const auto& Download : Downloads)
		OutDownloadIds.Add(Download.Key);
	return OutDownloadIds.Num() > 0;
}

bool UPulseDownloader::GetCompletedDownloads(TArray<FGuid>& OutDownloadIds) const
{
	OutDownloadIds.Empty();
	for (const auto& Download : SavedDownloads.DownloadHistory)
	{
		if (Download.SavedState != EDownloadState::Completed)
			continue;
		OutDownloadIds.Add(Download.Id);
	}
	return OutDownloadIds.Num() > 0;
}

bool UPulseDownloader::GetDownloadState(const FGuid& DownloadId, EDownloadState& OutState) const
{
	OutState = EDownloadState::None;
	if (Downloads.Contains(DownloadId))
	{
		OutState = Downloads[DownloadId]->DownloadState;
		return true;
	}
	const int32 index = SavedDownloads.DownloadHistory.IndexOfByPredicate([DownloadId](const FDownloadIdentifier& dId)-> bool { return dId.Id == DownloadId; });
	if (index >= 0 && UPulseSystemLibrary::FileExist(SavedDownloads.DownloadHistory[index].GetFilePath()))
	{
		OutState = EDownloadState::Completed;
		return true;
	}
	return false;
}

bool UPulseDownloader::GetDownloadTotalSize(const FGuid& DownloadId, int64& OutFileSize) const
{
	OutFileSize = 0;
	if (Downloads.Contains(DownloadId))
	{
		OutFileSize = Downloads[DownloadId]->GetTotalSize();
		return true;
	}
	const int32 index = SavedDownloads.DownloadHistory.IndexOfByPredicate([DownloadId](const FDownloadIdentifier& dId)-> bool { return dId.Id == DownloadId; });
	if (SavedDownloads.DownloadHistory.IsValidIndex(index))
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		OutFileSize = PF.FileSize(*SavedDownloads.DownloadHistory[index].GetFilePath());
		return true;
	}
	return false;
}

bool UPulseDownloader::GetDownloadFileName(const FGuid& DownloadId, FString& OutFileName) const
{
	OutFileName.Empty();
	if (Downloads.Contains(DownloadId))
	{
		OutFileName = Downloads[DownloadId]->Identifier.FileName;
		return true;
	}
	const int32 index = SavedDownloads.DownloadHistory.IndexOfByPredicate([DownloadId](const FDownloadIdentifier& dId)-> bool { return dId.Id == DownloadId; });
	if (SavedDownloads.DownloadHistory.IsValidIndex(index))
	{
		OutFileName = SavedDownloads.DownloadHistory[index].FileName;
		return true;
	}
	return false;
}

bool UPulseDownloader::GetDownloadIdentifier(const FGuid& DownloadId, FDownloadIdentifier& OutIdentifier) const
{
	if (Downloads.Contains(DownloadId))
	{
		OutIdentifier = Downloads[DownloadId]->Identifier;
		return true;
	}
	const int32 index = SavedDownloads.DownloadHistory.IndexOfByPredicate([DownloadId](const FDownloadIdentifier& dId)-> bool { return dId.Id == DownloadId; });
	if (SavedDownloads.DownloadHistory.IsValidIndex(index))
	{
		OutIdentifier = SavedDownloads.DownloadHistory[index];
		return true;
	}
	return false;
}

bool UPulseDownloader::GetDownloadProgress(const FGuid& DownloadId, float& OutProgress) const
{
	OutProgress = 0.0f;
	if (!Downloads.Contains(DownloadId))
		return false;
	if (!Downloads[DownloadId])
		return false;
	auto DownloadTask = Downloads[DownloadId];
	if (DownloadTask->DownloadState != EDownloadState::Downloading && DownloadTask->DownloadState != EDownloadState::Paused)
		return false;
	if (DownloadTask->GetTotalSize() > 0)
	{
		OutProgress = (float)DownloadTask->GetDownloadedSize() / (float)DownloadTask->GetTotalSize();
		return true;
	}
	return false;
}

bool UPulseDownloader::GetDetailedDownloadProgress(const FGuid& DownloadId, TArray<float>& OutProgresses) const
{
	OutProgresses.Empty();
	if (!Downloads.Contains(DownloadId))
		return false;
	if (!Downloads[DownloadId])
		return false;
	auto DownloadTask = Downloads[DownloadId];
	if (DownloadTask->DownloadState != EDownloadState::Downloading && DownloadTask->DownloadState != EDownloadState::Paused)
		return false;
	if (DownloadTask->GetTotalSize() > 0)
	{
		TArray<int64> TotalSizes;
		TArray<int64> DownloadedSizes;
		DownloadTask->GetDetailedTotalSizes(TotalSizes);
		DownloadTask->GetDetailedDownloadedSize(DownloadedSizes);
		for (int i = 0; i < TotalSizes.Num(); i++)
		{
			float p = 0;
			if (DownloadedSizes.IsValidIndex(i))
			{
				if (TotalSizes[i] > 0)
				{
					p = (float)DownloadedSizes[i] / (float)TotalSizes[i];
				}
			}
			OutProgresses.Add(p);
		}
		return true;
	}
	return false;
}

int32 UPulseDownloader::GetPerStateDownloadCount(int32 States) const
{
	int32 Count = 0;
	for (const auto& Download : Downloads)
	{
		if (!Download.Value)
			continue;
		if (States & static_cast<int32>(Download.Value->DownloadState))
			Count++;
	}
	if (States & static_cast<int32>(EDownloadState::Completed))
		Count += SavedDownloads.DownloadHistory.Num();
	return Count;
}

float UPulseDownloader::GetPerStateDownloadAverageProgress(int32 States) const
{
	TArray<float> Progresses;
	for (const auto& Download : Downloads)
	{
		if (!Download.Value)
			continue;
		float Progress = 0.0f;
		if (States & static_cast<int32>(Download.Value->DownloadState) && GetDownloadProgress(Download.Key, Progress))
			Progresses.Add(Progress);
	}
	if (States & static_cast<int32>(EDownloadState::Completed))
		for (const auto& Download : SavedDownloads.DownloadHistory)
			Progresses.Add(1);
	return UPulseSystemLibrary::ArrayAverage(Progresses);
}

void UPulseDownloader::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPulseDownloader, Log, TEXT("Download sub-System Initialization started"));
	if (auto config = GetProjectSettings())
	{
		_maxConcurrentDownloads = config->MaxConcurrentDownloads;
		_downloadChunkMBSize = config->DownloadChunkMBSize;
		_downloadChunkRetries = config->DownloadChunkRetries;
		_fileInfosQueryTimeOutSeconds = config->FileInfosQueryTimeOutSeconds;
		_maxConcurrentChunks = config->MaxConcurrentChunks;
	}
	LoadRememberFile();
}

void UPulseDownloader::StartQueueDownload()
{
	if (Downloads.IsEmpty())
		return;
	TArray<FGuid> DownloadIds;
	Downloads.GetKeys(DownloadIds);
	for (const auto& id : DownloadIds)
	{
		if (!Downloads[id])
		{
			Downloads.Remove(id);
			continue;
		}
		if (Downloads[id]->DownloadState != EDownloadState::ReadyToDownload && Downloads[id]->DownloadState != EDownloadState::Queued)
			continue;
		UE_LOG(LogPulseDownloader, Log, TEXT("Handle Awaiting download Id %s"), *id.ToString());
		if (!RequestDownloadStart(id))
			break;
	}
}

void UPulseDownloader::Deinitialize()
{
	Super::Deinitialize();
	SaveRememberFile();
	TArray<FGuid> DownloadIds;
	Downloads.GetKeys(DownloadIds);
	for (const auto& id : DownloadIds)
	{
		if (!Downloads[id])
		{
			Downloads.Remove(id);
			continue;
		}
		Downloads[id]->EmergencyHaltTask();
	}
}


void UPulseDownloader::VerifyRangeRequest(const FString& Url, TFunction<void(bool)> OnResult)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(Url);
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Range"), TEXT("bytes=0-1"));

	Request->OnProcessRequestComplete().BindLambda(
		[OnResult](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			if (!bSuccess || !Resp.IsValid())
			{
				OnResult(false);
				return;
			}

			const int32 Code = Resp->GetResponseCode();

			// 206 = Partial Content (resume supported)
			OnResult(Code == 206);
		}
	);

	Request->ProcessRequest();
}

void UPulseDownloader::GetRemoteFileInfos(const FString& Url, const float TimeOut, TFunction<void(FString, bool, int64)> OnComplete)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("HEAD"));
	Request->SetTimeout(TimeOut); // In sec

	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete](FHttpRequestPtr Req, FHttpResponsePtr Resp, bool bSuccess)
		{
			if (!bSuccess || !Resp.IsValid())
			{
				UE_LOG(LogPulseDownloader, Error, TEXT("Failed to get file size: Success? %d (url:%s)"), bSuccess, *Req->GetURL());
				OnComplete(Req->GetURL(), false, 0);
				return;
			}
			FString Filename;
			FString ContentDisposition = Resp->GetHeader(TEXT("Content-Disposition"));
			if (ContentDisposition.Split(TEXT("filename="), nullptr, &Filename))
				Filename = Filename.Replace(TEXT("\""), TEXT(""));
			const FString LengthStr = Resp->GetHeader(TEXT("Content-Length"));
			OnComplete(Filename, true, FCString::Atoi64(*LengthStr));
		}
	);

	Request->ProcessRequest();
}

bool UPulseDownloader::DownloadFileCollision(const FString& FileName, const FGuid QueryTaskUID, TObjectPtr<UPulseDownloadTask>& OutOngoingTask) const
{
	for (const auto& Download : Downloads)
	{
		if (!Download.Value)
			continue;
		if (Download.Value->Identifier.FileName == FileName)
		{
			if (Download.Value->Identifier.Id == QueryTaskUID)
				continue;
			if (Download.Value->DownloadState != EDownloadState::Downloading && Download.Value->DownloadState != EDownloadState::Paused)
				continue;
			OutOngoingTask = Download.Value;
			return true;
		}
	}
	return false;
}

bool UPulseDownloader::GetFileNameFromURL(const FString& Url, FString& OutFilename)
{
	OutFilename = FPaths::GetCleanFilename(Url);
	return !OutFilename.IsEmpty();
}

UPulseDownloader* UPulseDownloader::Get()
{
	if (FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport))
	{
		UGameInstance* GameInstance = WorldContext->OwningGameInstance;
		if (GameInstance)
		{
			return GameInstance->GetSubsystem<UPulseDownloader>();
		}
	}
	return nullptr;
}

FString UPulseDownloader::ToString(const FDownloadIdentifier& Identifier)
{
	return Identifier.ToString();
}



UPulseDownloadEventListener::UPulseDownloadEventListener()
{
	if (auto downloadSubSystem = UPulseDownloader::Get())
	{
		downloadSubSystem->OnDownloadComplete.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadComplete_Func);
		downloadSubSystem->OnDownloadPaused.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadPaused_Func);
		downloadSubSystem->OnDownloadReadyToStart.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadReadyToStart_Func);
		downloadSubSystem->OnDownloadQueued.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadQueued_Func);
		downloadSubSystem->OnDownloadResumedOrStarted.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadResumedOrStarted_Func);
		downloadSubSystem->OnDownloadOnGoing.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadOnGoing_Func);
		downloadSubSystem->OnDownloadCancelled.AddDynamic(this, &UPulseDownloadEventListener::OnDownloadCancelled_Func);
	}
}

UPulseDownloadEventListener::~UPulseDownloadEventListener()
{
	if (auto downloadSubSystem = UPulseDownloader::Get())
	{
		downloadSubSystem->OnDownloadComplete.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadComplete_Func);
		downloadSubSystem->OnDownloadPaused.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadPaused_Func);
		downloadSubSystem->OnDownloadReadyToStart.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadReadyToStart_Func);
		downloadSubSystem->OnDownloadQueued.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadQueued_Func);
		downloadSubSystem->OnDownloadResumedOrStarted.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadResumedOrStarted_Func);
		downloadSubSystem->OnDownloadOnGoing.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadOnGoing_Func);
		downloadSubSystem->OnDownloadCancelled.RemoveDynamic(this, &UPulseDownloadEventListener::OnDownloadCancelled_Func);
	}
}

void UPulseDownloadEventListener::OnDownloadComplete_Func(const FGuid& DownloadId)
{
	OnDownloadComplete.Broadcast(DownloadId);
}

void UPulseDownloadEventListener::OnDownloadPaused_Func(const FGuid& DownloadId)
{
	OnDownloadPaused.Broadcast(DownloadId);
}

void UPulseDownloadEventListener::OnDownloadReadyToStart_Func(const FGuid& DownloadId)
{
	OnDownloadReadyToStart.Broadcast(DownloadId);
}

void UPulseDownloadEventListener::OnDownloadQueued_Func(const FGuid& DownloadId)
{
	OnDownloadQueued.Broadcast(DownloadId);
}

void UPulseDownloadEventListener::OnDownloadResumedOrStarted_Func(const FGuid& DownloadId)
{
	OnDownloadResumedOrStarted.Broadcast(DownloadId);
}

void UPulseDownloadEventListener::OnDownloadOnGoing_Func(const FGuid& DownloadId)
{
	OnDownloadOnGoing.Broadcast(DownloadId);
}

void UPulseDownloadEventListener::OnDownloadCancelled_Func(const FGuid& DownloadId)
{
	OnDownloadCancelled.Broadcast(DownloadId);
}
