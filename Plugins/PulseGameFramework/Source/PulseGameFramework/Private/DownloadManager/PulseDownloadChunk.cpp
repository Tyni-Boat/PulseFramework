// Copyright © by Tyni Boat. All Rights Reserved.


#include "DownloadManager/PulseDownloadChunk.h"

#include "HttpModule.h"
#include "PulseGameFramework.h"
#include "Core/PulseSystemLibrary.h"
#include "Interfaces/IHttpResponse.h"


bool UPulseDownloadChunk::IsValid() const
{
	return !ChunkPath.IsEmpty() && FileEndByte > 0 && FileEndByte > GetActiveRange().X;
}

int32 UPulseDownloadChunk::GetChunkIndex() const
{
	FString Path;
	FString IndexStr;
	ChunkPath.Split(TEXT("_"), &Path, &IndexStr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	if (!IndexStr.IsNumeric())
		return -1;
	return FCString::Atoi(*IndexStr);
}

bool UPulseDownloadChunk::IsCompleted() const
{
	return GetLocalSize() == GetTargetSize();
}

int64 UPulseDownloadChunk::GetLocalSize() const
{
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	return PF.FileSize(*ChunkPath);
}

bool UPulseDownloadChunk::IsActive() const
{
	return IsValid() && Request.IsValid();
}

FInt64Vector2 UPulseDownloadChunk::GetActiveRange() const
{
	return FInt64Vector2(FileStartByte + LocalSize, FileEndByte);
}

int64 UPulseDownloadChunk::GetTargetSize() const
{
	return FileEndByte - FileStartByte + 1;
}

void UPulseDownloadChunk::InitializeChunk(const FString& LocalPath, int32 ChunkIdx, int64 From, int64 To)
{
	FString Path;
	FString extension;
	LocalPath.Split(TEXT("."), &Path, &extension, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	ChunkPath = FString::Printf(TEXT("%s_%d"), *Path, ChunkIdx);
	FileStartByte = From;
	FileEndByte = To;
	DownloadedSize = 0;
	LocalSize = FMath::Max(GetLocalSize(), 0);
	Request.Reset();
}

bool UPulseDownloadChunk::StartChunk(const FString& Url)
{
	if (!IsValid())
		return false;
	if (IsActive())
		return false;
	if (IsCompleted())
		return false;
	Request = MakeDownloadRequest(Url, GetActiveRange());
	if (!Request->OnRequestProgress64().IsBound())
	{
		Request->OnRequestProgress64().BindUObject(this, &UPulseDownloadChunk::OnChunkUpdateCallback);
	}
	if (!Request->OnProcessRequestComplete().IsBound())
	{
		Request->OnProcessRequestComplete().BindUObject(this, &UPulseDownloadChunk::OnChunkCompletedCallback);
	}
	Request->ProcessRequest();
	return true;
}

bool UPulseDownloadChunk::CancelChunk()
{
	if (Request.IsValid())
		Request->CancelRequest();
	else
		OnChunkFailed.Broadcast(this);
	return true;
}

void UPulseDownloadChunk::OnChunkUpdateCallback(FHttpRequestPtr Req, uint64 BytesSent, uint64 BytesReceived)
{
	DownloadedSize = BytesReceived;
	OnChunkUpdate.Broadcast(this);
}

void UPulseDownloadChunk::OnChunkCompletedCallback(TSharedPtr<IHttpRequest> Req, TSharedPtr<IHttpResponse> Response, bool success)
{
	TArray<uint8> Data = Response ? Response->GetContent() : TArray<uint8>();
	if (Data.Num() > 0)
	{
		TUniquePtr<IFileHandle> FileHandle;
		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		FileHandle.Reset(PF.OpenWrite(*ChunkPath, true));
		if (FileHandle.IsValid())
			FileHandle->Write(Data.GetData(), Data.Num());
		else
			FFileHelper::SaveArrayToFile(Data, *ChunkPath);
		FileHandle.Reset();
	}
	Request->OnRequestProgress64().Unbind();
	Request->OnProcessRequestComplete().Unbind();
	Request.Reset();
	const auto chunkIndex = GetChunkIndex();
	const auto diskSize = GetLocalSize();
	const bool completed = IsCompleted();
	UE_LOG(LogPulseDownloader, Log, TEXT("Chunk %d Download %s: %s/%s ; %s On disk, %s Response (Chunk path: %s)"), chunkIndex,
	       *FString(completed? TEXT("completed") : TEXT("partially completed")), *UPulseSystemLibrary::FileSizeToString(DownloadedSize + LocalSize),
	       *UPulseSystemLibrary::FileSizeToString(GetTargetSize()),
	       *UPulseSystemLibrary::FileSizeToString(diskSize), *UPulseSystemLibrary::FileSizeToString(sizeof(uint8) * Data.Num()),
	       *ChunkPath);
	if (completed)
		OnChunkCompleted.Broadcast(this);
	else
		OnChunkFailed.Broadcast(this);
}

TSharedRef<IHttpRequest> UPulseDownloadChunk::MakeDownloadRequest(const FString& URL, FInt64Vector2 DownloadRange)
{
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb(TEXT("GET"));
	if (DownloadRange.X >= 0 && DownloadRange.X < DownloadRange.Y)
	{
		const FString RangeHeaderValue = FString::Format(TEXT("bytes={0}-{1}"), {DownloadRange.X, DownloadRange.Y});
		Request->SetHeader(TEXT("Range"), RangeHeaderValue);
	}
	return Request;
}
