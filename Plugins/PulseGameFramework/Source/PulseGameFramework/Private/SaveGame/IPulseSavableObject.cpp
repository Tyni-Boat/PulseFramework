// Copyright © by Tyni Boat. All Rights Reserved.

#include "SaveGame/IPulseSavableObject.h"
#include "SaveGame/PulseSaveManager.h"


void IIPulseSavableObject::OnPreSaveEvent_Implementation()
{
}

void IIPulseSavableObject::OnPostLoadEvent_Implementation()
{
}

UObject* IIPulseSavableObject::OnBuildSaveObject_Implementation(TSubclassOf<UObject> Class)
{
	return nullptr;
}

void IIPulseSavableObject::OnLoadedSaveObject_Implementation(const UObject* LoadedObject)
{
}

UClass* IIPulseSavableObject::GetSaveObjectClass_Implementation()
{
	return nullptr;
}
