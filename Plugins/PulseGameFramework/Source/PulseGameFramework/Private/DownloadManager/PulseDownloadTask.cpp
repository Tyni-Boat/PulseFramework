// Copyright © by Tyni Boat. All Rights Reserved.


#include "DownloadManager/PulseDownloadTask.h"
#include "Algo/Sort.h"
#include "Core/PulseSystemLibrary.h"


bool UPulseDownloadTask::IsInitialized() const
{
	return TotalSize > 0 && Identifier.IsValid() && !Identifier.FileName.IsEmpty();
}

bool UPulseDownloadTask::IsComplete() const
{
	for (const UPulseDownloadChunk* Chunk : Chunks)
		if (!Chunk || !Chunk->IsCompleted())
			return false;
	return true;
}

int64 UPulseDownloadTask::GetTotalSize() const
{
	return IsInitialized() ? TotalSize : Identifier.SavedTotalSize;
}

int32 UPulseDownloadTask::GenerateChunks(const int64 ChunkSize)
{
	for (int i = 0; i < Chunks.Num(); ++i)
		if (Chunks[i] && Chunks[i]->IsActive())
			return -1;
	for (int i = Chunks.Num() - 1; i >= 0; i--)
		RemoveChunk(i);
	Chunks.Empty();

	// Get chunks from disk
	TArray<FString> ChunkPaths;
	FString fileName;
	FString extension;
	Identifier.FileName.Split(TEXT("."), &fileName, &extension, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString Path = FPaths::ProjectPersistentDownloadDir() + "/DownloadChunks/" + Identifier.Id.ToString();
	UPulseSystemLibrary::FileGetAllFilesInDirectory(Path, ChunkPaths, false, "");

	// Add Chunks
	int32 ChunkNum = FMath::CeilToInt((double)TotalSize / ChunkSize);
	for (int i = 0; i < ChunkNum; i++)
	{
		auto range = FInt64Vector2(ChunkSize * i, FMath::Min(ChunkSize * (i + 1), TotalSize) - 1);
		UPulseDownloadChunk* newChunk = NewObject<UPulseDownloadChunk>();
		newChunk->InitializeChunk(Path + "/" + Identifier.FileName, i, range.X, range.Y);
		const int32 pathIndex = ChunkPaths.IndexOfByKey(newChunk->ChunkPath);
		if (pathIndex != INDEX_NONE)
		{
			const auto size = newChunk->GetLocalSize();
			if (size <= newChunk->GetTargetSize())
			{
				ChunkPaths.RemoveAt(pathIndex);
			}
		}
		AddChunk(newChunk);
	}

	// Remove obsolete chunks
	for (int i = ChunkPaths.Num() - 1; i >= 0; i--)
	{
		UPulseSystemLibrary::FileDelete(ChunkPaths[i]);
	}
	return Chunks.Num();
}

void UPulseDownloadTask::StartChunksDownload(int32 MaxParallelChunks, const float TimeOut)
{
	ParallelChunkCount = FMath::Max(MaxParallelChunks, 1);
	int32 count = 0;
	int32 completed = 0;
	for (int i = 0; i < Chunks.Num(); ++i)
	{
		if (!Chunks[i])
			continue;
		if (Chunks[i]->IsCompleted())
		{
			completed++;
			continue;
		}
		if (Chunks[i]->IsActive())
		{
			count++;
			continue;
		}
		if (count >= ParallelChunkCount)
			break;
		if (Chunks[i]->StartChunk(Identifier.Url))
			count++;
	}
	if (completed > 0)
	{
		OnTaskUpdate.Broadcast(this);
	}
}

int64 UPulseDownloadTask::GetDownloadedSize()
{
	int64 size = 0;
	if (Chunks.Num() > 0)
	{
		for (const auto& chunk : Chunks)
		{
			if (!chunk)
				continue;
			size += (chunk->DownloadedSize + chunk->LocalSize);
		}
	}
	else
	{
		size = Identifier.SavedDownloadedSize;
	}
	return size;
}

void UPulseDownloadTask::GetDetailedDownloadedSize(TArray<int64>& OutChunkDownloadedSizes)
{
	OutChunkDownloadedSizes.Empty();
	for (const auto& chunk : Chunks)
	{
		if (!chunk)
			continue;
		OutChunkDownloadedSizes.Add(chunk->DownloadedSize + chunk->LocalSize);
	}
}

void UPulseDownloadTask::GetDetailedTotalSizes(TArray<int64>& OutChunkTotalSizes)
{
	OutChunkTotalSizes.Empty();
	for (const auto& chunk : Chunks)
	{
		if (!chunk)
			continue;
		OutChunkTotalSizes.Add(chunk->GetTargetSize());
	}
}

bool UPulseDownloadTask::StopChunk(UPulseDownloadChunk* Chunk)
{
	const int32 chunkIdx = Chunks.IndexOfByKey(Chunk);
	if (chunkIdx == INDEX_NONE)
		return false;
	if (!Chunks[chunkIdx])
		return false;
	return Chunks[chunkIdx]->CancelChunk();
}

bool UPulseDownloadTask::PauseTask()
{
	int32 Count = 0;
	for (int i = 0; i < Chunks.Num(); ++i)
	{
		if (!Chunks[i])
			continue;
		Count += Chunks[i]->CancelChunk() ? 1 : 0;
	}
	if (Count > 0)
	{
		bIsActionRequestOngoing = true;
		return true;
	}
	return false;
}

void UPulseDownloadTask::EmergencyHaltTask()
{
	for (int i = Chunks.Num() - 1; i >= 0; i--)
	{
		if (Chunks[i] && Chunks[i]->Request.IsValid())
		{
			Chunks[i]->Request->OnProcessRequestComplete().Unbind();
			Chunks[i]->Request->OnRequestProgress64().Unbind();
			Chunks[i]->Request->CancelRequest();
			Chunks[i]->Request.Reset();
		}
		RemoveChunk(i);
	}
	Chunks.Empty();
}

bool UPulseDownloadTask::CancelTask()
{
	int32 Count = 0;
	for (int i = 0; i < Chunks.Num(); ++i)
	{
		if (!Chunks[i])
			continue;
		Count += Chunks[i]->CancelChunk() ? 1 : 0;
	}
	if (Count > 0)
	{
		bIsActionRequestOngoing = true;
		return true;
	}
	return false;
}

bool UPulseDownloadTask::AddChunk(UPulseDownloadChunk* Chunk)
{
	if (!Chunk)
		return false;
	if (Chunks.Contains(Chunk))
		return false;
	Chunk->OnChunkCompleted.AddUObject(this, &UPulseDownloadTask::OnChunkCompleted);
	Chunk->OnChunkUpdate.AddUObject(this, &UPulseDownloadTask::OnChunkUpdated);
	Chunk->OnChunkFailed.AddUObject(this, &UPulseDownloadTask::OnChunkFailed);
	Chunks.Add(Chunk);
	return true;
}

bool UPulseDownloadTask::RemoveChunk(const int32 Index, bool DeleteFile)
{
	if (!Chunks.IsValidIndex(Index))
		return false;
	if (!Chunks[Index])
	{
		Chunks.RemoveAt(Index);
		return true;
	}
	Chunks[Index]->OnChunkCompleted.RemoveAll(this);
	Chunks[Index]->OnChunkUpdate.RemoveAll(this);
	Chunks[Index]->OnChunkFailed.RemoveAll(this);
	if (DeleteFile)
	{
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		PF.DeleteFile(*Chunks[Index]->ChunkPath);
		TArray<FString> ChunkPaths;
		FString chunkPath;
		FString chunkFileName;
		Chunks[Index]->ChunkPath.Split("/", &chunkPath, &chunkFileName, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
		UPulseSystemLibrary::FileGetAllFilesInDirectory(chunkPath, ChunkPaths, false, "");
		if (ChunkPaths.Num() <= 0)
			PF.DeleteDirectory(*chunkPath);
	}
	Chunks.RemoveAt(Index);
	return true;
}

void UPulseDownloadTask::DeleteChunkFiles()
{
	for (int i = Chunks.Num() - 1; i >= 0; i--)
		RemoveChunk(i, true);
}

void UPulseDownloadTask::OnChunkCompleted(UPulseDownloadChunk* Chunk)
{
	const int32 chunkIndex = Chunks.IndexOfByKey(Chunk);
	if (chunkIndex == INDEX_NONE)
		return;
	int32 count = 0;
	for (int i = 0; i < Chunks.Num(); ++i)
	{
		if (!Chunks[i])
			continue;
		if (Chunks[i]->IsCompleted() && !Chunks[i]->IsActive())
			count++;
	}
	if (count < Chunks.Num())
	{
		StartChunksDownload(ParallelChunkCount);
		return;
	}
	bIsActionRequestOngoing = false;
	//Write chunks to whole file
	Algo::Sort(Chunks, [](const TObjectPtr<UPulseDownloadChunk>& Chunk1, const TObjectPtr<UPulseDownloadChunk>& Chunk2)
	{
		return Chunk1 && Chunk2 && Chunk1->GetChunkIndex() < Chunk2->GetChunkIndex();
	});
	if (Chunks.Num() > 0)
	{
		TUniquePtr<IFileHandle> FileHandle;
		TArray<uint8> ChunkData;
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		// Trying to delete the file is it already exist
		PF.DeleteFile(*Identifier.GetFilePath());
		FileHandle.Reset(PF.OpenWrite(*Identifier.GetFilePath(), true));
		for (int i = 0; i < Chunks.Num(); ++i)
		{
			if (!Chunks[i])
				continue;
			ChunkData.Empty();
			if (!FFileHelper::LoadFileToArray(ChunkData, *Chunks[i]->ChunkPath))
				break;
			if (FileHandle.IsValid())
			{
				FileHandle->Write(ChunkData.GetData(), ChunkData.Num());
			}
			else if (Chunks[i]->GetChunkIndex() == 0)
			{
				FFileHelper::SaveArrayToFile(ChunkData, *Identifier.GetFilePath());
				FileHandle.Reset(PF.OpenWrite(*Identifier.GetFilePath(), true));
			}
		}
		FileHandle.Reset();
		// Delete chunks
		DeleteChunkFiles();
		// Reset
		Chunks.Empty();
	}
	OnTaskCompleted.Broadcast(this);
}

void UPulseDownloadTask::OnChunkUpdated(UPulseDownloadChunk* Chunk)
{
	OnTaskUpdate.Broadcast(this);
}

void UPulseDownloadTask::OnChunkFailed(UPulseDownloadChunk* Chunk)
{
	const int32 chunkIndex = Chunks.IndexOfByKey(Chunk);
	if (chunkIndex == INDEX_NONE)
		return;
	int32 count = 0;
	for (int i = 0; i < Chunks.Num(); ++i)
	{
		if (!Chunks[i])
			continue;
		if (!Chunks[i]->IsActive())
			count++;
	}
	if (count < Chunks.Num())
		return;
	bIsActionRequestOngoing = false;
	switch (DownloadState)
	{
	default:
		OnTaskFailed.Broadcast(this);
		break;
	case EPulseDownloadState::Paused:
		OnTaskPaused.Broadcast(this);
		break;
	case EPulseDownloadState::Cancelled:
		OnTaskCancelled.Broadcast(this);
		break;
	}
}
