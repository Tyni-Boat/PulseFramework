// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Core/PulseSubModuleBase.h"
#include "Core/CoreTypes.h"
#include "Core/PoolingSubModule/PulsePoolingManager.h"
#include "Core/SaveGameSubModule/PulseSaveManager.h"
#include "OperationModifierSubModule.generated.h"


#pragma region SubTypes

UENUM()
enum class EBuffCategoryType : uint8
{
	Actor = 0,
	PlayerCharacter = 1,
	PlayerController = 2,
	NPCCharacter = 3,
	User = 4,
};

UENUM(BlueprintType)
enum class EBuffStackingType : uint8
{
	None = 0 UMETA(DisplayName = "No Stacking, Select the highest value"),
	AdditiveValues = 1 UMETA(DisplayName = "Add all Values Together before Applying"),
	AverageValues = 2 UMETA(DisplayName = "Get the average from all Values before Applying"),
};

// Buff operations. Need to be synchronized with the server for multiplayer games
USTRUCT(blueprintType)
struct FBuffOperationModifier
{
	GENERATED_BODY()

public:
	// The unique hash Id of this modifier
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FString UID;

	// The ID of the character owning this buff if any
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FPrimaryAssetId CharacterID;

	// The ID of the User owning this buff if any
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FName UserID;

	// The player Index of the player owning this buff if any
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	int32 PlayerIndex = -1;

	// The Context of the operation. eg: Combat.Damage.Penalty.Resistance.Poison,
	// Combat.Damage.Boost.Attack.Physical, Shop.Buy.Boost.Discount.Weapon.Sword.All
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	FGameplayTag Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	ENumericOperator Operator = ENumericOperator::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EBuffStackingType StackingType = EBuffStackingType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float Value = 0;

	// Is the value a percentage of the input?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	bool bValueAsPercentage = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	double LifeTimeLeft = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	double Duration = -1;

	// The gameplay effect that will be activated. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	TSubclassOf<UGameplayEffect> GameplayAbilityEffect = nullptr;

	// The gameplay effect's level
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	float GameplayAbilityEffectLevel = 0;


	float GetModifierDiff(const float Input, FGameplayTag InContext) const;

	bool IsValidModifier() const;
};

// Package Modifiers
USTRUCT(BlueprintType)
struct FBuffOperationPack
{
	GENERATED_BODY()

public:

	inline FBuffOperationPack(){}
	inline FBuffOperationPack(const TArray<FBuffOperationModifier>& InModifiers): Modifiers(InModifiers){}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	TArray<FBuffOperationModifier> Modifiers;

	float GetPackDiff(const float Input, FGameplayTag Context, TMap<EBuffStackingType, float>* PerStackingDiff = nullptr) const;
	bool UpdateModifierPack(const float DeltaTime, TArray<FBuffOperationModifier>& ExpiredModifiers);
	bool AddModifier(FBuffOperationModifier Modifier);
	bool RemoveModifier(const FString& ModifierUID, FBuffOperationModifier& OutModifier);
};

// Modifiers Add Command
USTRUCT()
struct FBuffAddOperationCommand
{
	GENERATED_BODY()

public:
	FName Category;
	FPrimaryAssetId CharacterID;
	FBuffOperationModifier Modifier;
	TWeakObjectPtr<AActor> Actor;
	EBuffCategoryType catType;
};

// Modifiers Remove Command
USTRUCT()
struct FBuffRemoveOperationCommand
{
	GENERATED_BODY()

public:
	inline FBuffRemoveOperationCommand() : UID(""), CatType(EBuffCategoryType::Actor), Actor(nullptr)
	{
	}

	inline FBuffRemoveOperationCommand(const FString& InUID, EBuffCategoryType InCat = EBuffCategoryType::Actor, AActor* InActor = nullptr) : UID(InUID), CatType(InCat),
		Actor(InActor)
	{
	}

	FString UID;
	EBuffCategoryType CatType = EBuffCategoryType::Actor;
	TWeakObjectPtr<AActor> Actor;

	FName Category;
	FBuffOperationModifier Modifier;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUserBuffModifierChanged, FName, Category, FBuffOperationModifier, ModifierBuff, bool, hadBeenRemoved);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGlobalBuffModifierChanged, FName, Category, FBuffOperationModifier, ModifierBuff, bool, hadBeenRemoved);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerBuffModifierChanged, FName, Category, FBuffOperationModifier, ModifierBuff, bool, hadBeenRemoved);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNpcBuffModifierChanged, FName, Category, FBuffOperationModifier, ModifierBuff, bool, hadBeenRemoved);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnActorBuffModifierChanged, TWeakObjectPtr<AActor>, Actor, FName, Category, FBuffOperationModifier, ModifierBuff, bool,
                                              hadBeenRemoved);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHandleReleased);


