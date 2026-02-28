// Copyright © by Tyni Boat. All Rights Reserved.


#include "Tweening/PulseTween.h"

#include "PulseGameFramework.h"
#include "Kismet/GameplayStatics.h"
#include "Async/ParallelFor.h"
#include "Core/PulseSystemLibrary.h"


void UPulseTween::BeginDestroy()
{
	Super::BeginDestroy();
	_updatingSet.Empty();
	_startedSet.Empty();
	_pausedSet.Empty();
	_resumedSet.Empty();
	_completedSet.Empty();
	_cancelledSet.Empty();
	_pingPongApexSet.Empty();
	_loopSet.Empty();
	_sequenceCompletedSet.Empty();
	_sequenceChkSet.Empty();
	_sequenceMoveNextSet.Empty();
	_tweenInstanceIndexesMap.Empty();
	_tweenStatusChangeRequest.Empty();
	_tweenInstances.Empty();
}

void UPulseTween::Deinitialize()
{
	Super::Deinitialize();
}

TStatId UPulseTween::GetStatId() const
{
	return Super::GetStatID();
}

ETickableTickType UPulseTween::GetTickableTickType() const
{
	return IsInitialized() ? ETickableTickType::Always : ETickableTickType::Never;
}

void UPulseTween::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (auto config = GetProjectSettings())
	{
		_tweenCountMT = config->bUseMultiThreadedTween ? config->TweenCountToMultiThreadProcessing : -1;
	}
	UE_LOG(LogPulseTweening, Log, TEXT("Tweening sub-system initialized"));
}

bool UPulseTween::IsTickable() const
{
	return IsInitialized();
}

bool UPulseTween::IsTickableInEditor() const
{
	return false;
}

bool UPulseTween::IsTickableWhenPaused() const
{
	return true;
}

