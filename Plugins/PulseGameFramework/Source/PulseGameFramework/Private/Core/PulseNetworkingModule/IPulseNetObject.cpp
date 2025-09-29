// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseNetworkingModule/IPulseNetObject.h"


bool IIPulseNetObject::BindNetworkManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return false;
	TWeakObjectPtr<UObject> w_Ptr = thisObj;
	OnStateObjectRep_Raw = netMgr->OnNetReplication_Raw.AddLambda([w_Ptr](const FGameplayTag& Tag, const FReplicatedEntry& Value, EReplicationEntryOperationType Operation)-> void
	{
		if (auto obj = w_Ptr.Get()) IIPulseNetObject::Execute_OnNetValueReplicated(obj, Tag, Value, Operation);
	});
	OnStatelessObjectRep_Raw = netMgr->OnNetRPC_Raw.AddLambda([w_Ptr](const FGameplayTag& Tag, const FReplicatedEntry& Value)-> void
	{
		if (auto obj = w_Ptr.Get()) IIPulseNetObject::Execute_OnNetReceiveBroadcastEvent(obj, Tag, Value);
	});
	OnNetInit_Raw = netMgr->OnNetInit_Raw.AddLambda([w_Ptr]()-> void
	{
		if (auto obj = w_Ptr.Get()) IIPulseNetObject::Execute_OnNetInit(obj);
	});
	return true;
}

bool IIPulseNetObject::UnbindNetworkManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return false;
	if (OnNetInit_Raw.IsValid())
		netMgr->OnNetInit_Raw.Remove(OnNetInit_Raw);
	if (OnStateObjectRep_Raw.IsValid())
		netMgr->OnNetReplication_Raw.Remove(OnStateObjectRep_Raw);
	if (OnStatelessObjectRep_Raw.IsValid())
		netMgr->OnNetRPC_Raw.Remove(OnStatelessObjectRep_Raw);
	OnNetInit_Raw.Reset();
	OnStateObjectRep_Raw.Reset();
	OnStatelessObjectRep_Raw.Reset();
	return true;
}

bool IIPulseNetObject::ReplicateValue_Implementation(const FGameplayTag Tag, FReplicatedEntry Value)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	return UPulseNetManager::ReplicateValue(thisObj, Tag, Value);
}

bool IIPulseNetObject::BroadcastNetEvent_Implementation(const FGameplayTag Tag, FReplicatedEntry Value, bool Reliable)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	return UPulseNetManager::MakeRPCall(thisObj, Tag, Value, Reliable);
}

bool IIPulseNetObject::TryGetNetRepValue_Implementation(const FGameplayTag Tag, TArray<FReplicatedEntry>& OutValues, bool IncludeChildren)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	if (IncludeChildren)
		UPulseNetManager::TryGetReplicatedValues(thisObj, Tag, OutValues);
	FReplicatedEntry keyVal;
	if (UPulseNetManager::TryGetReplicatedValue(thisObj, Tag, keyVal))
		OutValues.Insert(keyVal, 0);
	return OutValues.Num() > 0;
}
