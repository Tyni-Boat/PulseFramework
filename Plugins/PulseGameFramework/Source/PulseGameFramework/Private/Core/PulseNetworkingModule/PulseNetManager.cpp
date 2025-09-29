// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseNetworkingModule/PulseNetManager.h"

#include "Chaos/ClusterUnionManager.h"
#include "Core/PulseCoreModule.h"
#include "Core/PulseModuleBase.h"
#include "Core/PulseSystemLibrary.h"
#include "GameFramework/GameStateBase.h"


void UPulseNetManager::OnWorldCreation_Internal(UWorld* World, FWorldInitializationValues WorldInitializationValues)
{
	if (!World)
		return;
	if (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::Inactive || World->WorldType == EWorldType::None)
		return;
	if (!(World->IsNetMode(NM_DedicatedServer) || World->IsNetMode(NM_ListenServer)))
		return;
	World->GetTimerManager().SetTimerForNextTick([&]()-> void
	{
		auto gs = UGameplayStatics::GetGameState(GetWorld());
		if (!gs)
			return;
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = gs;
		SpawnParams.Instigator = gs->GetInstigator();
		gs->GetWorld()->SpawnActor<APulseNetMgrActor>();
	});
}

ENetworkAuthorizationState UPulseNetManager::GetNetAuth() const
{
	return _netAuthorizationState;
}

ENetMode UPulseNetManager::GetNetMode() const
{
	if (_netActor)
		return _netActor->GetNetMode();
	return ENetMode::NM_MAX;
}

bool UPulseNetManager::HasAuthority() const
{
	if (_netActor)
		return _netActor->HasAuthority();
	return false;
}

bool UPulseNetManager::SetNetActorMgr(APulseNetMgrActor* NetComp, UPulseNetManager*& NetManager)
{
	if (!NetComp)
		return false;
	if (auto mgr = Get(NetComp))
	{
		if (mgr->_netActor != NetComp)
			if (mgr->_netActor != nullptr)
				return false;
		NetComp->bAlwaysRelevant = mgr->_bActorAlwaysRelevant;
		NetComp->SetNetUpdateFrequency(mgr->_netActorUpdateFrequency);
		NetComp->NetPriority = mgr->_netActorPriority;
		mgr->_netActor = NetComp;
		NetManager = mgr;
		return true;
	}
	return false;
}

void UPulseNetManager::HandleNetHistory()
{
	FReplicatedEntry entry;
	while (_pendingReplicatedValues.Dequeue(entry))
	{
		if (entry.FlagValue <= -1)
			RemoveReplicatedValue_Internal(entry.GTag(), true);
		else
			ReplicateValue_Internal(entry.GTag(), entry, true);
	}
	while (_pendingReliableRPCs.Dequeue(entry))
	{
		MakeRPCall_Internal(entry.GTag(), entry, true);
	}
}

bool UPulseNetManager::WantToTick() const
{
	return false;
}

bool UPulseNetManager::TickWhenPaused() const
{
	return false;
}

void UPulseNetManager::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	if (auto coreModule = Cast<UPulseCoreModule>(OwningModule))
	{
		if (auto config = coreModule->GetProjectConfig())
		{
			_bActorAlwaysRelevant = config->bNetworkManagerAlwaysRelevant;
			_netActorUpdateFrequency = config->NetUpdateFrequency;
			_netActorPriority = config->NetPriority;
		}
	}
	_worldInitDelegateHandle_Internal = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UPulseNetManager::OnWorldCreation_Internal);
}

void UPulseNetManager::DeinitializeSubModule()
{
	FWorldDelegates::OnPostWorldInitialization.Remove(_worldInitDelegateHandle_Internal);
	Super::DeinitializeSubModule();
}

void UPulseNetManager::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}

UPulseNetManager* UPulseNetManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;
	const auto gi = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!gi)
		return nullptr;
	const auto core = gi->GetSubsystem<UPulseCoreModule>();
	if (!core)
		return nullptr;
	return core->GetSubModule<UPulseNetManager>();
}

bool UPulseNetManager::SetNetAuthorization(const UObject* WorldContextObject, int32 NetAuthorization)
{
	if (auto mgr = Get(WorldContextObject))
	{
		const auto oldVal = mgr->_netAuthorizationState;
		mgr->_netAuthorizationState = static_cast<ENetworkAuthorizationState>(NetAuthorization);
		if (oldVal != mgr->_netAuthorizationState)
		{
			mgr->OnNetAuthorizationStateChanged.Broadcast(oldVal, mgr->_netAuthorizationState);
		}
		return true;
	}
	return false;
}

ENetworkAuthorizationState UPulseNetManager::GetNetAuthorization(const UObject* WorldContextObject)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->GetNetAuth();
	}
	return ENetworkAuthorizationState::None;
}

