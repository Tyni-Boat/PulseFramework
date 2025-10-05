// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once


#include "Core/SaveGameSubModule/IPulseSavableObject.h"

#include "Core/SaveGameSubModule/PulseSaveManager.h"


bool IIPulseSavableObject::BindSaveManager()
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return false;
	TWeakObjectPtr<UObject> w_Ptr = thisObj;
	OnSave_Raw = SaveManager->OnGameAboutToSave_Raw.AddLambda([w_Ptr](const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave)-> void
		{
			if (auto obj = w_Ptr.Get()) IIPulseSavableObject::Execute_OnPreSaveEvent(obj, SlotName, UserIndex, SaveMetaDataWrapper, bAutoSave);
		});
	OnLoad_Raw = SaveManager->OnGameLoaded_Raw.AddLambda([w_Ptr](ELoadSaveResponse Response, int32 UserIndex, UPulseSaveData* LoadedSaveData)-> void
		{
			if (auto obj = w_Ptr.Get()) IIPulseSavableObject::Execute_OnPostLoadEvent(obj, Response, UserIndex, LoadedSaveData);
		});
	UObject* saveObject = nullptr;
	if (IIPulseSavableObject::Execute_TryReadSavedValues(thisObj, IIPulseSavableObject::Execute_GetSaveClassType(thisObj), saveObject, 0))
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

bool IIPulseSavableObject::TryReadSavedValues_Implementation(TSubclassOf<UObject> Type, UObject*& OutResult,
	int32 UserIndex)
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return false;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return false;
	return SaveManager->ReadSavedValue(Type, OutResult, UserIndex);
}

void IIPulseSavableObject::OnPreSaveEvent_Implementation(const FString& SlotName, const int32 UserIndex,
	USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave)
{
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return;
	auto saveObj = IIPulseSavableObject::Execute_OnSaveObject(thisObj, SlotName, UserIndex, SaveMetaDataWrapper, bAutoSave);
	if (!saveObj || !saveObj->IsA(IIPulseSavableObject::Execute_GetSaveClassType(thisObj)))
		return;
	auto SaveManager = UPulseSaveManager::Get(thisObj);
	if (!SaveManager)
		return;
	SaveManager->WriteAsSavedValue(saveObj, UserIndex);
}

void IIPulseSavableObject::OnPostLoadEvent_Implementation(ELoadSaveResponse Response, int32 UserIndex,
	UPulseSaveData* LoadedSaveData)
{
	if (Response != ELoadSaveResponse::Success)
		return;
	auto thisObj = Cast<UObject>(this);
	if (!thisObj)
		return;
	UObject* OutObj = nullptr;
	if (IIPulseSavableObject::Execute_TryReadSavedValues(thisObj, IIPulseSavableObject::Execute_GetSaveClassType(thisObj), OutObj, UserIndex))
	{
		if (OutObj)
			IIPulseSavableObject::Execute_OnLoadedObject(thisObj, OutObj);
	}
}
