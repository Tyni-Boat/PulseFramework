// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/SaveGame/IPulseSavableObject.h"
#include "Core/SaveGame/PulseSaveManager.h"


bool IIPulseSavableObject::BindSaveManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return false;
	TWeakObjectPtr<UObject> w_Ptr = thisObj;
	OnSave_Raw = SaveManager->OnGameAboutToSave_Raw.AddLambda([w_Ptr]()-> void
		{
			if (auto obj = w_Ptr.Get()) IIPulseSavableObject::Execute_OnPreSaveEvent(obj);
		});
	OnLoad_Raw = SaveManager->OnGameLoaded_Raw.AddLambda([w_Ptr]()-> void
		{
			if (auto obj = w_Ptr.Get()) IIPulseSavableObject::Execute_OnPostLoadEvent(obj);
		});
	UObject* saveObject = nullptr;
	if (IIPulseSavableObject::Execute_TryReadSavedValues(thisObj, IIPulseSavableObject::Execute_GetSaveClassType(thisObj), saveObject))
		IIPulseSavableObject::Execute_OnLoadedObject(thisObj, saveObject);
	return true;
}

bool IIPulseSavableObject::UnbindSaveManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return false;
	if (OnSave_Raw.IsValid())
		SaveManager->OnGameAboutToSave_Raw.Remove(OnSave_Raw);
	if (OnLoad_Raw.IsValid())
		SaveManager->OnGameLoaded_Raw.Remove(OnLoad_Raw);
	OnSave_Raw.Reset();
	OnLoad_Raw.Reset();
	return true;
}

bool IIPulseSavableObject::TryReadSavedValues_Implementation(TSubclassOf<UObject> Type, UObject*& OutResult)
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return false;
	return SaveManager->ReadSavedValue(Type, OutResult);
}

void IIPulseSavableObject::OnPreSaveEvent_Implementation()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return;
	auto saveObj = IIPulseSavableObject::Execute_OnSaveObject(thisObj);
	if (!saveObj || !saveObj->IsA(IIPulseSavableObject::Execute_GetSaveClassType(thisObj)))
		return;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return;
	SaveManager->WriteAsSavedValue(saveObj);
}

void IIPulseSavableObject::OnPostLoadEvent_Implementation()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return;
	UObject* OutObj = nullptr;
	if (IIPulseSavableObject::Execute_TryReadSavedValues(thisObj, IIPulseSavableObject::Execute_GetSaveClassType(thisObj), OutObj))
	{
		if (OutObj)
			IIPulseSavableObject::Execute_OnLoadedObject(thisObj, OutObj);
	}
}
