// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "UObject/Object.h"
#include "PulseDownloadChunk.generated.h"



class UPulseDownloadChunk;
DECLARE_MULTICAST_DELEGATE_OneParam(FDownloadChunkEventDelegate, UPulseDownloadChunk* Chunk);


/**
 * Represent a part of an ongoing download.
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseDownloadChunk : public UObject
{
	GENERATED_BODY()

public:
	
	FDownloadChunkEventDelegate OnChunkUpdate;
	FDownloadChunkEventDelegate OnChunkCompleted;
	FDownloadChunkEventDelegate OnChunkFailed;

	UPROPERTY()
	FString ChunkPath = "";

	UPROPERTY()
	int64 FileStartByte = 0;

	UPROPERTY()
	int64 FileEndByte = 0;

	UPROPERTY()
	int64 DownloadedSize = 0;

	UPROPERTY()
	int64 LocalSize = 0;

	TSharedPtr<IHttpRequest> Request;

	
	bool IsValid() const;
	int32 GetChunkIndex() const;
	bool IsCompleted() const;
	// -1 if the file doesn't exist
	int64 GetLocalSize() const;
	bool IsActive() const;
	FInt64Vector2 GetActiveRange() const;
	int64 GetTargetSize() const;
	void InitializeChunk(const FString& LocalPath, int32 ChunkIdx, int64 From, int64 To);
	bool StartChunk(const FString& Url);
	bool CancelChunk();
	
	void OnChunkUpdateCallback(FHttpRequestPtr Req, uint64 BytesSent, uint64 BytesReceived);
	void OnChunkCompletedCallback(TSharedPtr<IHttpRequest> Req, TSharedPtr<IHttpResponse> Response, bool success);

	bool operator==(const UPulseDownloadChunk* Other) const
	{
		if (Other == nullptr)
			return false;
		return ChunkPath == Other->ChunkPath && FileStartByte == Other->FileStartByte && FileEndByte == Other->FileEndByte;
	}
	
	static TSharedRef<IHttpRequest> MakeDownloadRequest(const FString& URL, FInt64Vector2 DownloadRange = FInt64Vector2(-1));
};
