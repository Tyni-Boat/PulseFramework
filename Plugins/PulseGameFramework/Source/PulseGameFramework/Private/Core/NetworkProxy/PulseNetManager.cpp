// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/NetworkProxy/PulseNetManager.h"

#include "PulseGameFramework.h"
#include "Core/PulseDebugLibrary.h"
#include "Core/PulseSystemLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"


bool UPulseNetManager::RegisterProxy(APulseNetProxy* NetProxy)
{
	if (!NetProxy)
		return false;
	if (auto mgr = Get(NetProxy))
	{
		return mgr->AddNetProxy_Internal(NetProxy->GetPlayerID(), NetProxy);
	}
	return false;
}

bool UPulseNetManager::UnRegisterProxy(const APulseNetProxy* NetProxy)
{
	if (!NetProxy)
		return false;
	if (auto mgr = Get(NetProxy))
	{
		return mgr->RemoveNetProxy_Internal(NetProxy->GetPlayerID());
	}
	return false;
}

int64 UPulseNetManager::ConsumeRequestArrivalOrder(const APulseNetProxy* NetProxy)
{
	if (!NetProxy)
		return -1;
	if (auto mgr = Get(NetProxy))
	{
		if (!mgr->IsPlayerRegistered_Internal(NetProxy->GetPlayerID()))
			return -1;
		const int64 val = mgr->_serverQueryArrivalOrder;
		mgr->_serverQueryArrivalOrder++;
		return val;
	}
	return -1;
}

void UPulseNetManager::HandleNetHistory()
{
	FPendingRepData PendingRepData;
	while (_pendingReps.Dequeue(PendingRepData))
	{
		if (PendingRepData.bRemove)
			RemoveReplicatedValue_Internal(PendingRepData.Data.Tag, true);
		else
			ReplicateValue_Internal(PendingRepData.Data.Tag, PendingRepData.Data, true);
	}
}

TStatId UPulseNetManager::GetStatId() const
{
	return Super::GetStatID();
}

void UPulseNetManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	_netMode = GetWorld()->GetNetMode();
	if (auto config = GetProjectSettings())
	{
		_bNetworkManagerAlwaysRelevant = config->bNetworkManagerAlwaysRelevant;
		_netPriority = config->NetPriority;
		_netUpdateFrequency = config->NetUpdateFrequency;
		_checkPlayerEveryXFrame = config->NetCheckFrameInterval;
		_conserveDisconnectedPlayerNetProxies = config->bConserveDisconnecterPlayerNetProxies;
	}
	MonitorPlayers_Internal();
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Pulse Network manager Initialized")));
}

void UPulseNetManager::PostInitialize()
{
	Super::PostInitialize();
	MonitorPlayers_Internal();
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Pulse Network manager Post-Initialization")));
}

void UPulseNetManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GFrameCounter % _checkPlayerEveryXFrame == 0)
	{
		MonitorPlayers_Internal();
	}
}

