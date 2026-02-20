// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetManager.h"
#include "UObject/Interface.h"
#include "IPulseNetProxy.generated.h"


class UPulseNetManager;

UINTERFACE(NotBlueprintType, NotBlueprintable)
class UIPulseNetProxy : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface to replicate values by PulseNetManager using Tags.
 * Replication only work from Serer to clients. You'll have to use RPC to execute replication on server
 * Remember to Unbind on object destruction to avoid potential crashes.
 */
class PULSEGAMEFRAMEWORK_API IIPulseNetProxy
{
	GENERATED_BODY()
protected:
	FDelegateHandle OnValueReplication_Raw;
	FDelegateHandle OnNetInit_Raw;
	FDelegateHandle OnNetPlayerConnexionEvent_Raw;
	virtual void OnNetConnexionEvent_CallBack(const int32 PlayerId, bool bIsDisconnection, bool bIsLocalPlayer);
public:

	bool BindNetworkManager();
	
	bool UnbindNetworkManager();

	bool ReplicateValue_Implementation(const FName Tag, FPulseNetReplicatedData Value);

	bool RemoveReplicationTag_Implementation(const FName Tag);

	bool QueryNetValue_Implementation(const TArray<FName>& Tags, FPulseNetReplicatedData& OutValue, bool bIncludeChildValues, TArray<FPulseNetReplicatedData>& OutChildValues);
	
	virtual void OnNetInit();
	
	virtual void OnNetValueReplicated(const FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType OpType);
	
	virtual void OnPlayerConnection(const int32 PlayerId, bool bIsLocalPlayer);
	
	virtual void OnPlayerDisconnection(const int32 PlayerId, bool bIsLocalPlayer);
};
