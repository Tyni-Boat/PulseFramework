// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPulseSavableObject.generated.h"


// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UIPulseSavableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implement this interface to listen to save and load game events, and auto write values on save.
 */
class PULSEGAMEFRAMEWORK_API IIPulseSavableObject
{
	GENERATED_BODY()

public:

	// Called just before the game is saved
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable")
	void OnPreSaveEvent();
	virtual void OnPreSaveEvent_Implementation();

	// Called just after the game is loaded.
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable")
	void OnPostLoadEvent();
	virtual void OnPostLoadEvent_Implementation();

	// Implement to build the save object upon saving. the returned Object type MUST match the GetSaveObjectClass()
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable")
	UObject* OnBuildSaveObject(TSubclassOf<UObject> Class);
	virtual UObject* OnBuildSaveObject_Implementation(TSubclassOf<UObject> Class);

	// Implement to read the corresponding loaded object of type GetSaveObject()
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable", meta=(ForceAsFunction))
	void OnLoadedSaveObject(const UObject* LoadedObject);
	virtual void OnLoadedSaveObject_Implementation(const UObject* LoadedObject);

	// Get Save object class
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable")
	UClass* GetSaveObjectClass();
	virtual UClass* GetSaveObjectClass_Implementation();
};
