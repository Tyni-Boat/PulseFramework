// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetProxy.h"
#include "Core/PulseCoreTypes.h"
#include "Delegates/IDelegateInstance.h"
#include "Engine/WorldInitializationValues.h"
#include "PulseNetManager.generated.h"


USTRUCT()
struct FPendingRepData
{
	GENERATED_BODY()

public:
	FPendingRepData()
	{
	}

	FPendingRepData(FPulseNetReplicatedData data, bool remove = false) : Data(data), bRemove(remove)
	{
	}

	FPulseNetReplicatedData Data = {};
	bool bRemove = false;
};


/**
 * Offer a standardized way to replicate values over network, by using Tag and a replicated net Component.
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseNetManager : public UTickableWorldSubsystem, public IIPulseCore
{
	GENERATED_BODY()

private:
	FVector2D _masterLocalID_NetProxyID = FVector2D(-1);
	UPROPERTY()
	TMap<int32, TObjectPtr<APulseNetProxy>> _localNetProxies;
	UPROPERTY()
	TMap<int32, TObjectPtr<APulseNetProxy>> _remoteNetProxies;
	bool _bNetworkManagerAlwaysRelevant = true;
	float _netUpdateFrequency = 100.0f;
	float _netPriority = 2.8f;
	ENetMode _netMode = NM_Standalone;
	int _checkPlayerEveryXFrame = 5;
	int _currentPlayerCount = 0;
	bool _hadNetInit = false;
	bool _conserveDisconnectedPlayerNetProxies = false;
	TMap<int32, FProxySaved> _disconnectedPlayers;
	int64 _serverQueryArrivalOrder = 0;

	// Queues
	TQueue<FPendingRepData> _pendingReps;

public:
	static bool RegisterProxy(APulseNetProxy* NetProxy);
	static bool UnRegisterProxy(const APulseNetProxy* NetProxy);
	static int64 ConsumeRequestArrivalOrder(const APulseNetProxy* NetProxy);
	void HandleNetHistory();
	virtual TStatId GetStatId() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual void Deinitialize() override;

public:
	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetReplication OnNetReplication;
	FOnNetReplication_Raw OnNetReplication_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnPulseNetInit OnNetInit;
	FOnPulseNetInit_Raw OnNetInit_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetConnexionEvent OnNetPlayerConnexion;
	FOnNetConnexionEvent_Raw OnNetPlayerConnexion_Raw;

	static UPulseNetManager* Get(const UObject* WorldContext);

	// Get the local Id the net proxy registered as the master one.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	int32 GetNetLocalID() const;

	// Get the local Id of a controller
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	int32 GetControllerNetLocalID(AController* Controller) const;

	// Get the local Id of a controlled Pawn
	UFUNCTION(BlueprintPure, Category = "PulseCore|Network")
	int32 GetPawnNetLocalID(APawn* Pawn) const;

	// Get every replicated value associated with this Tag
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Network", meta = (AdvancedDisplay = 1, AutoCreateRefTerm = "Tags, FromSpecificPlayerIds"))
	bool QueryNetValue(const TArray<FName>& Tags, FPulseNetReplicatedData& OutValue, bool bIncludeChildValues, TArray<FPulseNetReplicatedData>& OutChildValues) const;

	// Replicate value by tag over. Work only when called on server.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Network", meta = (AdvancedDisplay = 2))
	void ReplicateValue(FName Tag, FPulseNetReplicatedData Value);

	// Remove and stop replicating value by tag. Work only when called on server.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Network", meta = ( AdvancedDisplay = 2))
	bool RemoveReplicatedValue(FName Tag);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	void ReplicateValue_Internal(FName Tag, FPulseNetReplicatedData Value, bool fromHistory = false);
	bool RemoveReplicatedValue_Internal(FName Tag, bool fromHistory = false);

	// Create a new Net proxy on server. 0-Unable to create proxy, 1-Created local proxy, 2-created Remote proxy
	int32 NewNetProxy_Internal(const int32& PlayerIndex) const;
	TObjectPtr<APulseNetProxy> GetMasterProxy_Internal() const;
	bool IsPlayerRegistered_Internal(const int32 PlayerID) const;
	// 0- Unregistered proxy, 1- local proxy, 2- Remote proxy
	int32 PlayerRegistrationType_Internal(const int32 PlayerID) const;
	void MonitorPlayers_Internal();
	bool AddNetProxy_Internal(const int32 PlayerID, APulseNetProxy* NetProxy);
	bool RemoveNetProxy_Internal(const int32 PlayerID);
	bool SetNewMasterNetProxy_Internal(APulseNetProxy* NetProxy);
	bool GetDisconnectedPlayerIds(TArray<int32>& OutPlayerIDs) const;
};

/**
 * @brief Utility object to easily handle pulse Net Manager events in blueprint.
 */
UCLASS(NotBlueprintable, BlueprintType)
class UPulseNetEventListener : public UObject
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

protected:
	UFUNCTION() void OnNetPlayerConnexion_Func(int32 PlayerID, bool bIsDisconnection, bool bIsLocalPlayer);
	UFUNCTION() void OnNetReplication_Func(FName Tag, FPulseNetReplicatedData Value, EReplicationEntryOperationType Operation);
	UFUNCTION() void OnNetInit_Func();

public:

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetReplication OnNetReplication;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnPulseNetInit OnNetInit;
	
	UPROPERTY(BlueprintAssignable, Category = "PulseCore|Network")
	FOnNetConnexionEvent OnNetPlayerConnexion;
	
};
