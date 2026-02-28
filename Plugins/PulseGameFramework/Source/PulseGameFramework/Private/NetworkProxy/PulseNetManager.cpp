// Copyright © by Tyni Boat. All Rights Reserved.


#include "NetworkProxy/PulseNetManager.h"

#include "PulseGameFramework.h"
#include "Core/PulseDebugLibrary.h"
#include "Core/PulseSystemLibrary.h"
#include "NetworkProxy/IPulseNetProxy.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"


bool UPulseNetManager::RegisterEmitter(APulseNetEmitter* Emitter)
{
	if (!Emitter)
		return false;
	const auto ctrl = Cast<AController>(Emitter->Owner);
	if (!ctrl)
		return false;
	if (!ctrl->IsLocalController())
		return false;
	auto mgr = Get(Emitter);
	if (!mgr)
		return false;
	if (mgr->_emitter)
		return false;
	mgr->_emitter = Emitter;
	mgr->OnNetInitialization.Broadcast(mgr->_receptor != nullptr, true);
	mgr->OnNetInitialization_Raw.Broadcast(mgr->_receptor != nullptr, true);
	UPulseSystemLibrary::ForeachActorInterface(mgr, UIPulseNetProxy::StaticClass(), [w_mgr = MakeWeakObjectPtr(mgr)](AActor* actor)
	{
		IIPulseNetProxy::Execute_OnNetInitialization(actor, w_mgr.IsValid() && w_mgr->_receptor != nullptr, w_mgr.IsValid() && w_mgr->_emitter != nullptr);
	});
	const auto mes = FString::Printf(TEXT("Pulse Net manager: Successfully set emitter %s ;player ID: %d"), *Emitter->GetName(), Emitter->GetPlayerID());
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(mgr, mes));
	return true;
}

bool UPulseNetManager::RegisterReceptor(APulseNetReceptor* Receptor)
{
	if (!Receptor)
		return false;
	auto mgr = Get(Receptor);
	if (!mgr)
		return false;
	if (mgr->_receptor)
		return false;
	mgr->MessageRepHandle = Receptor->OnItemEvent_raw.AddUObject(mgr, &UPulseNetManager::OnRep_ReplicatedValue);
	mgr->JoinHandle = Receptor->OnPlayerJoined_raw.AddUObject(mgr, &UPulseNetManager::OnRep_PlayerJoined);
	mgr->LeftHandle = Receptor->OnPlayerLeft_raw.AddUObject(mgr, &UPulseNetManager::OnRep_PlayerLeft);
	mgr->_receptor = Receptor;
	mgr->OnNetInitialization.Broadcast(true, mgr->_emitter != nullptr);
	mgr->OnNetInitialization_Raw.Broadcast(true, mgr->_emitter != nullptr);
	UPulseSystemLibrary::ForeachActorInterface(mgr, UIPulseNetProxy::StaticClass(), [w_mgr = MakeWeakObjectPtr(mgr)](AActor* actor)
	{
		IIPulseNetProxy::Execute_OnNetInitialization(actor, w_mgr.IsValid() && w_mgr->_receptor != nullptr, w_mgr.IsValid() && w_mgr->_emitter != nullptr);
	});
	const auto mes = FString::Printf(TEXT("Pulse Net manager: Successfully set receptor %s"), *Receptor->GetName());
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(mgr, mes));
	return true;
}

void UPulseNetManager::OnRep_ReplicatedValue(FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType Operation)
{
	OnNetMessageReceived.Broadcast(Tag, Value, Operation);
	OnNetMessageReceived_Raw.Broadcast(Tag, Value, Operation);
	UPulseSystemLibrary::ForeachActorInterface(this, UIPulseNetProxy::StaticClass(), [w_mgr = MakeWeakObjectPtr(this), Tag, Value, Operation](AActor* actor)
	{
		IIPulseNetProxy::Execute_OnNetMessageReceived(actor, Tag, Value, Operation);
	});
}

