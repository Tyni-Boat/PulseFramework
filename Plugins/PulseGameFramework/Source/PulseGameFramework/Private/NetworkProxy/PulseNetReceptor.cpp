// Copyright © by Tyni Boat. All Rights Reserved.


#include "NetworkProxy/PulseNetReceptor.h"

#include "PulseGameFramework.h"
#include "Core/PulseDebugLibrary.h"
#include "NetworkProxy/PulseNetManager.h"
#include "Net/UnrealNetwork.h"


// Sets default values
APulseNetReceptor::APulseNetReceptor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetParams();
	_replicatedValues.ArrayOwner = this;
}

void APulseNetReceptor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APulseNetReceptor, _replicatedValues);
}

void APulseNetReceptor::SetNetParams(const bool NetAlwaysRelevant, const float NetworkPriority, const float NetworkUpdateFrequency)
{
	bAlwaysRelevant = NetAlwaysRelevant; // For simplicity, making it always relevant.
	SetNetUpdateFrequency(NetworkUpdateFrequency); // High update frequency.
	NetPriority = NetworkPriority; // High priority.
}

void APulseNetReceptor::BeginPlay()
{
	Super::BeginPlay();
	UPulseNetManager::RegisterReceptor(this);
	// Trigger missed events if rep
	if (_replicatedValues.Items.Num() > 0)
	{
		for (const auto& item : _replicatedValues.Items)
		{
			OnItemEvent_raw.Broadcast(item.Entry.Tag, item.Entry, EReplicationEntryOperationType::AddNew);
		}
	}
}

void APulseNetReceptor::OnRep_ReplicatedValues()
{
}

void APulseNetReceptor::AddOrUpdateEntry_Server_Implementation(FPulseNetReplicatedData Value)
{
	if (Value.Tag.IsNone())
		return;
	Value.ServerArrivalOrder = _serverCounter;
	_serverCounter++;
	int32 indexOf = _replicatedValues.Items.IndexOfByPredicate([Value](const FReplicatedItem& Entry)-> bool
	{
		return !Entry.Entry.Tag.IsNone() && Entry.Entry.Tag == Value.Tag;
	});
	auto mes = FString::Printf(TEXT("Pulse Net Receptor: %s Request: %s"), *(indexOf >= 0 ? FString("Update") : FString("Add New")), *Value.ToString());
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
	if (_replicatedValues.Items.IsValidIndex(indexOf))
	{
		_replicatedValues.Items[indexOf].Entry.ServerArrivalOrder = Value.ServerArrivalOrder;
		_replicatedValues.Items[indexOf].Entry.OwnerPlayerID = Value.OwnerPlayerID;
		_replicatedValues.Items[indexOf].Entry.ItemVersion = LatestItemVersion(Value.Tag) + 1;
		_replicatedValues.Items[indexOf].Entry.SoftClassPtr = Value.SoftClassPtr;
		_replicatedValues.Items[indexOf].Entry.NameValue = Value.NameValue;
		_replicatedValues.Items[indexOf].Entry.StringValue = Value.StringValue;
		_replicatedValues.Items[indexOf].Entry.EnumValue = Value.EnumValue;
		_replicatedValues.Items[indexOf].Entry.FlagValue = Value.FlagValue;
		_replicatedValues.Items[indexOf].Entry.IntegerValue = Value.IntegerValue;
		_replicatedValues.Items[indexOf].Entry.DoubleValue = Value.DoubleValue;
		_replicatedValues.Items[indexOf].Entry.Float31Value = Value.Float31Value;
		_replicatedValues.Items[indexOf].Entry.Float32Value = Value.Float32Value;
		_replicatedValues.Items[indexOf].Entry.Float33Value = Value.Float33Value;
		_replicatedValues.MarkItemDirty(_replicatedValues.Items[indexOf]);

		OnItemEvent_raw.Broadcast(Value.Tag, Value, EReplicationEntryOperationType::Update);
		mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Completed Server Update: %s"), *Value.ToString());
	}
	else
	{
		FReplicatedItem NewItem;
		NewItem.Entry = Value;
		NewItem.Entry.ItemVersion = 0;
		_replicatedValues.Items.Add(NewItem);
		_replicatedValues.MarkItemDirty(_replicatedValues.Items.Last());

		OnItemEvent_raw.Broadcast(Value.Tag, Value, EReplicationEntryOperationType::AddNew);
		mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Completed Server Add New: %s"), *Value.ToString());
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
}

void APulseNetReceptor::RemoveEntry_Server_Implementation(FName Tag)
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
		for (int i = datas.Num() - 1; i >= 0; i--)
		{
			OnItemEvent_raw.Broadcast(datas[i].Tag, datas[i], EReplicationEntryOperationType::Remove);
			mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Completed Server Removed %s"), *datas[i].ToString());
			UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(this, mes));
		}
	}
}