void UPulseTween::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick);
	Super::Tick(DeltaTime);
	if (!_world)
		_world = GetWorld();
	if (!_world)
		return;
	const bool isPaused = UGameplayStatics::IsGamePaused(_world);
	const float timeDilation = UGameplayStatics::GetGlobalTimeDilation(_world);
	//Reset arrays
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::ResetSets);
		_updatingSet.Reset();
		_startedSet.Reset();
		_pausedSet.Reset();
		_resumedSet.Reset();
		_completedSet.Reset();
		_cancelledSet.Reset();
		_pingPongApexSet.Reset();
		_loopSet.Reset();
		_sequenceCompletedSet.Reset();
		_sequenceChkSet.Reset();
		_sequenceMoveNextSet.Reset();
	}
	// Add new ones
	if (!_newTweensQueue.IsEmpty())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::AddNewTweens);
		FPulseTweenInstance newInstance = {};
		while (_newTweensQueue.Dequeue(newInstance))
		{
			UE_LOG(LogPulseTweening, Log, TEXT("Tween %s Activated"), *newInstance.Identifier.ToString());
			_tweenInstances.Add(newInstance);
			// Update index map
			for (int i = 0; i < _tweenInstances.Num(); i++)
			{
				if (_tweenInstanceIndexesMap.Contains(_tweenInstances[i].Identifier))
					_tweenInstanceIndexesMap[_tweenInstances[i].Identifier] = i;
				else
					_tweenInstanceIndexesMap.Add(_tweenInstances[i].Identifier, i);
			}
		}
	}
	// handle pause/resume/cancel requests
	if (!_tweenStatusChangeRequest.IsEmpty())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::ChangesRequests);
		for (const TPair<FGuid, int>& ChangeRequest : _tweenStatusChangeRequest)
		{
			if (!_tweenInstanceIndexesMap.Contains(ChangeRequest.Key))
				continue;
			const int32 index = _tweenInstanceIndexesMap[ChangeRequest.Key];
			if (!_tweenInstances.IsValidIndex(index))
				continue;
			// Cancel
			if (ChangeRequest.Value == 3)
			{
				UE_LOG(LogPulseTweening, Log, TEXT("Tween %s Cancelled"), *_tweenInstances[index].Identifier.ToString());
				_reusedIndexQueue.Enqueue(index);
				continue;
			}
			_tweenInstances[index].TweenIsPaused = ChangeRequest.Value == 1;
			UE_LOG(LogPulseTweening, Log, TEXT("Tween %s %s"), *_tweenInstances[index].Identifier.ToString(), *FString(ChangeRequest.Value == 1? "Paused" : "Resumed"));
			// Reset
			if (ChangeRequest.Value == 4)
			{
				UE_LOG(LogPulseTweening, Log, TEXT("Tween %s Reset"), *_tweenInstances[index].Identifier.ToString());
				_tweenInstances[index].Reset();
			}
		}
		_tweenStatusChangeRequest.Reset();
	}
	//Reset arrays
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::ResetArrays);
		_reusedIndexList.Reset();
	}
	//Update Tween instances
	if (_tweenInstances.Num() > 0)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::UpdateTweenInstances);
		FRWLock Lock;
		// Single Thread
		if (_tweenCountMT < _tweenInstances.Num())
		{
			for (int i = _tweenInstances.Num() - 1; i >= 0; i--)
			{
				// Remove completed
				if (_tweenInstances[i].IsComplete())
				{
					_reusedIndexQueue.Enqueue(i);
					continue;
				}
				// Update instances
				_tweenInstances[i].Update(DeltaTime, timeDilation, isPaused);
				//Per Status Set
				AddToStatusSet(_tweenInstances[i], Lock);
			}
		}
		// Multi thread
		else
		{
			ParallelFor(_tweenInstances.Num(), [&](int32 Index)
			{
				FPulseTweenInstance& V = _tweenInstances[Index];
				// Remove completed
				if (V.IsComplete())
				{
					_reusedIndexQueue.Enqueue(Index);
					return;
				}
				// Update instances
				V.Update(DeltaTime, timeDilation, isPaused);
				//Per Status Set
				AddToStatusSet(V, Lock);
			});
		}
	}
	// Remove instances
	if (!_reusedIndexQueue.IsEmpty())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::RemovesInstances);
		int32 index = INDEX_NONE;
		while (_reusedIndexQueue.Dequeue(index))
		{
			if (index != INDEX_NONE)
				_reusedIndexList.Add(index);
		}
		_reusedIndexList.Sort();
		for (int32 i = _reusedIndexList.Num() - 1; i >= 0; i--)
		{
			index = _reusedIndexList[i];
			if (!_tweenInstances.IsValidIndex(index))
				continue;
			const FGuid guid = _tweenInstances[index].Identifier;
			if (_tweenInstanceIndexesMap.Contains(guid))
				_tweenInstanceIndexesMap.Remove(guid);
			if (!_unUsedGUIDs.Contains(guid))
				_unUsedGUIDs.Add(guid);
			if (_tweenInstances[index].IsComplete())
			{
				_sequenceChkSet.Add(guid);
			}
			else
			{
				_cancelledSet.Add(guid);
				_sequenceChkSet.Add(guid);
			}
			UE_LOG(LogPulseTweening, Log, TEXT("Tween %s %s"), *_tweenInstances[index].Identifier.ToString(),
			       *FString(_tweenInstances[index].IsComplete()? "Completed" : "Deleted"));
			_tweenInstances.RemoveAt(index);
		}
		// Update index map
		for (int i = 0; i < _tweenInstances.Num(); i++)
		{
			if (_tweenInstanceIndexesMap.Contains(_tweenInstances[i].Identifier))
				_tweenInstanceIndexesMap[_tweenInstances[i].Identifier] = i;
			else
				_tweenInstanceIndexesMap.Add(_tweenInstances[i].Identifier, i);
		}
	}
	// Broadcast Events
	if (MustTriggerEvents())
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Tick::BroadCasting);
		if (_updatingSet.Num() > 0)
			OnTweenUpdateEvent.Broadcast(_updatingSet);
		if (_startedSet.Num() > 0)
			OnTweenStartedEvent.Broadcast(_startedSet);
		if (_pausedSet.Num() > 0)
			OnTweenPausedEvent.Broadcast(_pausedSet);
		if (_resumedSet.Num() > 0)
			OnTweenResumedEvent.Broadcast(_resumedSet);
		if (_completedSet.Num() > 0)
			OnTweenCompletedEvent.Broadcast(_completedSet);
		if (_cancelledSet.Num() > 0)
			OnTweenCancelledEvent.Broadcast(_cancelledSet);
		if (_pingPongApexSet.Num() > 0)
			OnTweenPingPongApexEvent.Broadcast(_pingPongApexSet);
		if (_loopSet.Num() > 0)
			OnTweenLoopEvent.Broadcast(_loopSet);
		if (_sequenceChkSet.Num() > 0)
		{
			// Start next instance in sequence
			for (int i = _ActiveSequences.Num() - 1; i >= 0; i--)
			{
				int32 pastIndex = _ActiveSequences[i].CurrentIndex;
				if (_ActiveSequences[i].SequenceNext(_sequenceChkSet))
				{
					FPulseTweenInstance Instance;
					if (_ActiveSequences[i].GetCurrentInstance(Instance))
					{
						UE_LOG(LogPulseTweening, Log, TEXT("Sequence %s Moved Next to Index %d from %d"), *_ActiveSequences[i].TweenSequenceID.ToString(),
						       _ActiveSequences[i].CurrentIndex, pastIndex);
						_sequenceMoveNextSet.Add(_ActiveSequences[i].TweenSequenceID);
						if (_tweenInstanceIndexesMap.Contains(Instance.Identifier))
						{
							if (!_tweenInstances.IsValidIndex(_tweenInstanceIndexesMap[Instance.Identifier]))
								continue;
							_tweenInstances[_tweenInstanceIndexesMap[Instance.Identifier]].Reset();
						}
						else
						{
							_newTweensQueue.Enqueue(Instance);
						}
					}
				}
				else
				{
					_sequenceCompletedSet.Add(_ActiveSequences[i].TweenSequenceID);
					_ActiveSequences.RemoveAt(i);
				}
			}
		}
		if (_sequenceCompletedSet.Num() > 0)
			OnTweenSequenceCompletedEvent.Broadcast(_sequenceCompletedSet);
		if (_sequenceMoveNextSet.Num() > 0)
			OnTweenSequenceMoveNextEvent.Broadcast(_sequenceMoveNextSet);
	}
}

