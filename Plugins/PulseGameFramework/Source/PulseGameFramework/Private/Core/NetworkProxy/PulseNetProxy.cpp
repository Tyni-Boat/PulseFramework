// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/NetworkProxy/PulseNetProxy.h"

#include "PulseGameFramework.h"
#include "Core/PulseDebugLibrary.h"
#include "Core/NetworkProxy/PulseNetManager.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


void FReplicatedItem::PreReplicatedRemove(const struct FReplicatedArray& Serializer)
{
	if (Entry.Tag.IsNone())
		return;
	auto mes = FString::Printf(TEXT("Pulse Network Proxy Item: Failed To Remove tag: %s"), *Entry.Tag.ToString());
	if (auto NetManager = UPulseNetManager::Get(Serializer.ArrayOwner.Get()))
	{
		mes = FString::Printf(TEXT("Pulse Network Proxy Item: Remove Replicated tag: %s"), *Entry.Tag.ToString());
		NetManager->OnNetReplication_Raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Remove);
		NetManager->OnNetReplication.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Remove);
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(Serializer.ArrayOwner.Get(), mes));
}

void FReplicatedItem::PostReplicatedAdd(const struct FReplicatedArray& Serializer)
{
	if (Entry.Tag.IsNone())
		return;
	auto mes = FString::Printf(TEXT("Pulse Network Proxy Item: Failed To Add New tag: %s"), *Entry.Tag.ToString());
	if (auto NetManager = UPulseNetManager::Get(Serializer.ArrayOwner.Get()))
	{
		mes = FString::Printf(TEXT("Pulse Network Proxy Item: Add New Replicated tag: %s"), *Entry.Tag.ToString());
		NetManager->OnNetReplication_Raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::AddNew);
		NetManager->OnNetReplication.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::AddNew);
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(Serializer.ArrayOwner.Get(), mes));
}

void FReplicatedItem::PostReplicatedChange(const struct FReplicatedArray& Serializer)
{
	if (Entry.Tag.IsNone())
		return;
	auto mes = FString::Printf(TEXT("Pulse Network Proxy Item: Failed To Update tag: %s"), *Entry.Tag.ToString());
	if (auto NetManager = UPulseNetManager::Get(Serializer.ArrayOwner.Get()))
	{
		mes = FString::Printf(TEXT("Pulse Network Proxy Item: Update Replicated tag: %s"), *Entry.Tag.ToString());
		NetManager->OnNetReplication_Raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Update);
		NetManager->OnNetReplication.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Update);
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(Serializer.ArrayOwner.Get(), mes));
}

bool FReplicatedArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedItem, FReplicatedArray>(Items, DeltaParms, *this);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


APulseNetProxy::APulseNetProxy()
{
	bReplicates = true;
	SetNetParams();
	_replicatedValues.ArrayOwner = this;
}

void APulseNetProxy::BeginPlay()
{
	Super::BeginPlay();
	auto debugStr = FString::Printf(TEXT(""));
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		debugStr = FString::Printf(TEXT("Client Pulse Network proxy: %s begin Play. Player ID: %d , Local ID: %d"), *GetName(), PlayerID, LocalPlayerControllerID);
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		SetPlayerID(PlayerID, true);
	}
	else
	{
		debugStr = FString::Printf(TEXT("Server Pulse Network proxy: %s begin Play. Player ID: %d , Local ID: %d"), *GetName(), PlayerID, LocalPlayerControllerID);
		UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
	}
}

void APulseNetProxy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APulseNetProxy, _replicatedValues);
	DOREPLIFETIME(APulseNetProxy, PlayerID);
}

void APulseNetProxy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		UPulseNetManager::UnRegisterProxy(this);
	}
}

