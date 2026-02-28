// Copyright © by Tyni Boat. All Rights Reserved.


#include "DownloadManager/PulseDownloadEventListener.h"

#include "DownloadManager/PulseDownloader.h"


// Sets default values for this component's properties
UPulseDownloadEventListener::UPulseDownloadEventListener()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPulseDownloadEventListener::PostInitProperties()
{
	Super::PostInitProperties();
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

void UPulseDownloadEventListener::BeginDestroy()
{
	Super::BeginDestroy();
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