bool UPulseTween::GetTweenInstance(const FGuid& Guid, FPulseTweenInstance& OutInstance)
{
	if (!_tweenInstanceIndexesMap.Contains(Guid))
		return false;
	const int32 index = _tweenInstanceIndexesMap[Guid];
	if (!_tweenInstances.IsValidIndex(index))
		return false;
	OutInstance = _tweenInstances[index];
	return true;
}

bool UPulseTween::PauseTweenInstance(const FGuid& Guid)
{
	if (!_tweenInstanceIndexesMap.Contains(Guid))
		return false;
	if (_tweenStatusChangeRequest.Contains(Guid))
	{
		if (_tweenStatusChangeRequest[Guid] == 3 || _tweenStatusChangeRequest[Guid] == 4)
			return false;
		_tweenStatusChangeRequest[Guid] = 1;
	}
	else
	{
		_tweenStatusChangeRequest.Add(Guid, 1);
	}
	return true;
}

bool UPulseTween::ResumeTweenInstance(const FGuid& Guid)
{
	if (!_tweenInstanceIndexesMap.Contains(Guid))
		return false;
	if (_tweenStatusChangeRequest.Contains(Guid))
	{
		if (_tweenStatusChangeRequest[Guid] == 3 || _tweenStatusChangeRequest[Guid] == 4)
			return false;
		_tweenStatusChangeRequest[Guid] = 2;
	}
	else
	{
		_tweenStatusChangeRequest.Add(Guid, 2);
	}
	return true;
}

bool UPulseTween::ResetTweenInstance(const FGuid& Guid)
{
	if (!_tweenInstanceIndexesMap.Contains(Guid))
		return false;
	if (_tweenStatusChangeRequest.Contains(Guid))
	{
		if (_tweenStatusChangeRequest[Guid] == 3)
			return false;
		_tweenStatusChangeRequest[Guid] = 4;
	}
	else
	{
		_tweenStatusChangeRequest.Add(Guid, 4);
	}
	return true;
}

