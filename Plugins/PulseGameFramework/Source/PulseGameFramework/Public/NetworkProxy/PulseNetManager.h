// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetEmitter.h"
#include "PulseNetReceptor.h"
#include "Core/PulseCoreTypes.h"
#include "Delegates/IDelegateInstance.h"
#include "Engine/WorldInitializationValues.h"
#include "PulseNetManager.generated.h"


/**
 * Offer a standardized way to replicate values over network, by using Tags.
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseNetManager : public UTickableWorldSubsystem, public IIPulseCore
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TObjectPtr<APulseNetEmitter> _emitter;
	UPROPERTY()
	TObjectPtr<APulseNetReceptor> _receptor;
	FDelegateHandle PostLoginHandle;
	FDelegateHandle LogoutHandle;
	FDelegateHandle MessageRepHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle LeftHandle;
	
	bool _bNetworkManagerAlwaysRelevant = true;
	float _netUpdateFrequency = 100.0f;
	float _netPriority = 2.8f;
	bool wasInitialized = false;

public:
	static bool RegisterEmitter(APulseNetEmitter* Emitter);
	static bool RegisterReceptor(APulseNetReceptor* Receptor);
	
	virtual TStatId GetStatId() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual void Deinitialize() override;

protected:

	UFUNCTION()
	void OnRep_ReplicatedValue(FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType Operation);
	
	UFUNCTION()
	void OnRep_PlayerPostLogin(AGameModeBase* GameMode, APlayerController* JoiningPlayer);
	
	UFUNCTION()
	void OnRep_PlayerLogout(AGameModeBase* GameMode, AController* LeavingPlayer);

	UFUNCTION()
	void OnRep_PlayerJoined(int32 playerID);

	UFUNCTION()
	void OnRep_PlayerLeft(int32 playerID);
	
public:
	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetReplication OnNetMessageReceived;
	FOnNetReplication_Raw OnNetMessageReceived_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnPulseNetInit OnNetInitialization;
	FOnPulseNetInit_Raw OnNetInitialization_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetConnexionEvent OnNetPlayerJoined;
	FOnNetConnexionEvent_Raw OnNetPlayerJoined_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetConnexionEvent OnNetPlayerLeft;
	FOnNetConnexionEvent_Raw OnNetPlayerLeft_Raw;

	static UPulseNetManager* Get(const UObject* WorldContext);
	
	static APulseNetReceptor* GetReceptor(const UObject* WorldContext);

	// Get the player Id of the emitter used to send net messages
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	int32 GetLocalPLayerID() const;

	// Get the player Id of the emitter used to send net messages
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	void GetLocalCapabilities(bool& bCanBroadcast, bool& bCanReceive) const;
	
	/**
	 * @brief Get every replicated value associated with this Tag
	 * @param Tags The tag to lookup for
	 * @param bIncludeDerivedTags include any derived tag. eg: Tag "foo.random" will query for itself as well as "foo.random.itemA","foo.random.itemB" and so on 
	 * @param OutValues The resulting values. by default is sorted as [Tag1, Tag1.A, Tag1.B, Tag2, Tag2.A, ...]
	 * @param bSortByArrivalDate sort the resulting values by arrival date, from the oldest to the newest.
	 * @return true if any value was found.
	 */
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Network", meta = (AdvancedDisplay = 1, AutoCreateRefTerm = "Tags, FromSpecificPlayerIds"))
	bool QueryNetMessage(const TArray<FName>& Tags, bool bIncludeDerivedTags, TArray<FPulseNetReplicatedData>& OutValues, bool bSortByArrivalDate = true) const;

	/**
	 * @brief Broadcast a net message.
	 * @param Tag Message tag
	 * @param Value Message content
	 * @return false if the net manager is missing the emitter.
	 */
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Network", meta = (AdvancedDisplay = 2))
	bool BroadcastNetMessage(FName Tag, FPulseNetReplicatedData Value);

	/**
	 * @brief Remove a net message entry.
	 * @param Tag Message tag to remove
	 * @return false if the net manager is missing the emitter.
	 */
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Network", meta = ( AdvancedDisplay = 2))
	bool DeleteNetMessage(FName Tag);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	bool SpawnReceptor_Internal();
	void SpawnLocalEmitters_Internal();
};
