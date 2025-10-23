// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PulseModuleBase.h"
#include "PulseCoreModule.generated.h"


class UPulseInventoryManager;
class UPulseTimeOfDayManager;
class UPulseSaveManager;
class UOperationModifierSubModule;
class UPulseNetManager;

UCLASS(config = Game, defaultconfig, Category = Settings, meta = (DisplayName = "Pulse Core Config"))
class UCoreProjectSetting : public UDeveloperSettings
{
	GENERATED_BODY()

public:
#pragma region Pooling System

	UPROPERTY(EditAnywhere, Config, Category = "Pooling System")
	int32 GlobalPoolLimit = 100;

#pragma endregion

#pragma region Time of Day

	UPROPERTY(EditAnywhere, Config, Category = "Time of Day")
	FDateTime DefaultGameDate;

	// Use a non-null and non-empty key to replicate game time over network.
	UPROPERTY(EditAnywhere, Config, Category = "Time of Day")
	bool bReplicateDateTime = false;
	
	UPROPERTY(EditAnywhere, Config, Category = "Time of Day")
	bool bSaveAndLoadGameTime = false;

	UPROPERTY(EditAnywhere, Config, Category = "Time of Day")
	bool bTickGameTime = true;

	UPROPERTY(EditAnywhere, Config, Category = "Time of Day")
	bool bUseSystemTimeNowOnInitialize = true;

#pragma endregion

#pragma region Save System

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	bool bUseAutoSave = true;

	// Verify the save cache upon reading. Slower but can prevent one to read an object that'd been modified by a previous reader.
	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	bool bUseSaveCacheInvalidationOnRead = true;

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	float AutoSaveIntervalInSeconds = 300;

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	int32 ManualSaveSlots = 5;

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	int32 PerSlotTambour = 3;

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	FName AutoSaveSlotBaseName = "AutoSave";

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	FName ManualSaveSlotBaseName = "ManualSave";

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	int32 SaveUserCount = 1;

#pragma endregion

#pragma region Pulse Asset Management

	// The Main Directory Where newly created Pulse Assets will be created and Query from at Editor Time.
	// Useful when using with an editor window to manage assets.
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management")
	FDirectoryPath MainGameAssetSavePath;
	
#pragma endregion

#pragma region Buffing System

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)")
	bool bSaveAndLoadActiveBuffs = true;

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)")
	bool bReplicateActiveBuffs = false;

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)")
	int32 BuffHandlePoolingLimit = 1000;

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)", meta=(ToolTip = "Veru experimental feature. use with caution"))
	bool bAutoApplyGameplayEffects = false;
	
#pragma endregion

#pragma region Network Manager

	UPROPERTY(EditAnywhere, Config, Category = "Network Manager|Replication")
	bool bNetworkManagerAlwaysRelevant = true;
	
	UPROPERTY(EditAnywhere, Config, Category = "Network Manager|Replication")
	float NetUpdateFrequency = 100.0f;
	
	UPROPERTY(EditAnywhere, Config, Category = "Network Manager|Replication")
	float NetPriority = 2.8f;
	
#pragma endregion

#pragma region Inventory Manager

	UPROPERTY(EditAnywhere, Config, Category = "Pulse Inventory")
	bool bReplicateInventory = false;
	
	UPROPERTY(EditAnywhere, Config, Category = "Pulse Inventory")
	bool bSaveInventory = false;
	
#pragma endregion
};


/**
 * The core Module of the framework
 */
UCLASS(Within = GameInstance)
class PULSEGAMEFRAMEWORK_API UPulseCoreModule : public UPulseModuleBase, public ConfigurableModule<UCoreProjectSetting>
{
	GENERATED_BODY()

public:
	virtual UCoreProjectSetting* GetProjectConfig() const override;
	virtual FName GetModuleName() const override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;

protected:
	virtual TArray<TSubclassOf<UPulseSubModuleBase>> GetSubmodulesTypes() const override;

public:
	UFUNCTION(BlueprintPure, Category = "Pulse Core")
	UPulseSaveManager* GetSaveManager();

	UFUNCTION(BlueprintPure, Category = "Pulse Core")
	UPulseTimeOfDayManager* GetTimeOfDayManager();

	UFUNCTION(BlueprintPure, Category = "Pulse Core")
	UOperationModifierSubModule* GetBuffManager();

	UFUNCTION(BlueprintPure, Category = "Pulse Core")
	UPulseNetManager* GetNetManager();

	UFUNCTION(BlueprintPure, Category = "Pulse Core")
	UPulseInventoryManager* GetInventoryManager();
};