bool UPulseTween::ResetSequence(const FGuid& Guid)
{
	const int32 indexOf = _ActiveSequences.IndexOfByPredicate([Guid](const FPulseTweenSequence& sequence)-> bool
	{
		return sequence.TweenSequenceID == Guid;
	});
	if (indexOf == INDEX_NONE)
		return false;
	FPulseTweenInstance Instance = {};
	if (_ActiveSequences[indexOf].GetCurrentInstance(Instance))
		CancelTweenInstance(Instance.Identifier);
	Instance = {};
	_ActiveSequences[indexOf].CurrentIndex = 0;
	if (_ActiveSequences[indexOf].GetCurrentInstance(Instance))
	{
		if (_tweenInstanceIndexesMap.Contains(Instance.Identifier))
		{
			if (!_tweenInstances.IsValidIndex(_tweenInstanceIndexesMap[Instance.Identifier]))
				return false;
			ResetTweenInstance(Instance.Identifier);
			_ActiveSequences[indexOf].wasRestarted = true;
		}
		else
		{
			_newTweensQueue.Enqueue(Instance);
		}
		return true;
	}
	return false;
}

bool UPulseTween::CancelTweenInstance(const FGuid& Guid)
{
	if (!_tweenInstanceIndexesMap.Contains(Guid))
		return false;
	if (_tweenStatusChangeRequest.Contains(Guid))
	{
		_tweenStatusChangeRequest[Guid] = 3;
	}
	else
	{
		_tweenStatusChangeRequest.Add(Guid, 3);
	}
	return true;
}

bool UPulseTween::CancelSequence(const FGuid& Guid)
{
	const int32 indexOf = _ActiveSequences.IndexOfByPredicate([Guid](const FPulseTweenSequence& sequence)-> bool
	{
		return sequence.TweenSequenceID == Guid;
	});
	if (indexOf == INDEX_NONE)
		return false;
	FPulseTweenInstance Instance = {};
	if (_ActiveSequences[indexOf].GetCurrentInstance(Instance))
		CancelTweenInstance(Instance.Identifier);
	_ActiveSequences[indexOf].CurrentIndex = -1;
	return true;
}

bool UPulseTween::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UPulseTween::AddToStatusSet(const FPulseTweenInstance& tweenInstance, FRWLock& Lock)
{
	switch (tweenInstance.Status)
	{
	default: break;
	case EPulseTweenStatus::Updating:
		{
			FWriteScopeLock W(Lock);
			_updatingSet.Add(tweenInstance.Identifier);
			return true;
		}
		break;
	case EPulseTweenStatus::JustStarted:
		{
			FWriteScopeLock W(Lock);
			_startedSet.Add(tweenInstance.Identifier);
			UE_LOG(LogPulseTweening, Log, TEXT("Tween %s Started"), *tweenInstance.Identifier.ToString());
			return true;
		}
		break;
	case EPulseTweenStatus::JustPaused:
		{
			FWriteScopeLock W(Lock);
			_pausedSet.Add(tweenInstance.Identifier);
			return true;
		}
		break;
	case EPulseTweenStatus::JustResumed:
		{
			FWriteScopeLock W(Lock);
			_resumedSet.Add(tweenInstance.Identifier);
			return true;
		}
		break;
	case EPulseTweenStatus::JustReachedPingPongApex:
		{
			FWriteScopeLock W(Lock);
			_pingPongApexSet.Add(tweenInstance.Identifier);
			return true;
		}
		break;
	case EPulseTweenStatus::JustLooped:
		{
			FWriteScopeLock W(Lock);
			_loopSet.Add(tweenInstance.Identifier);
			return true;
		}
		break;
	case EPulseTweenStatus::JustCompleted:
		{
			FWriteScopeLock W(Lock);
			_completedSet.Add(tweenInstance.Identifier);
			return true;
		}
		break;
	}
	return false;
}