void UPulseNetManager::Deinitialize()
{
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

int32 UPulseNetManager::GetNetLocalID() const
{
	return _masterLocalID_NetProxyID.Y;
}

int32 UPulseNetManager::GetControllerNetLocalID(APlayerController* Controller) const
{
	if (!Controller)
		return -1;
	const auto playerState = Controller->GetPlayerState<APlayerState>();
	if (!playerState)
		return -1;
	return playerState->GetPlayerId();
}

bool UPulseNetManager::QueryNetValue(const TArray<FName>& Tags, FPulseNetReplicatedData& OutValue, bool bIncludeChildValues, TArray<FPulseNetReplicatedData>& OutChildValues) const
{
	OutChildValues.Empty();
	int resultCount = 0;
	if (Tags.IsEmpty())
		return false;
	for (const auto tag : Tags)
	{
		for (const auto& proxyPair : _remoteNetProxies)
		{
			if (!proxyPair.Value)
				continue;
			resultCount += proxyPair.Value->QueryNetValue(tag, OutValue, bIncludeChildValues, OutChildValues) ? 1 : 0;
		}
		for (const auto& proxyPair : _localNetProxies)
		{
			if (!proxyPair.Value)
				continue;
			resultCount += proxyPair.Value->QueryNetValue(tag, OutValue, bIncludeChildValues, OutChildValues) ? 1 : 0;
		}
	}
	OutChildValues.Sort([](const FPulseNetReplicatedData& a, const FPulseNetReplicatedData& b)
		{
			return a.ServerArrivalOrder < b.ServerArrivalOrder;
		});
	return resultCount > 0;
}

void UPulseNetManager::ReplicateValue(FName Tag, FPulseNetReplicatedData Value)
{
	ReplicateValue_Internal(Tag, Value);
}

bool UPulseNetManager::RemoveReplicatedValue(FName Tag)
{
	return RemoveReplicatedValue_Internal(Tag);
}


bool UPulseNetManager::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UPulseNetManager::ReplicateValue_Internal(FName Tag, FPulseNetReplicatedData Value, bool fromHistory)
{
	if (auto netActor = GetMasterProxy_Internal())
	{
		Value.Tag = Tag;
		if (!fromHistory && !_pendingReps.IsEmpty())
		{
			_pendingReps.Enqueue(FPendingRepData(Value));
			return;
		}
		const auto mes = FString::Printf(TEXT("Pulse Network Manager: Replicate Request tag: %s"), *Tag.ToString());
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
		netActor->ReliableServerRPC_AddOrUpdate(Value);
	}
	else if (!fromHistory)
	{
		Value.Tag = Tag;
		_pendingReps.Enqueue(FPendingRepData(Value));
	}
}

bool UPulseNetManager::RemoveReplicatedValue_Internal(FName Tag, bool fromHistory)
{
	if (auto NetProxy = GetMasterProxy_Internal())
	{
		if (!fromHistory && !_pendingReps.IsEmpty())
		{
			FPulseNetReplicatedData Entry;
			Entry.Tag = Tag;
			_pendingReps.Enqueue(FPendingRepData(Entry, true));
			return true;
		}
		if (!NetProxy->TagExist(Tag))
			return false;
		const auto mes = FString::Printf(TEXT("Pulse Network Manager: Remove Request tag: %s"), *Tag.ToString());
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
		NetProxy->ReliableServerRPC_Remove(Tag);
		return true;
	}
	else if (!fromHistory)
	{
		FPulseNetReplicatedData Entry;
		Entry.Tag = Tag;
		_pendingReps.Enqueue(FPendingRepData(Entry, true));
		return true;
	}
	return false;
}

int32 UPulseNetManager::NewNetProxy_Internal(const int32& PlayerIndex) const
{
	const auto netMode = GetWorld()->GetNetMode();
	if (netMode >= NM_Client)
	{
		//UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Impossible to Create Pulse Network proxy On Client")));
		return 0;
	}
	const auto gameState = GetWorld()->GetGameState();
	if (!gameState)
	{
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Impossible to Create Pulse Network proxy: Null Game State")));
		return 0;
	}
	if (!gameState->PlayerArray.IsValidIndex(PlayerIndex))
	{
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Impossible to Create Pulse Network proxy: Invalid player index")));
		return 0;
	}
	const auto playerState = gameState->PlayerArray[PlayerIndex];
	if (!playerState)
	{
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Impossible to Create Pulse Network proxy: Null Player state")));
		return 0;
	}
	int32 result = 0;
	const FVector netParams = FVector(_bNetworkManagerAlwaysRelevant ? 1 : 0, _netPriority, _netUpdateFrequency);
	const int32 playerId = playerState->GetPlayerId();
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = playerState;
	spawnParams.CustomPreSpawnInitalization = [netParams, playerId, playerState](AActor* actorSpawn)-> void
		{
			if (auto proxy = Cast<APulseNetProxy>(actorSpawn))
			{
				proxy->SetNetParams(netParams.X > 0, netParams.Y, netParams.Z);
				proxy->SetPlayerID(playerId);
			}
		};
	const auto newSpawn = GetWorld()->SpawnActor<APulseNetProxy>(APulseNetProxy::StaticClass(), spawnParams);
	if (newSpawn != nullptr)
	{
		result = playerState->GetPlayerController() ? 1 : 2;
		const auto mes = FString::Printf(TEXT("Pulse Network Proxy Created: %s with player ID: %d"), *newSpawn->GetName(), playerId);
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	}
	else
	{
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, TEXT("Pulse Network Proxy Creation Failed")));
	}
	return result;
}