UCLASS()
class USaveBuffOperationManagerData : public UObject
{
	GENERATED_BODY()

public:
	// Buffs to the global user. resist between sessions, saves, platform and maybe games. Have to be server auth
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Buff)
	TMap<FName, FBuffOperationPack> PlayerUserModifiers;

	// Buffs to the player save. resist between sessions only. Must be sync for multi-players
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Buff)
	TMap<FName, FBuffOperationPack> PlayerSessionModifiers;

	// Buffs to a specific player character ID. resist between sessions only. Must be sync for multi-players
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Buff)
	TMap<FName, FBuffOperationPack> PlayerCharacterModifiers;

	// Buffs to a specific NPC character ID. resist between sessions only. Must be sync for multi-players
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Buff)
	TMap<FName, FBuffOperationPack> NpcCharacterModifiers;
};

UCLASS(BlueprintType, Blueprintable, Transient)
class UBuffModifierHandler : public UObject, public IIPoolingObject
{
	GENERATED_BODY()

private:
	FString _uid;
	EBuffCategoryType _type = EBuffCategoryType::Actor;
	FName _category = "";
	int32 _index = INDEX_NONE;
	bool _bIsBound = false;

	UFUNCTION()
	void OnBuffChanged_Internal(FName Category, FBuffOperationModifier ModifierBuff, bool hadBeenRemoved);
	UFUNCTION()
	void OnBuffChangedActor_Internal(TWeakObjectPtr<AActor> Actor, FName Name, FBuffOperationModifier BuffOperationModifier, bool hadBeenRemoved);

public:
	UPROPERTY(BlueprintAssignable, Category = "Modifier")
	FOnHandleReleased OnHandleReleased;

	virtual bool OnPoolingObjectSpawned_Implementation(const FPoolingParams SpawnData) override;
	virtual void OnPoolingObjectDespawned_Implementation() override;
	UFUNCTION(BlueprintPure, Category = "Modifier")
	FBuffOperationModifier GetModifier() const;
	UFUNCTION(BlueprintPure, Category = "Modifier")
	bool IsValid() const;
	void BindManager(UOperationModifierSubModule* Mgr);
	void UnbindManager(UOperationModifierSubModule* Mgr);
};

#pragma endregion


