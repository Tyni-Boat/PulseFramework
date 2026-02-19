// Copyright � by Tyni Boat. All Rights Reserved.

#include "Core/AssetManager/Types/Character/CharacterTypes.h"

#include "Core/PulseSystemLibrary.h"
#include "Core/AssetManager/PulseAssetManager.h"
#include "Core/AssetManager/Types/Character/BasePulseCharacterAsset.h"
#include "Core/SaveGame/PulseSaveManager.h"


bool UCharactersSavedObject::TryReadCharacterSavedData(UObject* WorldContextObject, TSubclassOf<UBasePulseCharacterAsset> Type, const int32 CharacterId,
                                                       FCharacterSavedDatas& OutCharacterSaved)
{
	if (!WorldContextObject)
		return false;
	auto saveMgr = UPulseSaveManager::Get(WorldContextObject);
	if (!saveMgr)
		return false;
	FPrimaryAssetId AssetId;
	if (!UPulseBPAssetManager::TryGetPrimaryAssetID(Type, CharacterId, AssetId))
		return false;
	UObject* saveData;
	if (saveMgr->ReadSavedValue(UCharactersSavedObject::StaticClass(), saveData))
	{
		if (auto asCharEvoSave = Cast<UCharactersSavedObject>(saveData))
		{
			if (asCharEvoSave->PerCharacterEvolution.Contains(AssetId))
			{
				OutCharacterSaved = asCharEvoSave->PerCharacterEvolution[AssetId];
				return true;
			}
		}
	}
	return false;
}


bool UCharactersSavedObject::TryWriteCharacterSavedData(UObject* WorldContextObject, TSubclassOf<UBasePulseCharacterAsset> Type, const int32 CharacterId,
                                                        const FCharacterSavedDatas CharacterSave)
{
	if (!WorldContextObject)
		return false;
	auto saveMgr = UPulseSaveManager::Get(WorldContextObject);
	if (!saveMgr)
		return false;
	FPrimaryAssetId AssetId;
	if (!UPulseBPAssetManager::TryGetPrimaryAssetID(Type, CharacterId, AssetId))
		return false;
	UObject* saveData;
	UCharactersSavedObject* CharSave = nullptr;
	if (saveMgr->ReadSavedValue(UCharactersSavedObject::StaticClass(), saveData))
		CharSave = Cast<UCharactersSavedObject>(saveData);
	if (!CharSave)
		CharSave = NewObject<UCharactersSavedObject>();
	if (!UPulseSystemLibrary::MapAddOrUpdateValue(CharSave->PerCharacterEvolution, AssetId, CharacterSave))
		return false;
	if (!saveMgr->WriteAsSavedValue(CharSave))
		return false;
	return true;
}
