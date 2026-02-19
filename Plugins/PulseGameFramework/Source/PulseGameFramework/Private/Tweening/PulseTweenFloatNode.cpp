// Copyright © by Tyni Boat. All Rights Reserved.


#include "Tweening/PulseTweenFloatNode.h"

#include "PulseGameFramework.h"


void UPulseTweenFloatNode::OnUpdate_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	float Value = 0;
	float Percentage = 0;
	if (!TweenManager)
		return;
	if (!TweenManager->GetTweenValues(TweenGUID, Value, Percentage))
		return;
	OnTweenUpdate.Broadcast(Value, Percentage);
}

void UPulseTweenFloatNode::OnStarted_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenStarted.Broadcast();
}

void UPulseTweenFloatNode::OnPaused_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenPaused.Broadcast();
}

void UPulseTweenFloatNode::OnResumed_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenResumed.Broadcast();
}

void UPulseTweenFloatNode::OnPingPong_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenPingPongApex.Broadcast();
}

void UPulseTweenFloatNode::OnLooped_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenLooped.Broadcast();
}

void UPulseTweenFloatNode::OnCompleted_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenCompleted.Broadcast();
	if (!SequenceGUID.IsValid())
		SetReadyToDestroy();
}

void UPulseTweenFloatNode::OnSequenceMoveNext_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(SequenceGUID))
		return;
	if (!TweenManager)
		return;
	auto old = TweenGUID;
	int32 index = -1;
	TweenManager->GetSequenceCurrentTweenID(SequenceGUID, TweenGUID, index);
	UE_LOG(LogPulseTweening, Log, TEXT("Node Tween UID from %s to %s"), *old.ToString(), *TweenGUID.ToString());
}

void UPulseTweenFloatNode::OnSequenceCompleted_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(SequenceGUID))
		return;
	OnSequenceCompleted.Broadcast();
	SetReadyToDestroy();
}

void UPulseTweenFloatNode::OnCancelled_Internal(TSet<FGuid> Set)
{
	if (!Set.Contains(TweenGUID))
		return;
	OnTweenCancelled.Broadcast();
	if (!SequenceGUID.IsValid())
		SetReadyToDestroy();
}

