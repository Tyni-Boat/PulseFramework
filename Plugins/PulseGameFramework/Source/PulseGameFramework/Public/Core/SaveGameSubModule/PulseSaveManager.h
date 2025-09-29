// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseSubModuleBase.h"
#include "GameFramework/SaveGame.h"
#include "PulseSaveManager.generated.h"


#pragma region SubTypes

UENUM(BlueprintType)
enum ELoadSaveResponse
{
	Success,
	NullSaveData,
	UnknownSaveDataType,
	HashMismatch,
};

UENUM(BlueprintType)
enum ESaveSlotTypeInfos
{
	ManualSaveSlot,
	AutoSaveSlot,
	UnknownSaveSlot,
};

USTRUCT(blueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveMetaData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FDateTime LastSaveDate = FDateTime::MinValue();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	double GameProgression = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FText Title = FText();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FText Description = FText();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	TArray<float> Quantities = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	TSoftObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	TSoftObjectPtr<UTexture2D> BackGround;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	FString SaveHash;
};

USTRUCT(blueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveMetaPack
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	TArray<FSaveMetaData> SaveTambour;
};

USTRUCT(blueprintType)
struct PULSEGAMEFRAMEWORK_API FSaveDataPack
{
	GENERATED_BODY()
	
public:
	
	inline FSaveDataPack(){}
	
	inline FSaveDataPack(const TArray<uint8>& InSaveData): SaveByteArray(InSaveData){}
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = SaveMeta)
	TArray<uint8> SaveByteArray;
};

UCLASS(BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UPulseSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = Basic)
	TMap<FName, FSaveDataPack> ProgressionSaveDatas;
	
	UPROPERTY(SkipSerialization)
	TMap<TObjectPtr<UClass>, TObjectPtr<UObject>> ProgressionCachedDatas;

	UFUNCTION(BlueprintCallable, Category = Basic)
	bool CheckCacheIntegrity(TSubclassOf<UObject> Type) const;
	
	UFUNCTION(BlueprintCallable, Category = Basic)
	void InvalidateCache(TSubclassOf<UObject> Type);
	
	UFUNCTION(BlueprintCallable, Category = Basic)
	void InvalidateAllCache();
};

UCLASS(BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API USaveMetaWrapper : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SaveMeta)
	FSaveMetaData SaveMetaData = FSaveMetaData();
};

UCLASS(BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UPulseSaveManagerParams : public USaveGame
{
	GENERATED_BODY()

public:
	
	void SetMetaCount(int32 ManualSlotCount, int32 AutoSaveMetaCount, int32 ManualSaveMetaCount);

	UPROPERTY(VisibleAnywhere, Category = Basic)
	int32 LastManualSaveSlotIndex = 0;
	
	UPROPERTY(VisibleAnywhere, Category = Basic)
	int32 LastAutoSaveTambourIndex = -1;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	int32 LastManualSaveTambourIndex = -1;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	TArray<FSaveMetaPack> AutoSavesTambour = {};

	UPROPERTY(VisibleAnywhere, Category = Basic)
	TArray<FSaveMetaPack> ManualSavesTambour = {};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPostLoadGame, ELoadSaveResponse, Response, int32, UserIndex, UPulseSaveData*, LoadedSaveData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPostSaveGame, const FString&, SlotName, const int32, UserIndex, bool, bSuccess, bool, bAutoSave);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPreSaveGame, const FString&, SlotName, const int32, UserIndex, USaveMetaWrapper*, SaveMetaDataWrapper, bool, bAutoSave);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreLoadGame, const FString&, SlotName, const int32, UserIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveDeleted, const int32, UserIndex, TArray<FString>, DeletedSlots);

#pragma endregion