void UPulseNetManager::OnRep_PlayerPostLogin(AGameModeBase* GameMode, APlayerController* JoiningPlayer)
{
	if (!JoiningPlayer)
		return;
	auto playerState = JoiningPlayer->GetPlayerState<APlayerState>();
	if (!playerState)
		return;
	// Spawn emitter
	const FVector netParams = FVector(_bNetworkManagerAlwaysRelevant ? 1 : 0, _netPriority, _netUpdateFrequency);
	const int32 playerId = playerState->GetPlayerId();
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = JoiningPlayer;
	spawnParams.CustomPreSpawnInitalization = [netParams, playerId, ps = MakeWeakObjectPtr(playerState)](AActor* actorSpawn)-> void
	{
		actorSpawn->bNetUseOwnerRelevancy = true;
		actorSpawn->bOnlyRelevantToOwner = true;
		if (auto emitter = Cast<APulseNetEmitter>(actorSpawn))
		{
			emitter->TempPLayerID = playerId;
			emitter->SetNetParams(netParams.Y, netParams.Z);
		}
	};
	const auto newSpawn = GetWorld()->SpawnActor<APulseNetEmitter>(APulseNetEmitter::StaticClass(), spawnParams);
	if (newSpawn != nullptr)
	{
		if (_emitter != newSpawn)
		{
			const auto mes = FString::Printf(TEXT("Pulse Net Manager: Created Emitter: %s for %s player ID: %d"), *newSpawn->GetName()
				, *FString(JoiningPlayer->IsLocalController()? TEXT("Local") : TEXT("Joining")), playerId);
			UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
		}
	}
	else
	{
		const auto mes = FString::Printf(TEXT("Pulse Net Manager: failed to create Emitter for joining player ID: %d"), playerId);
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	}
	if (_receptor)
		_receptor->OnPlayerJoined_Multicast(playerId);
}

void UPulseNetManager::OnRep_PlayerLogout(AGameModeBase* GameMode, AController* LeavingPlayer)
{
	if (_receptor && LeavingPlayer && LeavingPlayer->GetPlayerState<APlayerState>())
		_receptor->OnPlayerLeft_Multicast(LeavingPlayer->GetPlayerState<APlayerState>()->GetPlayerId());
}

void UPulseNetManager::OnRep_PlayerJoined(int32 playerID)
{
	OnNetPlayerJoined.Broadcast(playerID);
	OnNetPlayerJoined_Raw.Broadcast(playerID);
	UPulseSystemLibrary::ForeachActorInterface(this, UIPulseNetProxy::StaticClass(), [w_mgr = MakeWeakObjectPtr(this), playerID](AActor* actor)
	{
		IIPulseNetProxy::Execute_OnNetPlayerJoined(actor, playerID);
	});
}

void UPulseNetManager::OnRep_PlayerLeft(int32 playerID)
{
	OnNetPlayerLeft.Broadcast(playerID);
	OnNetPlayerLeft_Raw.Broadcast(playerID);
	UPulseSystemLibrary::ForeachActorInterface(this, UIPulseNetProxy::StaticClass(), [w_mgr = MakeWeakObjectPtr(this), playerID](AActor* actor)
	{
		IIPulseNetProxy::Execute_OnNetPlayerLeft(actor, playerID);
	});
}

APulseNetReceptor* UPulseNetManager::GetReceptor(const UObject* WorldContext)
{
	if (auto mgr = Get(WorldContext))
		return mgr->_receptor;
	return nullptr;
}

int32 UPulseNetManager::GetLocalPLayerID() const
{
	if (!_emitter)
		return -1;
	return _emitter->GetPlayerID();
}

void UPulseNetManager::GetLocalCapabilities(bool& bCanBroadcast, bool& bCanReceive) const
{
	bCanBroadcast = _emitter != nullptr;
	bCanReceive = _receptor != nullptr;
}

bool UPulseNetManager::BroadcastNetMessage(FName Tag, FPulseNetReplicatedData Value)
{
	if (!_emitter)
		return false;
	Value.Tag = Tag;
	_emitter->BroadcastNetMessage(Value);
	return true;
}

bool UPulseNetManager::DeleteNetMessage(FName Tag)
{
	if (!_emitter)
		return false;
	_emitter->DeleteNetEntry(Tag);
	return true;
}

TStatId UPulseNetManager::GetStatId() const
{
	return Super::GetStatID();
}

void UPulseNetManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (auto config = GetProjectSettings())
	{
		_bNetworkManagerAlwaysRelevant = config->bNetworkManagerAlwaysRelevant;
		_netPriority = config->NetPriority;
		_netUpdateFrequency = config->NetUpdateFrequency;
	}
}