bool UPulseTween::MustTriggerEvents() const
{
	return _updatingSet.Num() > 0 ||
		_startedSet.Num() > 0 ||
		_pausedSet.Num() > 0 ||
		_resumedSet.Num() > 0 ||
		_pingPongApexSet.Num() > 0 ||
		_completedSet.Num() > 0 ||
		_loopSet.Num() > 0 ||
		_sequenceChkSet.Num() > 0 ||
		_sequenceMoveNextSet.Num() > 0 ||
		_sequenceCompletedSet.Num() > 0;
}

FGuid UPulseTween::GetTweenNewGuid()
{
	if (_unUsedGUIDs.Num() > 0)
		return _unUsedGUIDs.Pop();
	return FGuid::NewGuid();
}

bool UPulseTween::TweenParamsEquals(const FTweenParams& Params1, const FTweenParams& Params2)
{
	return Params1 == Params2;
}

bool UPulseTween::TweenParamsNotEquals(const FTweenParams& Params1, const FTweenParams& Params2)
{
	return !TweenParamsEquals(Params1, Params2);
}

void UPulseTween::DebugTween(FLinearColor Color)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::Debug);
	if (_tweenInstances.Num() <= 25)
	{
		if (_tweenInstances.Num() > 0)
		{
			for (int i = _tweenInstances.Num() - 1; i >= 0; i--)
			{
				const auto ins = _tweenInstances[i];
				const FGuid guid = ins.Identifier;
				UKismetSystemLibrary::PrintString(this, ins.ToString(), true, true, Color, 0, FName(guid.ToString()));
			}
		}
		else
		{
			UKismetSystemLibrary::PrintString(
				this, FString::Printf(TEXT("No Active Tween")), true, true, Color, 0, "ManyActivesTween");
		}
	}
	else
	{
		UKismetSystemLibrary::PrintString(
			this, FString::Printf(TEXT("%d Actives Tween"), _tweenInstances.Num()), true, true, Color, 0, "ManyActivesTween");
	}
}

bool UPulseTween::IsActiveTween(const FGuid& TweenGUID) const
{
	int32 index = _tweenInstances.IndexOfByPredicate([TweenGUID](const FPulseTweenInstance& Tween)-> bool { return Tween.Identifier == TweenGUID; });
	return index != INDEX_NONE;
}

bool UPulseTween::GetTweenValues(const FGuid& TweenGUID, float& OutTweenValue, float& OutTweenPercentage)
{
	FPulseTweenInstance tweenInstance = {};
	const bool success = GetTweenInstance(TweenGUID, tweenInstance);
	if (success)
	{
		OutTweenValue = tweenInstance.GetValue();
		OutTweenPercentage = tweenInstance.GetTotalDuration() > 0 ? tweenInstance.TotalTime / tweenInstance.GetTotalDuration() : 0.0f;
		return true;
	}
	return false;
}

bool UPulseTween::GetTweenSequenceID(const FGuid& TweenGUID, FGuid& OutSequenceID)
{
	const int32 indexOf = _ActiveSequences.IndexOfByPredicate([TweenGUID](const FPulseTweenSequence& sequence)-> bool
	{
		return sequence.ContainsTweenAtIndex(TweenGUID) != INDEX_NONE;
	});
	if (indexOf == INDEX_NONE)
		return false;
	OutSequenceID = _ActiveSequences[indexOf].TweenSequenceID;
	return true;
}

bool UPulseTween::GetSequenceCurrentTweenID(const FGuid& SequenceID, FGuid& OutTweenID, int32& OutTweenIndex)
{
	const int32 indexOf = _ActiveSequences.IndexOfByPredicate([SequenceID](const FPulseTweenSequence& sequence)-> bool { return sequence.TweenSequenceID == SequenceID; });
	if (indexOf == INDEX_NONE)
		return false;
	FPulseTweenInstance tweenInstance = {};
	if (!_ActiveSequences[indexOf].GetCurrentInstance(tweenInstance))
		return false;
	OutTweenID = tweenInstance.Identifier;
	OutTweenIndex = _ActiveSequences[indexOf].InstanceIndex(OutTweenID);
	return true;
}

