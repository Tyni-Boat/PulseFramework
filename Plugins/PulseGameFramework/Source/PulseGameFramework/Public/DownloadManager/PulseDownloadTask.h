// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseDownloadTypes.h"
#include "DownloadManager/PulseDownloadChunk.h"
#include "UObject/Object.h"
#include "PulseDownloadTask.generated.h"


class UPulseDownloadTask;
DECLARE_MULTICAST_DELEGATE_OneParam(FDownloadTaskEventDelegate, UPulseDownloadTask* Task);


/**
 * Represent an ongoing download task, with one or several download chunks 
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseDownloadTask : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FDownloadIdentifier Identifier;

	FDownloadTaskEventDelegate OnTaskUpdate;
	FDownloadTaskEventDelegate OnTaskCompleted;
	FDownloadTaskEventDelegate OnTaskFailed;
	FDownloadTaskEventDelegate OnTaskCancelled;
	FDownloadTaskEventDelegate OnTaskPaused;
	
	UPROPERTY(SkipSerialization)
	int64 TotalSize = 0;

	UPROPERTY(SkipSerialization)
	bool bDoServerSupportRange = false;

	UPROPERTY(SkipSerialization)
	int32 ParallelChunkCount = 1;
	
	UPROPERTY(SkipSerialization)
	EDownloadState DownloadState = EDownloadState::None;
	
	UPROPERTY(SkipSerialization)
	bool bIsInfosRequestOngoing = false;
	
	UPROPERTY(SkipSerialization)
	bool bIsActionRequestOngoing = false;
	
	UPROPERTY(SkipSerialization)
	bool bIsBound = false;

	UPROPERTY()
	TArray<TObjectPtr<UPulseDownloadChunk>> Chunks;

	bool IsInitialized() const;
	bool IsComplete() const;
	int64 GetTotalSize() const;
	
	int32 GenerateChunks(const int64 ChunkSize);
	void StartChunksDownload(int32 MaxParallelChunks = 3, const float TimeOut = 5);
	int64 GetDownloadedSize();
	void GetDetailedDownloadedSize(TArray<int64>& OutChunkDownloadedSizes);
	void GetDetailedTotalSizes(TArray<int64>& OutChunkTotalSizes);
	bool StopChunk(UPulseDownloadChunk* Chunk);
	bool PauseTask();	
	void EmergencyHaltTask();
	bool CancelTask();
	bool AddChunk(UPulseDownloadChunk* Chunk);
	bool RemoveChunk(const int32 Index, bool DeleteFile = false);
	void DeleteChunkFiles();

	void OnChunkCompleted(UPulseDownloadChunk* Chunk);
	void OnChunkUpdated(UPulseDownloadChunk* Chunk);	
	void OnChunkFailed(UPulseDownloadChunk* Chunk);

	bool operator==(const UPulseDownloadTask& Other) const
	{
		return Identifier == Other.Identifier;
	}
};