void APulseNetProxy::SetPlayerID(const int32 newPlayerID, bool Override)
{
	if (PlayerID == newPlayerID && !Override)
	{
		const auto debugStr = FString::Printf(TEXT("Pulse Network proxy: %s Set PLayer ID FAILED. Same Player ID: %d"), *GetName(), newPlayerID);
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		return;
	}
	if (newPlayerID < 0)
	{
		const auto debugStr = FString::Printf(TEXT("Pulse Network proxy: %s Set PLayer ID FAILED. Invalid New Player ID: %d"), *GetName(), newPlayerID);
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		return;
	}
	PlayerID = newPlayerID;
	const auto gameState = GetWorld()->GetGameState();
	if (!gameState)
	{
		const auto debugStr = FString::Printf(TEXT("Pulse Network proxy: %s Set PLayer ID FAILED. Invalid Game State"), *GetName());
		UE_LOG(LogPulseNetProxy, Error, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		return;
	}
	const int32 playerIndex = gameState->PlayerArray.IndexOfByPredicate([&](const APlayerState* playerState)-> bool { return playerState->GetPlayerId() == PlayerID; });
	if (playerIndex == INDEX_NONE)
	{
		const auto debugStr = FString::Printf(TEXT("Pulse Network proxy: %s Set PLayer ID FAILED. Invalid Player Index for player ID: %d, Retrying"), *GetName(), PlayerID);
		UE_LOG(LogPulseNetProxy, Warning, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
		GetWorld()->GetTimerManager().SetTimerForNextTick([&]()-> void { SetPlayerID(PlayerID, true); });
		return;
	}
	bool registered = false;
	LocalPlayerControllerID = UGameplayStatics::GetPlayerControllerID(gameState->PlayerArray[playerIndex]->GetPlayerController());
	if (UPulseNetManager::RegisterProxy(this))
	{
		registered = true;
	}
	const auto debugStr = FString::Printf(
		TEXT("Pulse Network proxy: %s Set PLayer ID. Local Player ID: %d, Player ID: %d , Registered? %d"), *GetName(), LocalPlayerControllerID, PlayerID, registered);
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, debugStr));
}

void APulseNetProxy::SetNetParams(const bool NetAlwaysRelevant, const float NetworkPriority, const float NetworkUpdateFrequency)
{
	bAlwaysRelevant = NetAlwaysRelevant; // For simplicity, making it always relevant.
	SetNetUpdateFrequency(NetworkUpdateFrequency); // High update frequency.
	NetPriority = NetworkPriority; // High priority.
}

void APulseNetProxy::InitFromSaved(const FProxySaved& ProxySaved)
{
	for (const FPulseNetReplicatedData& Value : ProxySaved.RepArray)
	{
		int32 indexOf = _replicatedValues.Items.IndexOfByPredicate([Value](const FReplicatedItem& Entry)-> bool
		{
			return !Entry.Entry.Tag.IsNone() && Entry.Entry.Tag == Value.Tag;
		});
		if (!_replicatedValues.Items.IsValidIndex(indexOf))
		{
			FReplicatedItem NewItem;
			NewItem.Entry = Value;
			_replicatedValues.Items.Add(NewItem);
			_replicatedValues.MarkItemDirty(_replicatedValues.Items.Last());
		}
	}
}

FProxySaved APulseNetProxy::SaveProxy()
{
	FProxySaved ProxySaved;
	for (const FReplicatedItem& Value : _replicatedValues.Items)
	{
		ProxySaved.RepArray.Add(Value.Entry);
	}
	return ProxySaved;
}

int32 APulseNetProxy::GetLocalPlayerControllerID() const
{
	return LocalPlayerControllerID;
}

APlayerController* APulseNetProxy::GetLocalPlayerController() const
{
	return UGameplayStatics::GetPlayerControllerFromID(this, GetLocalPlayerControllerID());
}

bool APulseNetProxy::TagExist(const FName& Tag) const
{
	int32 indexOf = _replicatedValues.Items.IndexOfByPredicate([Tag](const FReplicatedItem& Entry)-> bool
	{
		return !Entry.Entry.Tag.IsNone() && Entry.Entry.Tag == Tag;
	});
	return indexOf != INDEX_NONE;
}

void APulseNetProxy::OnRep_ReplicatedValues()
{
}

int32 APulseNetProxy::GetPlayerID() const
{
	return PlayerID;
}

