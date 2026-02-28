// Copyright © by Tyni Boat. All Rights Reserved.


#include "Tweening/PulseTweenEventListener.h"

#include "Tweening/PulseTween.h"


// Sets default values for this component's properties
UPulseTweenEventListener::UPulseTweenEventListener()
{
	PrimaryComponentTick.bCanEverTick = false;
}


void UPulseTweenEventListener::PostInitProperties()
{
	Super::PostInitProperties();
	auto world = GetWorld();
	if (!world)
		return;
	if (auto tweenSubSystem = world->GetSubsystem<UPulseTween>())
	{
		tweenSubSystem->OnTweenUpdateEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenUpdateEvent_Func);
		tweenSubSystem->OnTweenStartedEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenStartedEvent_Func);
		tweenSubSystem->OnTweenPausedEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenPausedEvent_Func);
		tweenSubSystem->OnTweenResumedEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenResumedEvent_Func);
		tweenSubSystem->OnTweenCompletedEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenCompletedEvent_Func);
		tweenSubSystem->OnTweenCancelledEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenCancelledEvent_Func);
		tweenSubSystem->OnTweenPingPongApexEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenPingPongApexEvent_Func);
		tweenSubSystem->OnTweenLoopEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenLoopEvent_Func);
		tweenSubSystem->OnTweenSequenceMoveNextEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenSequenceMoveNextEvent_Func);
		tweenSubSystem->OnTweenSequenceCompletedEvent.AddDynamic(this, &UPulseTweenEventListener::OnTweenCompletedEvent_Func);
	}
}

void UPulseTweenEventListener::BeginDestroy()
{
	Super::BeginDestroy();
	auto world = GetWorld();
	if (!world)
		return;
	if (auto tweenSubSystem = world->GetSubsystem<UPulseTween>())
	{
		tweenSubSystem->OnTweenUpdateEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenUpdateEvent_Func);
		tweenSubSystem->OnTweenStartedEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenStartedEvent_Func);
		tweenSubSystem->OnTweenPausedEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenPausedEvent_Func);
		tweenSubSystem->OnTweenResumedEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenResumedEvent_Func);
		tweenSubSystem->OnTweenCompletedEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenCompletedEvent_Func);
		tweenSubSystem->OnTweenCancelledEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenCancelledEvent_Func);
		tweenSubSystem->OnTweenPingPongApexEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenPingPongApexEvent_Func);
		tweenSubSystem->OnTweenLoopEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenLoopEvent_Func);
		tweenSubSystem->OnTweenSequenceMoveNextEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenSequenceMoveNextEvent_Func);
		tweenSubSystem->OnTweenSequenceCompletedEvent.RemoveDynamic(this, &UPulseTweenEventListener::OnTweenCompletedEvent_Func);
	}
}

void UPulseTweenEventListener::OnTweenUpdateEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenUpdateEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenStartedEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenStartedEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenPausedEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenPausedEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenResumedEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenResumedEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenCompletedEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenCompletedEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenCancelledEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenCancelledEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenPingPongApexEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenPingPongApexEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenLoopEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenLoopEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenSequenceMoveNextEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenSequenceMoveNextEvent.Broadcast(TweenUIDSet);
}

void UPulseTweenEventListener::OnTweenSequenceCompletedEvent_Func(TSet<FGuid> TweenUIDSet)
{
	OnTweenSequenceCompletedEvent.Broadcast(TweenUIDSet);
}

