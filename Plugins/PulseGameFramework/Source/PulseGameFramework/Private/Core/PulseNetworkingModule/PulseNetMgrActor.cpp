// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseNetworkingModule/PulseNetMgrActor.h"
#include "Core/PulseNetworkingModule/PulseNetManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"


void FReplicatedItem::PreReplicatedRemove(const struct FReplicatedArray& Serializer)
{
	if (!Entry.IsValidNetEntry())
		return;
	if (auto repActor = Cast<APulseNetMgrActor>(Serializer.OwningObject))
	{
		if (repActor->_manager.Get())
		{
			repActor->_manager->OnNetReplication_Raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Remove);
			repActor->_manager->OnNetReplication.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Remove);
		}
	}
}

void FReplicatedItem::PostReplicatedAdd(const struct FReplicatedArray& Serializer)
{
	if (!Entry.IsValidNetEntry())
		return;
	if (auto repActor = Cast<APulseNetMgrActor>(Serializer.OwningObject))
	{
		if (repActor->_manager.Get())
		{
			repActor->_manager->OnNetReplication_Raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::AddNew);
			repActor->_manager->OnNetReplication.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::AddNew);
		}
	}
}

void FReplicatedItem::PostReplicatedChange(const struct FReplicatedArray& Serializer)
{
	if (!Entry.IsValidNetEntry())
		return;
	if (auto repActor = Cast<APulseNetMgrActor>(Serializer.OwningObject))
	{
		if (repActor->_manager.Get())
		{
			repActor->_manager->OnNetReplication_Raw.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Update);
			repActor->_manager->OnNetReplication.Broadcast(Entry.Tag, Entry, EReplicationEntryOperationType::Update);
		}
	}
}

bool FReplicatedArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	return FFastArraySerializer::FastArrayDeltaSerialize<FReplicatedItem, FReplicatedArray>(Items, DeltaParms, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


APulseNetMgrActor::APulseNetMgrActor()
{
	bReplicates = true;
	bAlwaysRelevant = true; // For simplicity, making it always relevant.
	SetNetUpdateFrequency(100.0f); // High update frequency.
	NetPriority = 2.8f; // High priority.
}

void APulseNetMgrActor::BeginPlay()
{
	Super::BeginPlay();
	UPulseNetManager* mgr = nullptr;
	_replicatedValues.OwningObject = this;
	if (!UPulseNetManager::SetNetActorMgr(this, mgr))
	{
		Destroy();
		return;
	}
	_manager = mgr;
	_manager->HandleNetHistory();
	_manager->OnNetInit_Raw.Broadcast();
	_manager->OnNetInit.Broadcast();
}

void APulseNetMgrActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(APulseNetMgrActor, _replicatedValues);
}

void APulseNetMgrActor::OnRep_ReplicatedValues()
{
}

void APulseNetMgrActor::PostMulticastRPC(FReplicatedEntry Value) const
{
	if (!_manager.Get())
		return;
	_manager->OnNetRPC_Raw.Broadcast(Value.Tag, Value);
	_manager->OnNetRPC.Broadcast(Value.Tag, Value);
}

void APulseNetMgrActor::ReplicateValue_Implementation(FReplicatedEntry Value)
{
	if (!Value.IsValidNetEntry())
		return;
	int32 indexOf = _replicatedValues.Items.IndexOfByPredicate([Value](const FReplicatedItem& Entry)-> bool
	{
		return Entry.Entry.IsValidNetEntry() && Entry.Entry.Tag == Value.Tag && Entry.Entry.WeakObjectPtr == Value.WeakObjectPtr;
	});
	if (_replicatedValues.Items.IsValidIndex(indexOf))
	{
		_replicatedValues.Items[indexOf].Entry.WeakObjectPtr = Value.WeakObjectPtr;
		_replicatedValues.Items[indexOf].Entry.NameValue = Value.NameValue;
		_replicatedValues.Items[indexOf].Entry.EnumValue = Value.EnumValue;
		_replicatedValues.Items[indexOf].Entry.FlagValue = Value.FlagValue;
		_replicatedValues.Items[indexOf].Entry.IntegerValue = Value.IntegerValue;
		_replicatedValues.Items[indexOf].Entry.DoubleValue = Value.DoubleValue;
		_replicatedValues.Items[indexOf].Entry.Float31Value = Value.Float31Value;
		_replicatedValues.Items[indexOf].Entry.Float32Value = Value.Float32Value;
		_replicatedValues.Items[indexOf].Entry.Float33Value = Value.Float33Value;
		_replicatedValues.MarkItemDirty(_replicatedValues.Items[indexOf]);
	}
	else
	{
		FReplicatedItem NewItem;
		NewItem.Entry = Value;
		_replicatedValues.Items.Add(NewItem);
		_replicatedValues.MarkArrayDirty();
	}
}

void APulseNetMgrActor::RemoveReplicatedItem_Implementation(FName Tag, bool IncludeChildValues, UObject* objPtr)
{
	if (Tag.IsNone())
		return;
	TArray<int32> indexes;
	for (int i = 0; i < _replicatedValues.Items.Num(); i++)
	{
		if (!IncludeChildValues)
		{
			if (indexes.Num() > 0)
				break;
			if (_replicatedValues.Items[i].Entry.Tag != Tag)
				continue;
		}
		else
		{
			if (!_replicatedValues.Items[i].Entry.Tag.ToString().Contains(Tag.ToString()))
				continue;
		}
		if(objPtr && _replicatedValues.Items[i].Entry.WeakObjectPtr.Get() != objPtr)
			continue;
		indexes.Add(i);
	}
	for (int i = indexes.Num() - 1; i >= 0; i--)
	{
		_replicatedValues.Items.RemoveAt(indexes[i]);
	}
	if (indexes.Num() > 0)
		_replicatedValues.MarkArrayDirty();
}

void APulseNetMgrActor::ReliableServerRPC_Implementation(FReplicatedEntry Value)
{
	ReliableMulticastRPC(Value);
}

void APulseNetMgrActor::UnreliableServerRPC_Implementation(FReplicatedEntry Value)
{
	UnreliableMulticastRPC(Value);
}

void APulseNetMgrActor::ReliableMulticastRPC_Implementation(FReplicatedEntry Value)
{
	PostMulticastRPC(Value);
}

void APulseNetMgrActor::UnreliableMulticastRPC_Implementation(FReplicatedEntry Value)
{
	PostMulticastRPC(Value);
}


bool APulseNetMgrActor::GetReplicatedValues(FName Tag, TArray<FReplicatedEntry>& Values, UObject* objPtr)
{
	if (Tag.IsNone())
		return false;
	Values.Empty();
	for (int i = 0; i < _replicatedValues.Items.Num(); i++)
	{
		if (!_replicatedValues.Items[i].Entry.Tag.ToString().Contains(Tag.ToString()))
			continue;
		if (objPtr && _replicatedValues.Items[i].Entry.WeakObjectPtr.Get() != objPtr)
			continue;
		Values.Add(_replicatedValues.Items[i].Entry);
	}
	return Values.Num() > 0;
}