void APulseNetReceptor::OnPlayerJoined_Multicast_Implementation(int32 PlayerID)
{
	OnPlayerJoined_raw.Broadcast(PlayerID);
}

void APulseNetReceptor::OnPlayerLeft_Multicast_Implementation(int32 PlayerID)
{
	OnPlayerLeft_raw.Broadcast(PlayerID);
}

bool APulseNetReceptor::QueryNetValues(const TArray<FName>& MessageTags, TArray<FPulseNetReplicatedData>& OutValues, bool bExactMatch) const
{
	if (MessageTags.IsEmpty())
		return false;
	int32 tagFounds = 0;
	for (const auto& Tag : MessageTags)
	{
		if (Tag.IsNone())
			continue;
		for (int i = 0; i < _replicatedValues.Items.Num(); i++)
		{
			if (!_replicatedValues.Items[i].Entry.Tag.ToString().Contains(Tag.ToString()))
				continue;
			if (_replicatedValues.Items[i].Entry.Tag == Tag)
			{
				tagFounds++;
				OutValues.Add(_replicatedValues.Items[i].Entry);
				continue;
			}
			if (bExactMatch)
				continue;
			tagFounds++;
			OutValues.Add(_replicatedValues.Items[i].Entry);
		}
	}
	return tagFounds > 0;
}

int64 APulseNetReceptor::LatestServerArrivalCounter() const
{
	int64 counter = 0;
	for (const auto& item : _replicatedValues.Items)
		if (item.Entry.ServerArrivalOrder > counter)
			counter = item.Entry.ServerArrivalOrder;
	return counter;
}

int64 APulseNetReceptor::LatestItemVersion(const FName Tag) const
{
	int64 version = 0;
	for (const auto& item : _replicatedValues.Items)
		if (item.Entry.Tag == Tag && item.Entry.ItemVersion > version)
			version = item.Entry.ItemVersion;
	return version;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void FReplicatedItem::PreReplicatedRemove(const struct FReplicatedArray& Serializer)
{
	if (Entry.Tag.IsNone())
		return;
	auto mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Could not trigger Remove event for %s : null or invalid array owner"), *Entry.ToString());
	if (auto receptor = Cast<APulseNetReceptor>(Serializer.ArrayOwner.Get()))
	{
		mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Completed Client Removed %s"), *Entry.ToString());
		receptor->OnItemEvent_raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Remove);
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(Serializer.ArrayOwner.Get(), mes));
}

void FReplicatedItem::PostReplicatedAdd(const struct FReplicatedArray& Serializer)
{
	if (Entry.Tag.IsNone())
		return;
	auto mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Could not trigger Add New event for %s : null or invalid array owner"), *Entry.ToString());
	if (auto receptor = Cast<APulseNetReceptor>(Serializer.ArrayOwner.Get()))
	{
		mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Completed Client Add New: %s"), *Entry.ToString());
		receptor->OnItemEvent_raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::AddNew);
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(Serializer.ArrayOwner.Get(), mes));
}

void FReplicatedItem::PostReplicatedChange(const struct FReplicatedArray& Serializer)
{
	if (Entry.Tag.IsNone())
		return;
	auto mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Could not trigger Update event for %s : null or invalid array owner"), *Entry.ToString());
	if (auto receptor = Cast<APulseNetReceptor>(Serializer.ArrayOwner.Get()))
	{
		mes = FString::Printf(TEXT("Pulse Net Receptor: Item >> Completed Client Update: %s"), *Entry.ToString());
		receptor->OnItemEvent_raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Update);
	}
	UE_LOG(LogPulseNetProxy, Log, TEXT("%s"), *UPulseDebugLibrary::DebugNetLog(Serializer.ArrayOwner.Get(), mes));
}

bool FReplicatedArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedItem, FReplicatedArray>(Items, DeltaParms, *this);
}
