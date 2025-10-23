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

ENetRole UPulseNetManager::GetNetRole() const
{
	if (_netActor)
		return _netActor->GetLocalRole();
	return ROLE_None;
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
			RemoveReplicatedValue_Internal(entry.Tag, nullptr, true);
		else
			ReplicateValue_Internal(entry.Tag, entry, true);
	}
	while (_pendingReliableRPCs.Dequeue(entry))
	{
		MakeRPCall_Internal(entry.Tag, entry, true);
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

bool UPulseNetManager::TryGetReplicatedValues(const UObject* WorldContextObject, FName Tag, TArray<FReplicatedEntry>& OutValues, UObject* ForObj)
{
	if (auto mgr = Get(WorldContextObject))
	{
		if (auto netActor = mgr->_netActor)
		{
			if (netActor->GetReplicatedValues(Tag, OutValues, ForObj))
				return true;
		}
	}
	return false;
}

ENetworkOperationResult UPulseNetManager::ReplicateValue(const UObject* WorldContextObject, FName Tag, FReplicatedEntry Value)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->ReplicateValue_Internal(Tag, Value);
	}
	return ENetworkOperationResult::UnknowError;
}

ENetworkOperationResult UPulseNetManager::ReplicateValue_Internal(FName Tag, FReplicatedEntry Value, bool fromHistory)
{
	if (!(static_cast<int32>(GetNetAuth()) & static_cast<int32>(ENetworkAuthorizationState::GameplayValues)))
		return ENetworkOperationResult::MissingAuthorisation;
	if (auto netActor = _netActor)
	{
		if (!HasAuthority())
			return ENetworkOperationResult::NoAuthority;
		Value.Tag = Tag;
		if (!fromHistory && !_pendingReplicatedValues.IsEmpty())
		{
			_pendingReplicatedValues.Enqueue(Value);
			return ENetworkOperationResult::Success;
		}
		netActor->ReplicateValue(Value);
		return ENetworkOperationResult::Success;
	}
	else if (!fromHistory)
	{
		Value.Tag = Tag;
		_pendingReplicatedValues.Enqueue(Value);
		return ENetworkOperationResult::Success;
	}
	return ENetworkOperationResult::UnknowError;
}

ENetworkOperationResult UPulseNetManager::RemoveReplicatedValue(const UObject* WorldContextObject, FName Tag, UObject* ForObj)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->RemoveReplicatedValue_Internal(Tag, ForObj, false);
	}
	return ENetworkOperationResult::UnknowError;
}

ENetworkOperationResult UPulseNetManager::RemoveReplicatedValue_Internal(FName Tag, UObject* ForObj, bool fromHistory)
{
	if (!(static_cast<int32>(GetNetAuth()) & static_cast<int32>(ENetworkAuthorizationState::GameplayValues)))
		return ENetworkOperationResult::MissingAuthorisation;
	if (auto netActor = _netActor)
	{
		if (!HasAuthority())
			return ENetworkOperationResult::NoAuthority;
		if (!fromHistory && !_pendingReplicatedValues.IsEmpty())
		{
			FReplicatedEntry Entry;
			Entry.Tag = Tag;
			Entry.WeakObjectPtr = ForObj;
			Entry.FlagValue = -1;
			_pendingReplicatedValues.Enqueue(Entry);
			return ENetworkOperationResult::Success;
		}
		netActor->RemoveReplicatedItem(Tag, true, ForObj);
		return ENetworkOperationResult::Success;
	}
	else if (!fromHistory)
	{
		FReplicatedEntry Entry;
		Entry.Tag = Tag;
		Entry.WeakObjectPtr = ForObj;
		Entry.FlagValue = -1;
		_pendingReplicatedValues.Enqueue(Entry);
		return ENetworkOperationResult::Success;
	}
	return ENetworkOperationResult::UnknowError;
}

ENetworkOperationResult UPulseNetManager::MakeRPCall(const UObject* WorldContextObject, FName Tag, FReplicatedEntry Value, bool Reliable)
{
	if (auto mgr = Get(WorldContextObject))
	{
		return mgr->MakeRPCall_Internal(Tag, Value, Reliable);
	}
	return ENetworkOperationResult::UnknowError;
}

ENetworkOperationResult UPulseNetManager::MakeRPCall_Internal(FName Tag, FReplicatedEntry Value, bool Reliable, bool fromHistory)
{
	if (!(static_cast<int32>(GetNetAuth()) & static_cast<int32>(ENetworkAuthorizationState::GameplayValues)))
		return ENetworkOperationResult::MissingAuthorisation;
	if (auto netActor = _netActor)
	{
		if (GetNetRole() == ROLE_SimulatedProxy)
			return ENetworkOperationResult::MissingAuthorisation;
		Value.Tag = Tag;
		if (Reliable)
		{
			if (!fromHistory && !_pendingReliableRPCs.IsEmpty())
			{
				_pendingReliableRPCs.Enqueue(Value);
				return ENetworkOperationResult::Success;
			}
			netActor->ReliableServerRPC(Value);
		}
		else
			netActor->UnreliableServerRPC(Value);
		return ENetworkOperationResult::Success;
	}
	else if (Reliable && !fromHistory)
	{
		Value.Tag = Tag;
		_pendingReliableRPCs.Enqueue(Value);
		return ENetworkOperationResult::Success;
	}
	return ENetworkOperationResult::UnknowError;
}
