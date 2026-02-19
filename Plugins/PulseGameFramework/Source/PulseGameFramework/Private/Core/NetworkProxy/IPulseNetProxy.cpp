// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/NetworkProxy/IPulseNetProxy.h"



bool IIPulseNetProxy::BindNetworkManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return false;
	TWeakObjectPtr<UObject> w_Ptr = thisObj;
	OnValueReplication_Raw = netMgr->OnNetReplication_Raw.AddLambda([w_Ptr](const FName& Tag, const FPulseNetReplicatedData& Value, EReplicationEntryOperationType Operation)-> void
	{
		if (auto obj = Cast<IIPulseNetProxy>(w_Ptr.Get())) obj->OnNetValueReplicated(Tag, Value, Operation);
	});
	OnNetInit_Raw = netMgr->OnNetInit_Raw.AddLambda([w_Ptr]()-> void
	{
		if (auto obj = Cast<IIPulseNetProxy>(w_Ptr.Get())) obj->OnNetInit();
	});
	return true;
}

bool IIPulseNetProxy::UnbindNetworkManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return false;
	if (OnNetInit_Raw.IsValid())
		netMgr->OnNetInit_Raw.Remove(OnNetInit_Raw);
	if (OnValueReplication_Raw.IsValid())
		netMgr->OnNetReplication_Raw.Remove(OnValueReplication_Raw);
	OnNetInit_Raw.Reset();
	OnValueReplication_Raw.Reset();
	return true;
}


bool IIPulseNetProxy::ReplicateValue_Implementation(const FName Tag, FPulseNetReplicatedData Value)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	if (auto mgr = UPulseNetManager::Get(thisObj))
	{
		mgr->ReplicateValue(Tag, Value);
		return true;
	}
	return false;
}

bool IIPulseNetProxy::RemoveReplicationTag_Implementation(const FName Tag)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	if (auto mgr = UPulseNetManager::Get(thisObj))
	{
		mgr->RemoveReplicatedValue(Tag);
		return true;
	}
	return false;
}

bool IIPulseNetProxy::QueryNetValue_Implementation(const TArray<FName>& Tags, FPulseNetReplicatedData& OutValue, bool bIncludeChildValues, TArray<FPulseNetReplicatedData>& OutChildValues)
{
	if (!Tags.IsEmpty())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	if (auto mgr = UPulseNetManager::Get(thisObj))
	{
		mgr->QueryNetValue(Tags, OutValue, bIncludeChildValues, OutChildValues);
		return true;
	}
	return false;
}

void IIPulseNetProxy::OnNetInit()
{
}

void IIPulseNetProxy::OnNetValueReplicated(const FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType OpType)
{
}