/**
 * Manage Save and loads of the game
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseSaveManager : public UPulseSubModuleBase
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<int32, TObjectPtr<UPulseSaveManagerParams>> SaveParams;
	FName _baseManualSaveSlotName = "ManualSave";
	FName _baseAutoSaveSlotName = "AutoSave";
	int32 _manualSaveSlotCount = 1;
	int32 _perManualSlotTambour = 1;
	int32 _perAutoSlotTambour = 1;
	bool _useAutoSave = false;
	bool _useSaveCacheValidationOnRead = false;
	float _autoSaveInterval = 300;
	float _autoSaveCountDown = 300;
	int32 _saveUserCount = 1;

protected:
	// The saved objects that contains the progression data
	UPROPERTY()
	TMap<int32, TObjectPtr<UPulseSaveData>> PerUserSavedProgression;

	UFUNCTION()
	void OnSavedGame_Internal(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	UFUNCTION()
	void OnLoadedGame_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);

	UFUNCTION()
	void OnLoadedParams_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);

	FString GetNextTambourSlotName(const int32 UserIndex, const int32 SlotIndex, int32& OutUsedTambourIndex, bool bIsAutoSave = false);

	void LoadSaveParamForUser(int32 UserIndex);

	bool SaveParamsForUser(const int32 UserIndex, const int32 SlotIndex, const int32 TambourIndex, const FSaveMetaData& SaveMetaData, bool bIsAutoSave);

	bool SaveGameInternal(const int32 UserIndex, const int32 SlotIndex, bool bIsAutoSave);

public:
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Manager")
	bool bIsLoadingGame;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Manager")
	int32 SavingGame = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Manager")
	int32 SavingParams = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Manager")
	int32 InitializationCountDown = 0;

	UPROPERTY(BlueprintAssignable, Category = "Save Manager")
	FOnPostLoadGame OnGameLoaded;

	UPROPERTY(BlueprintAssignable, Category = "Save Manager")
	FOnPostSaveGame OnGameSaved;

	UPROPERTY(BlueprintAssignable, Category = "Save Manager")
	FOnPreSaveGame OnGameAboutToSave;

	UPROPERTY(BlueprintAssignable, Category = "Save Manager")
	FOnPreLoadGame OnGameAboutToLoad;

	UPROPERTY(BlueprintAssignable, Category = "Save Manager")
	FOnSaveDeleted OnSaveDeleted;


	// Auto-Save the game data.
	UFUNCTION(BlueprintCallable, Category = "Save Manager", meta = (AdvancedDisplay = 0))
	bool AutoSaveGame(int32 UserIndex = 0);


	// Save the game data to a specified slot
	UFUNCTION(BlueprintCallable, Category = "Save Manager", meta = (AdvancedDisplay = 0))
	bool ManualSaveGame(int32 UserIndex = 0, int32 SaveSlot = 0);


	// Load the game data from a specified slot
	UFUNCTION(BlueprintCallable, Category = "Save Manager", meta = (AdvancedDisplay = 0))
	bool LoadGame(const FString SlotName = "", int32 UserIndex = 0);


	// Delete Save At Slot
	UFUNCTION(BlueprintCallable, Category = "Save Manager", meta = (AdvancedDisplay = 0))
	bool DeleteSave(const int32 UserIndex, const int32 SlotIndex = 0, bool bIsAutoSaveSlot = false);

	// Delete All Saves
	UFUNCTION(BlueprintCallable, Category = "Save Manager", meta = (AdvancedDisplay = 0))
	bool DeleteAllSaves();


	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool ReadSavedValue(TSubclassOf<UObject> Type, UObject*& OutResult, int32 UserIndex = 0);


	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool WriteAsSavedValue(UObject* Value, int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool TryGetSaveMetaDatas(const int32 SlotIndex, TArray<FSaveMetaData>& OutMetaDatas, int32 UserIndex = 0, bool bAutoSaveSlot = false, bool bOnlyValidSaveDateTime = false) const;

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool TryGetSaveMetaData(const FString& SlotName, const int32 UserIndex, FSaveMetaData& OutMetaData) const;
	
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool TryGetSlotInfos(const FString& SlotName, TEnumAsByte<ESaveSlotTypeInfos>& OutSlotType, int32& OutSlotIndex, int32& OutTambourIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool GetSlotName(const int32 SlotIndex, FString& OutSlotName, bool bAutoSaveSlot = false, const int32 TambourIndex = 0) const;

	// The Number of time we can make temp saves (tambour) on the same slot.
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	int32 GetPerTambourCount(bool bAutoSaveSlot = false) const;

	// The Number of available slot to use
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	int32 GetSaveSlotCount(bool bAutoSaveSlot = false) const;

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool IsValidSaveSlot(const int32 UserIndex, const int32 SlotIndex, bool bAutoSaveSlot = false) const;

	// X- the slot index, Y- The tambour index
	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	FVector2D GetLastUsedSaveUID(const int32 UserIndex, bool bAutoSaveSlot = false) const;

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool IsValidSaveUser(int32 UserIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	int32 GetSaveUserCount() const;

	UFUNCTION(BlueprintCallable, Category = "Save Manager")
	bool VerifyLoadSaveHash(const FString& SlotName, const int32 UserIndex, UPulseSaveData* Data) const;
};
