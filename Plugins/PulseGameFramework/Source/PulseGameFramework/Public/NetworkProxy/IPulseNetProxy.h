// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetManager.h"
#include "UObject/Interface.h"
#include "IPulseNetProxy.generated.h"


class UPulseNetManager;

UINTERFACE(MinimalAPI, Blueprintable)
class UIPulseNetProxy : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface to replicate values by PulseNetManager using Tags.
 */
class PULSEGAMEFRAMEWORK_API IIPulseNetProxy
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnNetMessageReceived(const FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType OpType);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnNetInitialization(bool bCanReceive, bool bCanEmit);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnNetPlayerJoined(const int32 PlayerId);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnNetPlayerLeft(const int32 PlayerId);
};
