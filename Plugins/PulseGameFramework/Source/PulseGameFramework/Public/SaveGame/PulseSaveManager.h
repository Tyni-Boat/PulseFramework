// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameSaveProvider.h"
#include "Core/PulseCoreTypes.h"
#include "PulseSaveManager.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveLoadedMetaEvent, FSaveMetaDataBundle, MetaBundle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveDeletedEvent, TSubclassOf<UGameSaveProvider>, ProviderClass, FSaveMetaData, Meta);


/**
 * Manage Save and loads of the game
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseSaveManager : public UGameInstanceSubsystem, public IIPulseCore
{
	GENERATED_BODY()

private:
	bool _useSaveCacheValidationOnRead = false;
	bool _useLoadHashVerification = false;
	TArray<TSubclassOf<UGameSaveProvider>> _savingClassSet;

protected:
	// The saved objects that contains the progression data
	UPROPERTY()
	TObjectPtr<UPulseSaveData> SavedProgression;

	// The List of provider per class
	UPROPERTY()
	TMap<TSubclassOf<UGameSaveProvider>, TObjectPtr<UGameSaveProvider>> ProvidersMap;

	UPROPERTY()
	TSubclassOf<UGameSaveProvider> LocalProviderClass;

	UPROPERTY()
	TObjectPtr<USaveMetaProcessor> MetaProcessor;

	UFUNCTION()
	void OnProviderEvent_Saved(TSubclassOf<UGameSaveProvider> Class, FSaveMetaData SaveMetaData, bool Success);
	UFUNCTION()
	void OnProviderEvent_Loaded(TSubclassOf<UGameSaveProvider> Class, UPulseSaveData* PulseSaveData, FSaveMetaData SaveMetaData);
	UFUNCTION()
	void OnProviderEvent_LoadedMetas(TSubclassOf<UGameSaveProvider> Class, FSaveMetaDataPack SaveMetaDataPack);
	UFUNCTION()
	void OnProviderEvent_Deleted(TSubclassOf<UGameSaveProvider> Class, FSaveMetaData SaveMetaData, bool Success);


	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	bool VerifyLoadSaveHash(const FSaveMetaData& Meta, UPulseSaveData* LoadedData) const;

	void Save_Internal(const FUserProfile& User, UPulseSaveData* data, FDateTime Time, const int32& SlotIndex, const TArray<TSubclassOf<UGameSaveProvider>>& ProviderClasses, bool bAutoSave);
	void LoadGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta, const TSubclassOf<UGameSaveProvider>& ProviderClass);
	void LoadMetas_Internal(const FUserProfile& User, const TSubclassOf<UGameSaveProvider>& ProviderClass);
	void DeleteGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta, const TSubclassOf<UGameSaveProvider>& ProviderClass);

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	FPulseTriggerEventRaw OnGameLoaded_Raw;
	FPulseTriggerEventRaw OnGameAboutToSave_Raw;
	FPulseTriggerEventRaw OnGameSaved_Raw;


	UPROPERTY(BlueprintAssignable, Category = "PulseCore|SaveSystem")
	FPulseTriggerEvent OnGameLoaded;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|SaveSystem")
	FPulseTriggerEvent OnGameAboutToSave;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|SaveSystem")
	FPulseTriggerEvent OnGameSaved;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|SaveSystem")
	FOnSaveLoadedMetaEvent OnLoadedMetas;

	UPROPERTY(BlueprintAssignable, Category = "PulseCore|SaveSystem")
	FOnSaveDeletedEvent OnDeleted;


	UFUNCTION(BlueprintPure, Category = "PulseCore|SaveSystem")
	EPulseSaveStatus GetSaveStatus() const;

	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem", meta = (DynamicOutputParam = "OutInstance", DeterminesOutputType = "Type"))
	bool GetSaveProvider(TSubclassOf<UGameSaveProvider> Type, UGameSaveProvider*& OutInstance);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	bool GetSaveProviderClasses(TArray<TSubclassOf<UGameSaveProvider>>& OutType);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	bool GetMetaProcessor(USaveMetaProcessor*& OutInstance);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem", meta=(DeterminesOutputType = "Type", DynamicOutputParam = "OutResult"))
	bool ReadSavedValue(TSubclassOf<UObject> Type, UObject*& OutResult);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	bool WriteAsSavedValue(UObject* Value);



	// Save the game data to all save providers except those in the exception list.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem", meta = (AdvancedDisplay = 0, AutoCreateRefTerm = "ExceptionList"))
	void SaveGame(const int32 SaveIndex, const TArray<TSubclassOf<UGameSaveProvider>>& ExceptionList, bool bAutoSave = false);

	// Load the game data from a provider
	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	void LoadGame(TSubclassOf<UGameSaveProvider> Provider, const FSaveMetaData& Meta);

	// Load save metas from a provider
	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	void LoadMetas(const FUserProfile& User, TSubclassOf<UGameSaveProvider> Provider);

	// Delete Save from a provider.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|SaveSystem")
	void DeleteSave(TSubclassOf<UGameSaveProvider> Provider, const FSaveMetaData& Meta);

	// Get The reference to the save manager
	static UPulseSaveManager* Get(const UObject* WorldContextObject);
};
