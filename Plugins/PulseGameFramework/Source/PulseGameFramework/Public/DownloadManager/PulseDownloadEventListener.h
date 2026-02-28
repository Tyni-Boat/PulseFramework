// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseDownloadTypes.h"
#include "Components/ActorComponent.h"
#include "PulseDownloadEventListener.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PULSEGAMEFRAMEWORK_API UPulseDownloadEventListener : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPulseDownloadEventListener();	
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

protected:

	UFUNCTION() void OnDownloadComplete_Func(const FGuid& DownloadId);
	UFUNCTION() void OnDownloadPaused_Func(const FGuid& DownloadId);
	UFUNCTION() void OnDownloadReadyToStart_Func(const FGuid& DownloadId);
	UFUNCTION() void OnDownloadQueued_Func(const FGuid& DownloadId);
	UFUNCTION() void OnDownloadResumedOrStarted_Func(const FGuid& DownloadId);
	UFUNCTION() void OnDownloadOnGoing_Func(const FGuid& DownloadId);
	UFUNCTION() void OnDownloadCancelled_Func(const FGuid& DownloadId);

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
};
