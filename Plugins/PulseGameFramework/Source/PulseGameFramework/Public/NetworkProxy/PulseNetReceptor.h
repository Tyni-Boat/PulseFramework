// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetTypes.h"
#include "GameFramework/Actor.h"
#include "PulseNetReceptor.generated.h"


class UPulseNetManager;

/**
 * The Pulse net receptor. owned by the server and replicated to all clients. Contains the replicated array
 */
UCLASS(ClassGroup=(PulseNet))
class PULSEGAMEFRAMEWORK_API APulseNetReceptor : public AActor
{
	GENERATED_BODY()

public:
	
	FOnNetReplication_Raw OnItemEvent_raw;
	FOnNetConnexionEvent_Raw OnPlayerJoined_raw;
	FOnNetConnexionEvent_Raw OnPlayerLeft_raw;

	APulseNetReceptor();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void SetNetParams(const bool NetAlwaysRelevant = true, const float NetworkPriority = 2.8f, const float NetworkUpdateFrequency = 100.0f);

protected:

	// only available on the server, used to order entry adds or updates
	int64 _serverCounter = 0;
	
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedValues)
	FReplicatedArray _replicatedValues;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRep_ReplicatedValues();

public:
	
	/**
	 * @brief Try to Add or Update an entry value with a valid Tag.
	 * @param Value 
	 */
	UFUNCTION(Server, Reliable)
	void AddOrUpdateEntry_Server(FPulseNetReplicatedData Value = FPulseNetReplicatedData());

	/**
	 * @brief Remove an item from the rep list. This will also remove any tag that contains this tag
	 * @param Tag entry tag to remove.
	 */
	UFUNCTION(Server, Reliable)
	void RemoveEntry_Server(FName Tag);

	
	UFUNCTION(NetMulticast, Reliable)
	void OnPlayerJoined_Multicast(int32 PlayerID);
	
	UFUNCTION(NetMulticast, Reliable)
	void OnPlayerLeft_Multicast(int32 PlayerID);

	/**
	 * @brief Query all values that match the tags.
	 * @param MessageTags The tag list to lookup for
	 * @param OutValues The found values
	 * @param bExactMatch return only value who tag match exactly or also include values that tags contains the lookup tags
	 * @return true if any value was found
	 */
	bool QueryNetValues(const TArray<FName>& MessageTags, TArray<FPulseNetReplicatedData>& OutValues, bool bExactMatch = false) const;


	/**
	 * @brief Get the highest Server Arrival Time of available items.
	 * @return 
	 */
	int64 LatestServerArrivalCounter() const;

	/**
	 * @brief Get the highest Version of an item in the available items.
	 * @return 
	 */
	int64 LatestItemVersion(const FName Tag) const;
};
