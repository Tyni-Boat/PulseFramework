// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseNetMgrActor.h"
#include "Core/PulseSubModuleBase.h"
#include "Delegates/IDelegateInstance.h"
#include "Engine/WorldInitializationValues.h"
#include "PulseNetManager.generated.h"


UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ENetworkAuthorizationState: uint8
{
	None = 0 UMETA(Hidden),
	RPC = 1 << 0 UMETA(DisplayName = "Remote Procedure Call"),
	GameplayValues = 1 << 1 UMETA(DisplayName = "Remote Procedure Call"),
};

ENUM_CLASS_FLAGS(ENetworkAuthorizationState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNetAuthorizationStateChanged, ENetworkAuthorizationState, LastValue, ENetworkAuthorizationState, NewValue);

/**
 * Manage all network communication
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseNetManager : public UPulseSubModuleBase
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TObjectPtr<APulseNetMgrActor> _netActor;	
	bool _bActorAlwaysRelevant = true; // For simplicity, making it always relevant.
	float _netActorUpdateFrequency = 100.0f; // High update frequency.
	float _netActorPriority = 2.8f; // High priority.
	ENetworkAuthorizationState _netAuthorizationState = ENetworkAuthorizationState::GameplayValues | ENetworkAuthorizationState::RPC;
	FDelegateHandle _worldInitDelegateHandle_Internal;
	void OnWorldCreation_Internal(UWorld* World, FWorldInitializationValues WorldInitializationValues);

	// Queues
	TQueue<FReplicatedEntry> _pendingReplicatedValues;
	TQueue<FReplicatedEntry> _pendingReliableRPCs;

public:
	ENetworkAuthorizationState GetNetAuth() const;
	ENetMode GetNetMode() const;
	bool HasAuthority() const;
	static bool SetNetActorMgr(APulseNetMgrActor* NetComp, UPulseNetManager*& NetManager);
	void HandleNetHistory();
	
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "PulseNetwork")
	FOnNetReplication OnNetReplication;
	FOnNetReplication_Raw OnNetReplication_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseNetwork")
	FOnPulseNetInit OnNetInit;
	FOnPulseNetInit_Raw OnNetInit_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseNetwork")
	FOnNetRPC OnNetRPC;
	FOnNetRPC_Raw OnNetRPC_Raw;

	UPROPERTY(BlueprintAssignable, Category = "PulseNetwork")
	FOnNetAuthorizationStateChanged OnNetAuthorizationStateChanged;


	static UPulseNetManager* Get(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 2))
	static bool SetNetAuthorization(const UObject* WorldContextObject, UPARAM(meta = (Bitmask, BitmaskEnum = ENetworkAuthorizationState))
	                                int32 NetAuthorization);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject"))
	static ENetworkAuthorizationState GetNetAuthorization(const UObject* WorldContextObject);

	// 0 - Standalone,
	// 1 - DedicatedServer,
	// 2 - ListenServer,
	// 3 - Client,
	// 4 - MAX, (Invalid)
	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject"))
	static int32 GetNetNetMode(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject"))
	static bool GetNetHasAuthority(const UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 2))
	static bool TryGetReplicatedValues(const UObject* WorldContextObject, FName Tag, TArray<FReplicatedEntry>& OutValues, UObject* ForObj = nullptr);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 2))
	static bool ReplicateValue(const UObject* WorldContextObject, FName Tag, FReplicatedEntry Value);
	bool ReplicateValue_Internal(FName Tag, FReplicatedEntry Value, bool fromHistory = false);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 2))
	static bool RemoveReplicatedValue(const UObject* WorldContextObject, FName Tag, UObject* ForObj = nullptr);
	bool RemoveReplicatedValue_Internal(FName Tag, UObject* ForObj = nullptr, bool fromHistory = false);

	UFUNCTION(BlueprintCallable, Category = "PulseNetwork", meta = (WorldContext = "WorldContextObject", AdvancedDisplay = 2))
	static bool MakeRPCall(const UObject* WorldContextObject, FName Tag, FReplicatedEntry Value, bool Reliable = false);
	bool MakeRPCall_Internal(FName Tag, FReplicatedEntry Value, bool Reliable = false, bool fromHistory = false);
};
