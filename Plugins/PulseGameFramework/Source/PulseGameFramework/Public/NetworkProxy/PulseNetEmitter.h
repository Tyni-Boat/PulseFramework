// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetTypes.h"
#include "GameFramework/Actor.h"
#include "PulseNetEmitter.generated.h"



/**
 * The Pulse net Emitter. owned by the Autonomous proxy/Listen server and replicated only to autonomous and server.
 */
UCLASS(ClassGroup=(PulseNet))
class PULSEGAMEFRAMEWORK_API APulseNetEmitter : public AActor
{
	GENERATED_BODY()
	
private:

	TWeakObjectPtr<class APlayerState> _cachedPs;
	
public:
	TMap<FName, FPulseNetReplicatedData> _PendingNetMessages;

	// The temporary player ID before a player state is assigned.
	UPROPERTY(VisibleAnywhere, Replicated, Category="PulseCore|Network")
	int32 TempPLayerID = -1;

	// Sets default values for this actor's properties
	APulseNetEmitter();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void SetNetParams(const float NetworkPriority = 2.8f, const float NetworkUpdateFrequency = 100.0f);

protected:

	virtual void BeginPlay() override;

public:
	/**
	 * @return The Player Id of the player state owning this emitter
	 */
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	int32 GetPlayerID() const;

	/**
	 * @brief Get the list of Broadcasted net messages waiting for confirmation. 
	 * @return true if any pending message was found
	 */
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	bool GetPendingMessages(TArray<FPulseNetReplicatedData>& OutPendingMessages) const;

	/**
	 * @brief Send a message to the server, that will be broadcasted to every receptor
	 * @param Value the actual coded net message
	 */
	UFUNCTION(BlueprintCallable, Category="PulseCore|Network")
	void BroadcastNetMessage(FPulseNetReplicatedData Value);

	/**
	 * @brief Tel the server to remove a net message entry
	 * @param Tag the message tag
	 */
	UFUNCTION(BlueprintCallable, Category="PulseCore|Network")
	void DeleteNetEntry(const FName Tag);

	/**
	 * @brief Send a message to the server, that will be broadcasted to every receptor
	 * @param Value the actual coded net message
	 */
	UFUNCTION(Server, Reliable)
	void BroadcastNetMessage_Server(FPulseNetReplicatedData Value);

	/**
	 * @brief Tel the server to remove a net message entry
	 * @param Tag the message tag
	 */
	UFUNCTION(Server, Reliable)
	void DeleteNetEntry_Server(const FName Tag);

	UFUNCTION()
	void OnRep_ReplicatedValue(FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType Operation);
};