void UPulseTweenFloatNode::Activate()
{
	if (!Owner)
	{
		UE_LOG(LogPulseTweening, Error, TEXT("World Context is NULL"));
		SetReadyToDestroy();
		return;
	}
	if (TweenParams.Num() <= 0)
	{
		UE_LOG(LogPulseTweening, Error, TEXT("No Tween Params"));
		SetReadyToDestroy();
		return;
	}
	TweenManager = Owner->GetWorld()->GetSubsystem<UPulseTween>();
	if (!TweenManager)
	{
		UE_LOG(LogPulseTweening, Error, TEXT("Tween Manager SubSystem doesn't exist"));
		SetReadyToDestroy();
		return;
	}
	if (isSequence)
	{
		FGuid seqID = {};
		if (TweenManager->CreateNewSequence(this, seqID, TweenParams, LoopSeq))
		{
			if (seqID.IsValid())
			{
				SequenceGUID = seqID;
				int32 Index = -1;
				TweenManager->GetSequenceCurrentTweenID(SequenceGUID, TweenGUID, Index);
			}
		}
	}
	else
	{
		FPulseTweenInstance instance = FPulseTweenInstance(TweenParams[0]);
		auto Guid = UPulseTween::Tween(Owner, instance);
		if (Guid.IsValid())
			TweenGUID = Guid;
	}
	if (TweenGUID.IsValid())
	{
		TweenManager->OnTweenUpdateEvent.AddDynamic(this, &UPulseTweenFloatNode::OnUpdate_Internal);
		TweenManager->OnTweenStartedEvent.AddDynamic(this, &UPulseTweenFloatNode::OnStarted_Internal);
		TweenManager->OnTweenPausedEvent.AddDynamic(this, &UPulseTweenFloatNode::OnPaused_Internal);
		TweenManager->OnTweenResumedEvent.AddDynamic(this, &UPulseTweenFloatNode::OnResumed_Internal);
		TweenManager->OnTweenPingPongApexEvent.AddDynamic(this, &UPulseTweenFloatNode::OnPingPong_Internal);
		TweenManager->OnTweenLoopEvent.AddDynamic(this, &UPulseTweenFloatNode::OnLooped_Internal);
		TweenManager->OnTweenCompletedEvent.AddDynamic(this, &UPulseTweenFloatNode::OnCompleted_Internal);
		TweenManager->OnTweenCancelledEvent.AddDynamic(this, &UPulseTweenFloatNode::OnCancelled_Internal);
		if (SequenceGUID.IsValid())
		{
			TweenManager->OnTweenSequenceCompletedEvent.AddDynamic(this, &UPulseTweenFloatNode::OnSequenceCompleted_Internal);
			TweenManager->OnTweenSequenceMoveNextEvent.AddDynamic(this, &UPulseTweenFloatNode::OnSequenceMoveNext_Internal);
		}
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UPulseTweenFloatNode::BeginDestroy()
{
	if (TweenManager)
	{
		TweenManager->OnTweenUpdateEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnUpdate_Internal);
		TweenManager->OnTweenStartedEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnStarted_Internal);
		TweenManager->OnTweenPausedEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnPaused_Internal);
		TweenManager->OnTweenResumedEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnResumed_Internal);
		TweenManager->OnTweenPingPongApexEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnPingPong_Internal);
		TweenManager->OnTweenLoopEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnLooped_Internal);
		TweenManager->OnTweenCompletedEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnCompleted_Internal);
		TweenManager->OnTweenCancelledEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnCancelled_Internal);
		TweenManager->OnTweenSequenceCompletedEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnSequenceCompleted_Internal);
		TweenManager->OnTweenSequenceMoveNextEvent.RemoveDynamic(this, &UPulseTweenFloatNode::OnSequenceMoveNext_Internal);
	}
	Super::BeginDestroy();
}

void UPulseTweenFloatNode::Pause()
{
	if (!TweenManager)
		return;
	TweenManager->PauseTweenInstance(TweenGUID);
}

void UPulseTweenFloatNode::Resume()
{
	if (!TweenManager)
		return;
	TweenManager->ResumeTweenInstance(TweenGUID);
}

void UPulseTweenFloatNode::Restart()
{
	if (!TweenManager)
		return;
	if (isSequence)
		TweenManager->ResetSequence(SequenceGUID);
	else
		TweenManager->ResetTweenInstance(TweenGUID);
}

void UPulseTweenFloatNode::Stop()
{
	if (!TweenManager)
		return;
	if (isSequence)
		TweenManager->CancelSequence(SequenceGUID);
	else
		TweenManager->CancelTweenInstance(TweenGUID);
}

UPulseTweenFloatNode* UPulseTweenFloatNode::PulseTweenValue(UObject* WorldContext, const FTweenParams& TweenParameters)
{
	UPulseTweenFloatNode* BlueprintNode = NewObject<UPulseTweenFloatNode>();
	BlueprintNode->Owner = WorldContext;
	BlueprintNode->TweenParams.Add(TweenParameters);
	return BlueprintNode;
}

UPulseTweenFloatNode* UPulseTweenFloatNode::PulseTweenSequenceValue(UObject* WorldContext, const TArray<FTweenParams>& SequenceParameters, int32 SequenceLoop)
{
	UPulseTweenFloatNode* BlueprintNode = NewObject<UPulseTweenFloatNode>();
	BlueprintNode->Owner = WorldContext;
	BlueprintNode->LoopSeq = SequenceLoop;
	BlueprintNode->TweenParams = SequenceParameters;
	BlueprintNode->isSequence = true;
	return BlueprintNode;
}
