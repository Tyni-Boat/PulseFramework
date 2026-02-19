// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseCoreTypes.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Object.h"
#include "GameSaveProvider.generated.h"

#define AUTO_SLOT "Auto"
#define MANUAL_SLOT "Manual"
#define META_SLOT "_Meta"

class UGameSaveProvider;

// Status of a save provider. every non-Idle status is blocking within the same provider.
UENUM(BlueprintType)
enum class EPulseSaveStatus : uint8
{
	Idle = 0,
	Saving = 1,
	Deleting = 2,
	Loading = 3, // This State is blocking at the manager level
	LoadingMetas = 4, // This State is blocking at the manager level
};

USTRUCT(blueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveMetaData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FDateTime LastSaveDate = FDateTime::MinValue();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FString UserLocalID = "";

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	int32 SlotIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	int32 SlotBufferIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	bool bIsAnAutoSave = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	FString SaveHash = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FString DetailsJson = "";

	inline FString GetSlotName(bool bMetaSlot = false) const
	{
		return FString::Printf(TEXT("%s_%s_%d_%d%s"), *UserLocalID, *FString(bIsAnAutoSave ? AUTO_SLOT : MANUAL_SLOT), SlotIndex, SlotBufferIndex,
		                       *FString(bMetaSlot ? META_SLOT : ""));
	}

	friend bool operator==(const FSaveMetaData& Lhs, const FSaveMetaData& RHS)
	{
		return Lhs.LastSaveDate == RHS.LastSaveDate
			&& Lhs.UserLocalID == RHS.UserLocalID
			&& Lhs.SlotIndex == RHS.SlotIndex
			&& Lhs.SlotBufferIndex == RHS.SlotBufferIndex
			&& Lhs.bIsAnAutoSave == RHS.bIsAnAutoSave
			&& Lhs.SaveHash == RHS.SaveHash;
	}

	friend bool operator!=(const FSaveMetaData& Lhs, const FSaveMetaData& RHS)
	{
		return !(Lhs == RHS);
	}
};

USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveMetaDataPack
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	TArray<FSaveMetaData> MetaDataList;
};

USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveMetaDataBundle
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	TMap<TSubclassOf<UGameSaveProvider> , FSaveMetaDataPack> MetaDataBundle;
};

USTRUCT(blueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveDataPack
{
	GENERATED_BODY()

public:
	inline FSaveDataPack()
	{
	}

	inline FSaveDataPack(const TArray<uint8>& InSaveData) : SaveByteArray(InSaveData)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	TArray<uint8> SaveByteArray;
};

UCLASS(BlueprintType, NotBlueprintable)
class PULSEGAMEFRAMEWORK_API UPulseSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "PulseCore|Savegame")
	TMap<FName, FSaveDataPack> ProgressionSaveData;

	UPROPERTY(SkipSerialization)
	TMap<TObjectPtr<UClass>, TObjectPtr<UObject>> ProgressionCachedData;

	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	bool CheckCacheIntegrity(TSubclassOf<UObject> Type) const;

	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	void InvalidateCache(TSubclassOf<UObject> Type);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	void InvalidateAllCache();
};

USTRUCT()
struct FPendingSave
{
	GENERATED_BODY()

public:
	FSaveMetaData Meta;
	FSaveDataPack SaveDataPack;
	FUserProfile UserProfile;
};


DECLARE_MULTICAST_DELEGATE_ThreeParams(FSaveProviderEndSaveEvent, TSubclassOf<UGameSaveProvider> ProviderClass, FSaveMetaData Meta, bool Success);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FSaveProviderEndLoadEvent, TSubclassOf<UGameSaveProvider> ProviderClass, UPulseSaveData* Data, FSaveMetaData Meta);
DECLARE_MULTICAST_DELEGATE_TwoParams(FSaveProviderEndLoadMetaEvent, TSubclassOf<UGameSaveProvider> ProviderClass, FSaveMetaDataPack MetaPack);

/**
 * The Base class for Any Game Meta Processor, That handle the details of a save's meta file
 */