bool UPulseTween::GetSequenceTweenCount(const FGuid& SequenceID, int32& OutTweenCount)
{
	const int32 indexOf = _ActiveSequences.IndexOfByPredicate([SequenceID](const FPulseTweenSequence& sequence)-> bool { return sequence.TweenSequenceID == SequenceID; });
	if (indexOf == INDEX_NONE)
		return false;
	OutTweenCount = _ActiveSequences[indexOf].TweenSequenceInstances.Num();
	return true;
}

bool UPulseTween::GetTweenSequenceValues(const FGuid& SequenceGUID, float& OutTweenValue, float& OutTweenPercentage, float& OutOverallPercentage)
{
	const int32 indexOf = _ActiveSequences.IndexOfByPredicate([SequenceGUID](const FPulseTweenSequence& sequence)-> bool { return sequence.TweenSequenceID == SequenceGUID; });
	if (indexOf == INDEX_NONE)
		return false;
	FPulseTweenInstance tweenInstance = {};
	if (!_ActiveSequences[indexOf].GetCurrentInstance(tweenInstance))
		return false;
	if (!GetTweenInstance(tweenInstance.Identifier, tweenInstance))
		return false;
	OutTweenValue = tweenInstance.GetValue();
	OutTweenPercentage = tweenInstance.GetTotalDuration() > 0 ? tweenInstance.TotalTime / tweenInstance.GetTotalDuration() : 0.0f;
	float unit = 1.0f / _ActiveSequences[indexOf].TweenSequenceInstances.Num();
	OutOverallPercentage = UPulseSystemLibrary::ArrayIndexPercentage(_ActiveSequences[indexOf].TweenSequenceInstances, _ActiveSequences[indexOf].CurrentIndex, false) + (unit *
		OutTweenPercentage);
	return true;
}

bool UPulseTween::CreateNewTween(const UObject* Owner, FGuid& OutTweenID, FTweenParams TweenParams, bool bAttachToOwner)
{
	FPulseTweenInstance instance = bAttachToOwner? FPulseTweenInstance(Owner, TweenParams) : FPulseTweenInstance(TweenParams);
	auto tweenID = Tween(Owner, instance);
	if (tweenID.IsValid())
	{
		OutTweenID = tweenID;
		return true;
	}
	return false;
}

bool UPulseTween::CreateNewSequence(const UObject* Owner, FGuid& OutSequenceID, const TArray<FTweenParams>& TweenParams, int32 SequenceLoops, bool bAttachToOwner)
{
	if (TweenParams.Num() <= 0)
		return false;
	FPulseTweenSequence tweenSequence = {};
	tweenSequence.LoopCount = SequenceLoops;
	tweenSequence.TweenSequenceID = GetTweenNewGuid();
	tweenSequence.Reset();
	for (int32 i = 0; i < TweenParams.Num(); i++)
	{
		FPulseTweenInstance instance = bAttachToOwner? FPulseTweenInstance(Owner, TweenParams[i]) : FPulseTweenInstance(TweenParams[i]);
		instance.Identifier = GetTweenNewGuid();
		tweenSequence.TweenSequenceInstances.Add(instance);
	}
	_newTweensQueue.Enqueue(tweenSequence.TweenSequenceInstances[0]);
	_ActiveSequences.Add(tweenSequence);
	OutSequenceID = tweenSequence.TweenSequenceID;
	return true;
}

FGuid UPulseTween::Tween(const UObject* Owner, const FPulseTweenInstance& TweenInstance)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(PulseTween::StartNewTween);
	if (!Owner)
		return {};
	const auto world = Owner->GetWorld();
	if (!world)
		return {};
	auto tweenManager = world->GetSubsystem<UPulseTween>();
	if (!tweenManager)
		return {};
	FPulseTweenInstance instance = TweenInstance;
	instance.Identifier = tweenManager->GetTweenNewGuid();
	tweenManager->_newTweensQueue.Enqueue(instance);
	return instance.Identifier;
}


