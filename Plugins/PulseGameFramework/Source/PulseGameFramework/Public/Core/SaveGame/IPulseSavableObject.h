// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
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

protected:
	FDelegateHandle OnSave_Raw;
	FDelegateHandle OnLoad_Raw;
public:

	bool BindSaveManager();
	
	bool UnbindSaveManager();

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable", meta=(AdvancedDisplay = 2))
	bool TryReadSavedValues(TSubclassOf<UObject> Type, UObject*& OutResult);
	bool TryReadSavedValues_Implementation(TSubclassOf<UObject> Type, UObject*& OutResult);

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable", meta=(AdvancedDisplay = 2))
	void OnPreSaveEvent();
	void OnPreSaveEvent_Implementation();

	// No Need to implement this function
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseCore|SaveSystem|Savable", meta=(AdvancedDisplay = 2))
	void OnPostLoadEvent();
	void OnPostLoadEvent_Implementation();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PulseCore|SaveSystem|Savable")
	UObject* OnSaveObject();
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PulseCore|SaveSystem|Savable")
	void OnLoadedObject(UObject* LoadedObject);

	// Must absolutely implement this one
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PulseCore|SaveSystem|Savable")
	UClass* GetSaveClassType();	
};
