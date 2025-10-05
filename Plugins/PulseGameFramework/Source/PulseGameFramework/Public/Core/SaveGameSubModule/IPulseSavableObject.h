// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/SaveGameSubModule/PulseSaveManager.h"
#include "IPulseSavableObject.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIPulseSavableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface to automatically Save and Load by PulseSaveManager using keys.
 */
class PULSEGAMEFRAMEWORK_API IIPulseSavableObject
{
	GENERATED_BODY()

public:
protected:
	FDelegateHandle OnSave_Raw;
	FDelegateHandle OnLoad_Raw;
public:

	bool BindSaveManager();
	
	bool UnbindSaveManager();

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Save Manager|Savable", meta=(AdvancedDisplay = 2))
	bool TryReadSavedValues(TSubclassOf<UObject> Type, UObject*& OutResult, int32 UserIndex = 0);
	bool TryReadSavedValues_Implementation(TSubclassOf<UObject> Type, UObject*& OutResult, int32 UserIndex = 0);

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Save Manager|Savable", meta=(AdvancedDisplay = 2))
	void OnPreSaveEvent(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave);
	void OnPreSaveEvent_Implementation(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave);

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Save Manager|Savable", meta=(AdvancedDisplay = 2))
	void OnPostLoadEvent(ELoadSaveResponse Response, int32 UserIndex, UPulseSaveData* LoadedSaveData);
	void OnPostLoadEvent_Implementation(ELoadSaveResponse Response, int32 UserIndex, UPulseSaveData* LoadedSaveData);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Save Manager|Savable")
	UObject* OnSaveObject(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave);
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Save Manager|Savable")
	void OnLoadedObject(UObject* LoadedObject);

	// Must absolutely implement this one
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Save Manager|Savable")
	UClass* GetSaveClassType();	
};