TObjectPtr<APulseNetProxy> UPulseNetManager::GetMasterProxy_Internal() const
{
	if (!_localNetProxies.Contains(_masterLocalID_NetProxyID.Y))
		return nullptr;
	return _localNetProxies[_masterLocalID_NetProxyID.Y];
}

bool UPulseNetManager::IsPlayerRegistered_Internal(const int32 PlayerID) const
{
	return PlayerRegistrationType_Internal(PlayerID) > 0;
}

int32 UPulseNetManager::PlayerRegistrationType_Internal(const int32 PlayerID) const
{
	if (_localNetProxies.Contains(PlayerID))
	{
		return 1;
	}
	if (_remoteNetProxies.Contains(PlayerID))
	{
		return 2;
	}
	return 0;
}


void UPulseNetManager::MonitorPlayers_Internal()
{
	const auto gameState = GetWorld()->GetGameState();
	if (!gameState)
		return;
	const int32 playerCount = gameState->PlayerArray.Num();
	if (playerCount == _currentPlayerCount)
		return;
	_currentPlayerCount = playerCount;
	TArray<int32> playerIDs;
	for (int i = 0; i < playerCount; ++i)
	{
		const int32 id = gameState->PlayerArray[i]->GetPlayerId();
		playerIDs.Add(id);
		if (IsPlayerRegistered_Internal(id))
			continue;
		const auto response = NewNetProxy_Internal(i);
		if (response == 1)
		{
			if (!_localNetProxies.Contains(id))
				_localNetProxies.Add(id);
		}
		else if (response == 2)
		{
			if (!_remoteNetProxies.Contains(id))
				_remoteNetProxies.Add(id);
		}
	}
	TArray<int32> allKeysIds;
	TArray<int32> keysIds;
	_localNetProxies.GetKeys(keysIds);
	UPulseSystemLibrary::ArrayCopyIntoFirst(allKeysIds, keysIds);
	keysIds.Reset();
	_remoteNetProxies.GetKeys(keysIds);
	UPulseSystemLibrary::ArrayCopyIntoFirst(allKeysIds, keysIds);
	for (int i = allKeysIds.Num() - 1; i >= 0; --i)
	{
		const int32 id = allKeysIds[i];
		if (playerIDs.Contains(id))
			continue;
		RemoveNetProxy_Internal(id);
	}
}

bool UPulseNetManager::AddNetProxy_Internal(const int32 PlayerID, APulseNetProxy* NetProxy)
{
	if (!NetProxy)
		return false;
	auto debugStr = FString::Printf(TEXT(""));
	if (NetProxy->GetLocalPlayerController())
	{
		if (_localNetProxies.Contains(PlayerID))
		{
			if (_netMode < NM_Client && _disconnectedPlayers.Contains(PlayerID))
			{
				NetProxy->InitFromSaved(_disconnectedPlayers[PlayerID]);
				_disconnectedPlayers.Remove(PlayerID);
			}
			_localNetProxies[PlayerID] = NetProxy;
		}
		else
		{
			_localNetProxies.Add(PlayerID, NetProxy);
		}
		debugStr = FString::Printf(
			TEXT("Pulse Network proxy %s Registered To Pulse Network Manager, Player ID: %d as Local Proxy"), *NetProxy->GetName(), NetProxy->GetPlayerID());
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		SetNewMasterNetProxy_Internal(NetProxy);
	}
	else
	{
		if (_remoteNetProxies.Contains(PlayerID))
		{
			if (_netMode < NM_Client && _disconnectedPlayers.Contains(PlayerID))
			{
				NetProxy->InitFromSaved(_disconnectedPlayers[PlayerID]);
				_disconnectedPlayers.Remove(PlayerID);
			}
			_remoteNetProxies[PlayerID] = NetProxy;
		}
		else
		{
			_remoteNetProxies.Add(PlayerID, NetProxy);
		}
		debugStr = FString::Printf(
			TEXT("Pulse Network proxy %s Registered To Pulse Network Manager, Player ID: %d as Remote Proxy"), *NetProxy->GetName(), NetProxy->GetPlayerID());
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
	}
	return true;
}