void APulseNetProxy::ReplicateValue_Implementation(FPulseNetReplicatedData Value)
{
	if (Value.Tag.IsNone())
		return;
	Value.ServerArrivalOrder = UPulseNetManager::ConsumeRequestArrivalOrder(this);
	if (Value.ServerArrivalOrder < 0)
		return;
	int32 indexOf = _replicatedValues.Items.IndexOfByPredicate([Value](const FReplicatedItem& Entry)-> bool
	{
		return !Entry.Entry.Tag.IsNone() && Entry.Entry.Tag == Value.Tag;
	});
	auto mes = FString::Printf(TEXT("Pulse Network Proxy: Replicate %s Request tag: %s ; size: %llu"), *(indexOf >= 0 ? FString("Update") : FString("Add New"))
	                           , *Value.Tag.ToString(), sizeof(Value));
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	if (_replicatedValues.Items.IsValidIndex(indexOf))
	{
		_replicatedValues.Items[indexOf].Entry.NameValue = Value.NameValue;
		_replicatedValues.Items[indexOf].Entry.EnumValue = Value.EnumValue;
		_replicatedValues.Items[indexOf].Entry.FlagValue = Value.FlagValue;
		_replicatedValues.Items[indexOf].Entry.IntegerValue = Value.IntegerValue;
		_replicatedValues.Items[indexOf].Entry.DoubleValue = Value.DoubleValue;
		_replicatedValues.Items[indexOf].Entry.Float31Value = Value.Float31Value;
		_replicatedValues.Items[indexOf].Entry.Float32Value = Value.Float32Value;
		_replicatedValues.Items[indexOf].Entry.Float33Value = Value.Float33Value;
		_replicatedValues.MarkItemDirty(_replicatedValues.Items[indexOf]);

		if (auto NetManager = UPulseNetManager::Get(this))
		{
			mes = FString::Printf(TEXT("Pulse Network Proxy Item: Server Update tag: %s"), *Value.Tag.ToString());
			NetManager->OnNetReplication_Raw.Broadcast(Value.Tag, Value, EReplicationEntryOperationType::Update);
			NetManager->OnNetReplication.Broadcast(Value.Tag, Value, EReplicationEntryOperationType::Update);
		}
	}
	else
	{
		FReplicatedItem NewItem;
		NewItem.Entry = Value;
		_replicatedValues.Items.Add(NewItem);
		_replicatedValues.MarkItemDirty(_replicatedValues.Items.Last());

		if (auto NetManager = UPulseNetManager::Get(this))
		{
			mes = FString::Printf(TEXT("Pulse Network Proxy Item: Server Add New tag: %s"), *Value.Tag.ToString());
			NetManager->OnNetReplication_Raw.Broadcast(Value.Tag, Value, EReplicationEntryOperationType::AddNew);
			NetManager->OnNetReplication.Broadcast(Value.Tag, Value, EReplicationEntryOperationType::AddNew);
		}
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
}

void APulseNetProxy::RemoveReplicatedItem_Implementation(FName Tag)
{
	if (Tag.IsNone())
		return;
	TArray<int32> indexes;
	TArray<FPulseNetReplicatedData> datas;
	for (int i = 0; i < _replicatedValues.Items.Num(); i++)
	{
		if (!_replicatedValues.Items[i].Entry.Tag.ToString().Contains(Tag.ToString()))
			continue;
		indexes.Add(i);
		datas.Add(_replicatedValues.Items[i].Entry);
	}
	auto mes = FString::Printf(TEXT(""));
	for (int i = indexes.Num() - 1; i >= 0; i--)
	{
		_replicatedValues.Items.RemoveAt(indexes[i]);
	}
	if (indexes.Num() > 0)
	{
		_replicatedValues.MarkArrayDirty();

		if (auto NetManager = UPulseNetManager::Get(this))
		{
			for (int i = datas.Num() - 1; i >= 0; i--)
			{
				NetManager->OnNetReplication_Raw.Broadcast(datas[i].Tag, datas[i], EReplicationEntryOperationType::Remove);
				NetManager->OnNetReplication.Broadcast(datas[i].Tag, datas[i], EReplicationEntryOperationType::Remove);
				mes = FString::Printf(TEXT("Pulse Network Proxy Item: Server Remove tag: %s ; Request size: %llu"), *datas[i].Tag.ToString(), sizeof(datas[i].Tag));
				UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
			}
		}
	}
}


void APulseNetProxy::ReliableServerRPC_AddOrUpdate_Implementation(FPulseNetReplicatedData Value)
{
	const auto mes = FString::Printf(TEXT("Pulse Network Proxy: Replicate Request tag: %s"), *Value.Tag.ToString());
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	ReplicateValue(Value);
}

void APulseNetProxy::ReliableServerRPC_Remove_Implementation(FName Tag)
{
	const auto mes = FString::Printf(TEXT("Pulse Network Proxy: Remove Request tag: %s"), *Tag.ToString());
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	RemoveReplicatedItem(Tag);
}

bool APulseNetProxy::QueryNetValue(FName Tag, FPulseNetReplicatedData& OutValue, bool bIncludeChildValues, TArray<FPulseNetReplicatedData>& OutChildValues) const
{
	if (Tag.IsNone())
		return false;
	bool foundValue = false;
	for (int i = 0; i < _replicatedValues.Items.Num(); i++)
	{
		if (!_replicatedValues.Items[i].Entry.Tag.ToString().Contains(Tag.ToString()))
			continue;
		if (_replicatedValues.Items[i].Entry.Tag == Tag)
		{
			foundValue = true;
			OutValue = _replicatedValues.Items[i].Entry;
			OutValue.OwnerPlayerID = GetPlayerID();
		}
		if (!bIncludeChildValues)
			continue;
		OutChildValues.Add(_replicatedValues.Items[i].Entry);
		OutChildValues.Last().OwnerPlayerID = GetPlayerID();
	}
	return foundValue || OutChildValues.Num() > 0;
}
