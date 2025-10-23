// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseNetworkingModule/IPulseNetObject.h"

#include "Core/PulseSystemLibrary.h"


bool IIPulseNetObject::BindNetworkManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return false;
	TWeakObjectPtr<UObject> w_Ptr = thisObj;
	OnStateObjectRep_Raw = netMgr->OnNetReplication_Raw.AddLambda([w_Ptr](const FName& Tag, const FReplicatedEntry& Value, EReplicationEntryOperationType Operation)-> void
	{
		if (auto obj = w_Ptr.Get()) IIPulseNetObject::Execute_OnNetValueReplicated(obj, Tag, Value, Operation);
	});
	OnStatelessObjectRep_Raw = netMgr->OnNetRPC_Raw.AddLambda([w_Ptr](const FName& Tag, const FReplicatedEntry& Value)-> void
	{
		if (auto obj = w_Ptr.Get())
		{
			TArray<FName> tagParts;
			if (UPulseSystemLibrary::ExtractNametagParts(Tag, tagParts) && (tagParts[0] == "ValueRPC001" || tagParts[0] == "ValueRPC002"))
			{
				if (UPulseNetManager::GetNetNetMode(obj) > 2)
					return;
				const bool isRemove = tagParts[0] == "ValueRPC002";
				tagParts.RemoveAt(0);
				FName newTag = UPulseSystemLibrary::ConstructNametag(tagParts);
				if (!newTag.IsValid())
					return;
				if (isRemove)
					UPulseNetManager::RemoveReplicatedValue(obj, Tag, Value.WeakObjectPtr.Get());
				else
					UPulseNetManager::ReplicateValue(obj, Tag, Value);
				return;
			}
			IIPulseNetObject::Execute_OnNetReceiveBroadcastEvent(obj, Tag, Value);
		}
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

ENetRole IIPulseNetObject::GetNetRole()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return ROLE_None;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return ROLE_None;
	return netMgr->GetNetRole();
}

bool IIPulseNetObject::GetNetHasAuthority()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto netMgr = UPulseNetManager::Get(thisObj);
	if (!netMgr)
		return false;
	return netMgr->HasAuthority();
}

bool IIPulseNetObject::FailedClientNetRepValueToRPCCall()
{
	return false;
}

bool IIPulseNetObject::ReplicateValue_Implementation(const FName Tag, FReplicatedEntry Value)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto opResult = UPulseNetManager::ReplicateValue(thisObj, Tag, Value);
	if (opResult == ENetworkOperationResult::NoAuthority && FailedClientNetRepValueToRPCCall())
	{
		FName finalTag = FName(FString::Printf(TEXT("%s.%s"), *FString("ValueRPC001"), *Tag.ToString()));
		opResult = Execute_BroadcastNetEvent(thisObj, finalTag, Value, true) ? ENetworkOperationResult::Success : opResult;
	}
	return opResult == ENetworkOperationResult::Success;
}

bool IIPulseNetObject::RemoveReplicationTag_Implementation(const FName Tag, UObject* SpecificObject)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto opResult = UPulseNetManager::RemoveReplicatedValue(thisObj, Tag, SpecificObject);
	if (opResult == ENetworkOperationResult::NoAuthority && FailedClientNetRepValueToRPCCall())
	{
		FName finalTag = FName(FString::Printf(TEXT("%s.%s"), *FString("ValueRPC002"), *Tag.ToString()));
		opResult = Execute_BroadcastNetEvent(thisObj, finalTag, FReplicatedEntry().WithObject(SpecificObject), true) ? ENetworkOperationResult::Success : opResult;
	}
	return opResult == ENetworkOperationResult::Success;
}

bool IIPulseNetObject::BroadcastNetEvent_Implementation(const FName Tag, FReplicatedEntry Value, bool Reliable)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	const auto opResult = UPulseNetManager::MakeRPCall(thisObj, Tag, Value, Reliable);
	return opResult == ENetworkOperationResult::Success;
}

bool IIPulseNetObject::TryGetNetRepValues_Implementation(const FName Tag, TArray<FReplicatedEntry>& OutValues)
{
	if (!Tag.IsValid())
		return false;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	UPulseNetManager::TryGetReplicatedValues(thisObj, Tag, OutValues);
	return OutValues.Num() > 0;
}