UCLASS(BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API USaveMetaProcessor : public UObject
{
	GENERATED_BODY()

public:
	// Build meta data detail json from Save data
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	FString BuildMetaDetails(const FSaveMetaData Meta, const UPulseSaveData* SaveData) const;
	FString BuildMetaDetails_Implementation(const FSaveMetaData Meta, const UPulseSaveData* SaveData) const;

	// Sort meta list from "best" to "worse". By default it's sorted by date.
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void SortMetaList(TArray<FSaveMetaData>& MetaList) const;
	virtual void SortMetaList_Implementation(TArray<FSaveMetaData>& MetaList) const;
};


/**
 * The Base class for Any Game save provider
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UGameSaveProvider : public UObject
{
	GENERATED_BODY()

protected:
	TQueue<FSaveMetaData, EQueueMode::Mpsc> _saveMetaQueue;
	TQueue<FSaveMetaData, EQueueMode::Mpsc> _loadMetaQueue;
	TQueue<bool, EQueueMode::Mpsc> _loadMetaDataQueue;
	TQueue<FSaveMetaData, EQueueMode::Mpsc> _deleteMetaQueue;
	TQueue<FPendingSave, EQueueMode::Mpsc> _pendingSaves;
	TMap<int32, int32> _autoSaveSlotBufferIndexes;
	TMap<int32, int32> _manualSaveSlotBufferIndexes;
	FString _lastSaveSlotName = "";

	void HandlePendingSaves();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PulseCore|Savegame")
	bool bAllowAutoSaves = false;

	FSaveProviderEndSaveEvent OnSavedGame;
	FSaveProviderEndLoadMetaEvent OnLoadedMetas;
	FSaveProviderEndLoadEvent OnLoadedGame;
	FSaveProviderEndSaveEvent OnDeletedGame;


	void Initialization_Internal(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings);
	// Initialization of the provider
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void Initialization(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings);
	virtual void Initialization_Implementation(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings);

	void Deinitialization_Internal(UPulseSaveManager* SaveManager);
	// Deinitialization of the provider
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void Deinitialization(UPulseSaveManager* SaveManager);
	virtual void Deinitialization_Implementation(UPulseSaveManager* SaveManager);


	//Game Save
	void BeginSave_Internal(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData);
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void BeginSave(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData);
	virtual void BeginSave_Implementation(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData);
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	void EndSave(bool bSuccess);

	// Meta Load
	void BeginLoadMeta_Internal(const FUserProfile& User);
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void BeginLoadMeta(const FUserProfile& User);
	virtual void BeginLoadMeta_Implementation(const FUserProfile& User);
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	void EndLoadMeta(const TArray<FSaveMetaData>& MetaDataList);

	// Game Load
	void BeginLoadGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta);
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void BeginLoadGame(const FUserProfile& User, const FSaveMetaData& Meta);
	virtual void BeginLoadGame_Implementation(const FUserProfile& User, const FSaveMetaData& Meta);
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	void EndLoadGame(UPulseSaveData* LoadedData);

	// Game and Meta Delete
	void BeginDeleteGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta);
	UFUNCTION(BlueprintNativeEvent, Category = "PulseCore|Savegame")
	void BeginDeleteGame(const FUserProfile& User, const FSaveMetaData& Meta);
	virtual void BeginDeleteGame_Implementation(const FUserProfile& User, const FSaveMetaData& Meta);
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Savegame")
	void EndDeleteGame(bool bSuccess);


	// Get The Meta data of the current save game. valid only between BeginSave and EndSave
	UFUNCTION(BlueprintPure, Category = "PulseCore|Savegame")
	bool GetSavingMeta(FSaveMetaData& OutMeta) const;

	// Get The slot name
	UFUNCTION(BlueprintPure, Category = "PulseCore|Savegame")
	static FString GetSlotName(const FSaveMetaData& Meta, bool bIsMetaSlot = false);

	// Get The Actual Status of the provider
	UFUNCTION(BlueprintPure, Category = "PulseCore|Savegame")
	EPulseSaveStatus GetStatus() const;

	// Get the current save buffer index for the slot
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "PulseCore|Savegame")
	int32 GetBufferSlot(const int32 SlotIndex, bool bIsAutoSaveSlot = false);
	virtual int32 GetBufferSlot_Implementation(const int32 SlotIndex, bool bIsAutoSaveSlot = false);

	// Check if a meta is from an auto save or not
	UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "PulseCore|Savegame")
	bool IsLastSavedMeta(const FSaveMetaData& Meta) const;
	virtual bool IsLastSavedMeta_Implementation(const FSaveMetaData& Meta) const;
};