int32 UPulseNetManager::GetNetNetMode(const UObject* WorldContextObject)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return static_cast<int32>(mgr->GetNetMode());
	}
	return -1;
}

bool UPulseNetManager::GetNetHasAuthority(const UObject* WorldContextObject)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->HasAuthority();
	}
	return false;
}

bool UPulseNetManager::TryGetReplicatedValue(const UObject* WorldContextObject, FGameplayTag Tag, FReplicatedEntry& OutValue)
{
	if (auto mgr = Get(WorldContextObject))
	{
		if (auto netActor = mgr->_netActor)
		{
			if (netActor->GetReplicatedValue(Tag, OutValue))
				return true;
		}
	}
	return false;
}

bool UPulseNetManager::TryGetReplicatedValues(const UObject* WorldContextObject, FGameplayTag Tag, TArray<FReplicatedEntry>& OutValues)
{
	if (auto mgr = Get(WorldContextObject))
	{
		if (auto netActor = mgr->_netActor)
		{
			if (netActor->GetReplicatedValues(Tag, OutValues))
				return true;
		}
	}
	return false;
}

bool UPulseNetManager::ReplicateValue(const UObject* WorldContextObject, FGameplayTag Tag, FReplicatedEntry Value)
{
	if (!Tag.IsValid())
		return false;
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->ReplicateValue_Internal(Tag, Value);
	}
	return false;
}

bool UPulseNetManager::ReplicateValue_Internal(FGameplayTag Tag, FReplicatedEntry Value, bool fromHistory)
{
	if (!(static_cast<int32>(GetNetAuth()) & static_cast<int32>(ENetworkAuthorizationState::GameplayValues)))
		return false;
	if (auto netActor = _netActor)
	{
		if (!HasAuthority())
			return false;
		Value.Tag = Tag.GetTagName();
		if (!fromHistory && !_pendingReplicatedValues.IsEmpty())
		{
			_pendingReplicatedValues.Enqueue(Value);
			return true;
		}
		netActor->ReplicateValue(Value);
		return true;
	}
	else if (!fromHistory)
	{
		Value.Tag = Tag.GetTagName();
		_pendingReplicatedValues.Enqueue(Value);
		return true;
	}
	return false;
}

bool UPulseNetManager::RemoveReplicatedValue(const UObject* WorldContextObject, FGameplayTag Tag)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->RemoveReplicatedValue_Internal(Tag);
	}
	return false;
}

bool UPulseNetManager::RemoveReplicatedValue_Internal(FGameplayTag Tag, bool fromHistory)
{
	if (!(static_cast<int32>(GetNetAuth()) & static_cast<int32>(ENetworkAuthorizationState::GameplayValues)))
		return false;
	if (auto netActor = _netActor)
	{
		if (!HasAuthority())
			return false;
		if (!fromHistory && !_pendingReplicatedValues.IsEmpty())
		{
			FReplicatedEntry Entry;
			Entry.Tag = Tag.GetTagName();
			Entry.FlagValue = -1;
			_pendingReplicatedValues.Enqueue(Entry);
			return true;
		}
		netActor->RemoveReplicatedItem(Tag.GetTagName());
		return true;
	}
	else if (!fromHistory)
	{
		FReplicatedEntry Entry;
		Entry.Tag = Tag.GetTagName();
		Entry.FlagValue = -1;
		_pendingReplicatedValues.Enqueue(Entry);
		return true;
	}
	return false;
}

bool UPulseNetManager::MakeRPCall(const UObject* WorldContextObject, FGameplayTag Tag, FReplicatedEntry Value, bool Reliable)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->MakeRPCall_Internal(Tag, Value, Reliable);
	}
	return false;
}

bool UPulseNetManager::MakeRPCall_Internal(FGameplayTag Tag, FReplicatedEntry Value, bool Reliable, bool fromHistory)
{
	if (!(static_cast<int32>(GetNetAuth()) & static_cast<int32>(ENetworkAuthorizationState::GameplayValues)))
		return false;
	if (auto netActor = _netActor)
	{
		if (!HasAuthority())
			return false;
		Value.Tag = Tag.GetTagName();
		if (Reliable)
		{
			if (!fromHistory && !_pendingReliableRPCs.IsEmpty())
			{
				_pendingReliableRPCs.Enqueue(Value);
				return true;
			}
			netActor->ReliableServerRPC(Value);
		}
		else
			netActor->UnreliableServerRPC(Value);
		return true;
	}
	else if (Reliable && !fromHistory)
	{
		Value.Tag = Tag.GetTagName();
		_pendingReliableRPCs.Enqueue(Value);
		return true;
	}
	return false;
}
