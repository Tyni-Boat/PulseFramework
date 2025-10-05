// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetManager.h"
#include "UObject/Interface.h"
#include "Core/CoreConcepts.h"
#include "Iris/ReplicationSystem/NetBlob/NetBlob.h"
#include "IPulseNetObject.generated.h"


class UPulseNetManager;

UINTERFACE(BlueprintType, Blueprintable)
class UIPulseNetObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface to automatically replicate by PulseNetManager using keys.
 * Remember to Unbind on object destruction to avoid potential crashes.
 */
class PULSEGAMEFRAMEWORK_API IIPulseNetObject
{
	GENERATED_BODY()
protected:
	FDelegateHandle OnStateObjectRep_Raw;
	FDelegateHandle OnStatelessObjectRep_Raw;
	FDelegateHandle OnNetInit_Raw;
public:

	bool BindNetworkManager();
	
	bool UnbindNetworkManager();

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseNetwork|Replication", meta=(AdvancedDisplay = 2))
	bool ReplicateValue(const FName Tag, FReplicatedEntry Value);
	bool ReplicateValue_Implementation(const FName Tag, FReplicatedEntry Value);

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseNetwork|Replication", meta=(AdvancedDisplay = 2))
	bool RemoveReplicationTag(const FName Tag, UObject* SpecificObject = nullptr);
	bool RemoveReplicationTag_Implementation(const FName Tag, UObject* SpecificObject = nullptr);

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseNetwork|Replication", meta=(AdvancedDisplay = 2))
	bool BroadcastNetEvent(const FName Tag, FReplicatedEntry Value, bool Reliable = false);
	bool BroadcastNetEvent_Implementation(const FName Tag, FReplicatedEntry Value, bool Reliable = false);

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseNetwork|Replication", meta=(AdvancedDisplay = 2))
	bool TryGetNetRepValues(const FName Tag, TArray<FReplicatedEntry>& OutValues);
	bool TryGetNetRepValues_Implementation(const FName Tag, TArray<FReplicatedEntry>& OutValues);

	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PulseNetwork|Replication")
	void OnNetInit();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PulseNetwork|Replication")
	void OnNetValueReplicated(const FName Tag, FReplicatedEntry Value, EReplicationEntryOperationType OpType);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PulseNetwork|Replication")
	void OnNetReceiveBroadcastEvent(const FName Tag, FReplicatedEntry Value);
};