void UPulseNetManager::Tick(float DeltaTime)
{
	if (wasInitialized)
		return;
	Super::Tick(DeltaTime);
	if (!UGameplayStatics::GetGameState(this))
		return;
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Pulse Network manager Initialisation")));
	const auto netMode = GetWorld()->GetNetMode();
	if (netMode < NM_Client && netMode > NM_Standalone)
	{
		// Spawn receptor
		SpawnReceptor_Internal();
		// Spawn local emitters
		SpawnLocalEmitters_Internal();
		// Bind to the global GameMode events
		if (!FGameModeEvents::GameModePostLoginEvent.IsBoundToObject(this))
		{
			PostLoginHandle = FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UPulseNetManager::OnRep_PlayerPostLogin);
		}
		if (!FGameModeEvents::GameModeLogoutEvent.IsBoundToObject(this))
		{
			LogoutHandle = FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UPulseNetManager::OnRep_PlayerLogout);
		}
	}
	wasInitialized = true;
}

void UPulseNetManager::Deinitialize()
{
	// Clean up bindings
	if (PostLoginHandle.IsValid())
	{
		FGameModeEvents::GameModePostLoginEvent.Remove(PostLoginHandle);
	}
	if (LogoutHandle.IsValid())
	{
		FGameModeEvents::GameModeLogoutEvent.Remove(LogoutHandle);
	}
	if (_receptor)
	{
		if (MessageRepHandle.IsValid())
			_receptor->OnItemEvent_raw.Remove(MessageRepHandle);
		if (JoinHandle.IsValid())
			_receptor->OnPlayerJoined_raw.Remove(JoinHandle);
		if (LeftHandle.IsValid())
			_receptor->OnPlayerLeft_raw.Remove(LeftHandle);
	}
	const auto netMode = GetWorld()->GetNetMode();
	if (netMode < NM_Client && netMode > NM_Standalone)
	{
		if (_receptor)
			_receptor->Destroy();
	}
	Super::Deinitialize();
}

UPulseNetManager* UPulseNetManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;
	const auto world = WorldContextObject->GetWorld();
	if (!world)
		return nullptr;
	return world->GetSubsystem<UPulseNetManager>();
}


bool UPulseNetManager::QueryNetMessage(const TArray<FName>& Tags, bool bIncludeDerivedTags, TArray<FPulseNetReplicatedData>& OutValues, bool bSortByArrivalDate) const
{
	int count = OutValues.Num();
	int resultCount = 0;
	if (Tags.IsEmpty())
		return false;
	// Remove Duplicates
	auto tagSet = TSet(Tags);
	auto inTags = tagSet.Array();
	// Look out
	if (_receptor)
	{
		if (_receptor->QueryNetValues(inTags, OutValues, !bIncludeDerivedTags))
		{
			resultCount = FMath::Abs(OutValues.Num() - count);
		}
	}
	if (bSortByArrivalDate)
	{
		OutValues.Sort([](const FPulseNetReplicatedData& a, const FPulseNetReplicatedData& b)
		{
			return a.ServerArrivalOrder < b.ServerArrivalOrder;
		});
	}
	return resultCount > 0;
}


bool UPulseNetManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UPulseNetManager::SpawnReceptor_Internal()
{
	auto gameState = UGameplayStatics::GetGameState(this);
	if (!gameState)
		return false;
	const FVector netParams = FVector(_bNetworkManagerAlwaysRelevant ? 1 : 0, _netPriority, _netUpdateFrequency);
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = UGameplayStatics::GetGameState(this);
	spawnParams.CustomPreSpawnInitalization = [netParams](AActor* actorSpawn)-> void
	{
		if (auto receptor = Cast<APulseNetReceptor>(actorSpawn))
		{
			receptor->SetNetParams(netParams.X > 0, netParams.Y, netParams.Z);
		}
	};
	const auto newSpawn = GetWorld()->SpawnActor<APulseNetReceptor>(APulseNetReceptor::StaticClass(), spawnParams);
	if (newSpawn != nullptr)
	{
		if (_receptor != newSpawn)
		{
			const auto mes = FString::Printf(TEXT("Pulse Net Manager: Created Receptor: %s"), *newSpawn->GetName());
			UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
		}
		return true;
	}

	const auto mes = FString::Printf(TEXT("Pulse Net Manager: failed to create Receptor"));
	UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	return false;
}

void UPulseNetManager::SpawnLocalEmitters_Internal()
{
	int32 localPlayerCount = UGameplayStatics::GetNumLocalPlayerControllers(this);
	auto GameMode = UGameplayStatics::GetGameMode(this);
	if (localPlayerCount > 0)
	{
		for (int32 i = 0; i < localPlayerCount; ++i)
		{
			const auto plCtrl = UGameplayStatics::GetPlayerController(this, i);
			if (!plCtrl)
				continue;
			if (!plCtrl->IsLocalController())
				continue;
			OnRep_PlayerPostLogin(GameMode, plCtrl);
		}
	}
}