bool UPulseNetManager::RemoveNetProxy_Internal(const int32 PlayerID)
{
	const auto netMode = GetWorld()->GetNetMode();
	bool found = false;
	if (_remoteNetProxies.Contains(PlayerID))
	{
		found = true;
		if (netMode < NM_Client)
		{
			if (_remoteNetProxies[PlayerID].Get())
			{
				if (_conserveDisconnectedPlayerNetProxies)
				{
					_disconnectedPlayers.Add(PlayerID, _remoteNetProxies[PlayerID]->SaveProxy());
				}
				_remoteNetProxies[PlayerID].Get()->Destroy();
			}
		}
		_remoteNetProxies.Remove(PlayerID);
	}
	if (_localNetProxies.Contains(PlayerID))
	{
		found = true;
		if (netMode < NM_Client)
		{
			if (_localNetProxies[PlayerID].Get())
			{
				if (_conserveDisconnectedPlayerNetProxies)
				{
					_disconnectedPlayers.Add(PlayerID, _localNetProxies[PlayerID]->SaveProxy());
				}
				_localNetProxies[PlayerID].Get()->Destroy();
			}
		}
		_localNetProxies.Remove(PlayerID);

		if (_masterLocalID_NetProxyID.Y == PlayerID)
		{
			bool hadSetNewMasterNetProxy = false;
			for (const TPair<int, TObjectPtr<APulseNetProxy>>& Pair : _localNetProxies)
			{
				if (SetNewMasterNetProxy_Internal(Pair.Value))
				{
					hadSetNewMasterNetProxy = true;
				}
			}
			if (!hadSetNewMasterNetProxy)
			{
				_masterLocalID_NetProxyID = FVector2D(-1, -1);
			}
		}
	}
	if (found)
	{
		const auto debugStr = FString::Printf(
			TEXT("Removed Network proxy Player ID: %d"), PlayerID);
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
	}
	return found;
}

bool UPulseNetManager::SetNewMasterNetProxy_Internal(APulseNetProxy* NetProxy)
{
	if (!NetProxy)
		return false;
	const int32 playerId = NetProxy->GetPlayerID();
	if (_masterLocalID_NetProxyID.Y == playerId)
		return false;
	const auto controller = NetProxy->GetLocalPlayerController();
	if (!controller)
		return false;
	const int32 localControllerID = UGameplayStatics::GetPlayerControllerID(controller);
	if (localControllerID >= 0 && controller == GetWorld()->GetFirstPlayerController())
	{
		if (!_localNetProxies.Contains(playerId))
		{
			_localNetProxies.Add(playerId, NetProxy);
		}
		const FVector2D lastMaster = _masterLocalID_NetProxyID;
		_masterLocalID_NetProxyID = FVector2D(localControllerID, playerId);
		if (lastMaster.Y < 0)
		{
			HandleNetHistory();
			if (!_hadNetInit)
			{
				OnNetInit.Broadcast();
				OnNetInit_Raw.Broadcast();
				_hadNetInit = true;
			}
		}
		const auto debugStr = FString::Printf(
			TEXT("Pulse Network proxy %s, Player ID: %d Is designated as the new Master Proxy"), *NetProxy->GetName(), playerId);
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		return true;
	}
	return false;
}

bool UPulseNetManager::GetDisconnectedPlayerIds(TArray<int32>& OutPlayerIDs) const
{
	OutPlayerIDs.Empty();
	const auto netMode = GetWorld()->GetNetMode();
	if (netMode >= NM_Client)
		return false;
	if (!_conserveDisconnectedPlayerNetProxies)
		return false;
	return _disconnectedPlayers.GetKeys(OutPlayerIDs) > 0;
}