/**
 * Handle update of buffs and debuffs
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UOperationModifierSubModule : public UPulseSubModuleBase
{
	GENERATED_BODY()

private:
	bool _autoApplyGameplayEffects = false;
	// Buffs to the global user. resist between sessions, saves, platform and maybe games. Have to be server auth
	TMap<FName, FBuffOperationPack> _playerUserModifiers;
	// Buffs to the player save. resist between sessions only. Must be sync for multi-players
	TMap<FName, FBuffOperationPack> _playerSessionModifiers;
	// Buffs to a specific player character ID. resist between sessions only. Must be sync for multi-players
	TMap<FName, FBuffOperationPack> _playerCharacterModifiers;
	// Buffs to a specific NPC character ID. resist between sessions only. Must be sync for multi-players
	TMap<FName, FBuffOperationPack> _npcCharacterModifiers;
	// Buffs to a specific actor in the scene. Linked to the life span of that actor. Must be sync for multi-players
	TMap<FName, FBuffOperationPack> _actorModifiers;
	UPROPERTY()
	TMap<TWeakObjectPtr<AActor>, FBuffOperationPack> _actorLinks;

	TQueue<FBuffAddOperationCommand> _playerUserAddCmd;
	TQueue<FBuffRemoveOperationCommand> _playerUserRemoveCmd;
	TQueue<FBuffAddOperationCommand> _playerSessionAddCmd;
	TQueue<FBuffRemoveOperationCommand> _playerSessionRemoveCmd;
	TQueue<FBuffAddOperationCommand> _playerCharacterAddCmd;
	TQueue<FBuffRemoveOperationCommand> _playerCharacterRemoveCmd;
	TQueue<FBuffAddOperationCommand> _npcCharacterAddCmd;
	TQueue<FBuffRemoveOperationCommand> _npcCharacterRemoveCmd;
	TQueue<FBuffAddOperationCommand> _actorAddCmd;
	TQueue<FBuffRemoveOperationCommand> _actorRemoveCmd;
	
	TQueue<FBuffRemoveOperationCommand> _removeCmdBroadcastQueue;

	UPROPERTY()
	TMap<FString, TWeakObjectPtr<UBuffModifierHandler>> _modifierHandlers;
	UPROPERTY()
	TMap<FString, FGameplayEffectSpecHandle> _modifierGameplayEffectHandles;

protected:
	void UpdatePack(TMap<FName, FBuffOperationPack>& TargetPack, const float DeltaTime, EBuffCategoryType Type);
	static void HandleEmpties(TMap<FName, FBuffOperationPack>& TargetPack);
	void ExecuteAddCommands(TMap<FName, FBuffOperationPack>& TargetPack, TQueue<FBuffAddOperationCommand>& SourceQueue);
	void ExecuteRemoveCommands(TMap<FName, FBuffOperationPack>& TargetPack, TQueue<FBuffRemoveOperationCommand>& SourceQueue);
	void BroadcastRemoveCommands();
	bool FindBuff(TMap<FName, FBuffOperationPack>& TargetPack, const FString& UID, FName& OutCategory, int32& OutIndex, TWeakObjectPtr<AActor>& OutActor);
	UFUNCTION()
	void OnActorDestroyed_Internal(AActor* DestroyedActor);
	UFUNCTION()
	void OnSaveBuffs_Internal(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave);
	UFUNCTION()
	void OnLoadGameBuffs_Internal(ELoadSaveResponse Response, int32 UserIndex, UPulseSaveData* LoadedSaveData);
	static UOperationModifierSubModule* Get(const UObject* WorldContext);
	TQueue<FBuffAddOperationCommand>* GetAddQueue(const FBuffOperationModifier& Modifier, const FName& Category, FBuffAddOperationCommand& OutCommand, AActor* Actor = nullptr);
	TQueue<FBuffRemoveOperationCommand>* GetRemoveQueue(const FString& ModifierUID, FBuffRemoveOperationCommand& OutCommand);
	bool AddGameplayEffect(UObject* AbilitySystemTarget, FBuffOperationModifier Buff);
	bool RemoveGameplayEffect(FGameplayEffectSpecHandle OutSpec);

public:
	UPROPERTY(BlueprintAssignable, Category = "Buff Modifiers")
	FOnUserBuffModifierChanged OnUserBuffModifierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Buff Modifiers")
	FOnGlobalBuffModifierChanged OnGlobalBuffModifierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Buff Modifiers")
	FOnPlayerBuffModifierChanged OnPlayerBuffModifierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Buff Modifiers")
	FOnNpcBuffModifierChanged OnNpcBuffModifierChanged;

	UPROPERTY(BlueprintAssignable, Category = "Buff Modifiers")
	FOnActorBuffModifierChanged OnActorBuffModifierChanged;


	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;


	// Apply every modifier that fit the context (actor->npc->player->global-user)
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static float ApplyAllModifiers(const UObject* WorldContext, const float Input, FGameplayTag Context);

	// Apply every applicable to the character modifier that fit the context (npc->player->global-user)
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static float ApplyCharacterModifiers(const UObject* WorldContext, const float Input, FPrimaryAssetId CharacterID, FGameplayTag Context);

	// Apply every applicable to the character modifier that fit the context (npc->player->global-user)
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static float ApplyPlayerCharacterModifiers(const UObject* WorldContext, const float Input, FPrimaryAssetId CharacterID, FGameplayTag Context);

	// Apply every applicable to the Actor modifier that fit the context (actor->npc->player->global-user)
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static float ApplyActorModifiers(const UObject* WorldContext, const float Input, AActor* Actor, FGameplayTag Context, int32 PlayerIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetAll(const UObject* WorldContext, TMap<FName, FBuffOperationPack>& OutGlobalCategoryModifiers, TMap<FName, FBuffOperationPack>& OutUserCategoryModifiers,
	                   TMap<FName, FBuffOperationPack>& OutPlayerCategoryModifiers, TMap<FName, FBuffOperationPack>& OutNpcCategoryModifiers,
	                   TMap<FName, FBuffOperationPack>& OutActorCategoryModifiers);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetAllModifiers(const UObject* WorldContext, FGameplayTag Context, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetUserModifiers(const UObject* WorldContext, FGameplayTag Context, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetGlobalModifiers(const UObject* WorldContext, FGameplayTag Context, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetPlayerCharacterModifiers(const UObject* WorldContext, FGameplayTag Context, FPrimaryAssetId ID, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetNPCCharacterModifiers(const UObject* WorldContext, FGameplayTag Context, FPrimaryAssetId ID, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers);

	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static void GetActorModifiers(const UObject* WorldContext, FGameplayTag Context, AActor* Actor, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers);


	// Add a Buff.
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext, AutoCreateRefTerm = "Category"))
	static bool AddModifier(const UObject* WorldContext, const FBuffOperationModifier& Modifier, const FName& Category);

	// Remove a Buff.
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static bool RemoveModifier(const UObject* WorldContext, const FString& ModifierUID);

	// Verify if a buff is valid
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static bool VerifyModifier(const UObject* WorldContext, const FString& ModifierUID);

	// get a reference to a modifier.
	static bool TryGetModifierRef(const UObject* WorldContext, const FString& ModifierUID, FBuffOperationModifier& OutModifier, EBuffCategoryType& OutType, FName& OutCategory,
	                              int32& OutIndex);

	// Try to get direct access to a modifier directly
	static bool ModifierDirectAccess(const UObject* WorldContext, EBuffCategoryType Type, const FName& Category, int32 Index, FBuffOperationModifier& OutModifier);

	// get the Handler to a Modifier
	UFUNCTION(BlueprintCallable, Category = "Buff Modifiers", meta=(WorldContext = WorldContext))
	static bool TryGetModifierHandler(UObject* WorldContext, const FString& ModifierUID, UBuffModifierHandler*& OutHandler);

	// get a pointer to a modifier.
	static void UnbindHandle(const UObject* WorldContext, UBuffModifierHandler* Handler);
	
	// Group modifiers per context from a pack
	UFUNCTION(BlueprintPure, Category = "Buff Modifiers")
	static void GroupByContext(const FBuffOperationPack& Pack, TMap<FGameplayTag, FBuffOperationPack>& OutPerContextPacks);
};
