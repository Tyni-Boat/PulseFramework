// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PulseDownloadTypes.generated.h"


UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = true, BitmaskEnum = "/Script/PulseGameFramework.EDownloadState"))
enum class EPulseDownloadState : uint8
{
	None = 0 UMETA(Hidden),
	Queued = 1 << 0 UMETA(ToolTip = "In Queue and waiting for download infos from server"),
	ReadyToDownload = 1 << 1 UMETA(ToolTip = "In Queue and ready to be downloaded"),
	Downloading = 1 << 2 UMETA(ToolTip = "Actively downloading"),
	Paused = 1 << 3 UMETA(ToolTip = "Set in paused state"),
	Completed = 1 << 4 UMETA(ToolTip = "Completed successfully"),
	Failed = 1 << 5 UMETA(ToolTip = "Completed with failure"),
	Cancelled = 1 << 6 UMETA(ToolTip = "Cancelled by user"),
};
ENUM_CLASS_FLAGS(EPulseDownloadState);

USTRUCT(BlueprintType)
struct FDownloadIdentifier
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PulseCore|Download Manager")
	FGuid Id;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PulseCore|Download Manager")
	FString Url;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PulseCore|Download Manager")
	FString Directory;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PulseCore|Download Manager")
	FString FileName;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "PulseCore|Download Manager")
	FDateTime StartDate = FDateTime::MinValue();

	// The state of the download identifier when saved
	UPROPERTY()
	EPulseDownloadState SavedState = EPulseDownloadState::None;
	UPROPERTY()
	int64 SavedTotalSize = 0;
	UPROPERTY()
	int64 SavedDownloadedSize = 0;

	bool IsValid() const
	{
		return Id.IsValid() && !Url.IsEmpty() && !Directory.IsEmpty();
	}

	FString GetFilePath() const
	{
		return FString::Printf(TEXT("%s/%s"), *Directory, *FileName);
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("Id:%s; File Name:%s; Url:%s, Directory:%s"), *Id.ToString(), *FileName, *Url, *Directory);
	}

	bool operator==(const FDownloadIdentifier& rhs) const
	{
		return Id == rhs.Id && Url == rhs.Url;
	}
};


USTRUCT()
struct FPulseDownloadSaved
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FDownloadIdentifier> DownloadHistory;
};

UCLASS()
class USavedDownload : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPulseDownloadSaved PulseDownloadSaved;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPulseDownloadDelegateEvent, const FGuid&, DownloadId);
