// Copyright � by Tyni Boat. All Rights Reserved.

#include "Core/PulseResourceManagement/Types/Character/CharacterTypes.h"
#include "Core/PulseCoreModule.h"
#include "Core/PulseSystemLibrary.h"
#include "Core/PulseResourceManagement/PulseAssetManager.h"
#include "Core/PulseResourceManagement/Types/Character/BasePulseCharacterAsset.h"
#include "Core/SaveGameSubModule/PulseSaveManager.h"


bool UCharactersSavedObject::TryReadCharacterSavedDatas(UObject* WorldContextObject, TSubclassOf<UBasePulseCharacterAsset> Type, const int32 CharacterId,
                                                        FCharacterSavedDatas& OutCharacterSaved, const int32 SaveUserIndex)
{
	FPrimaryAssetId AssetId;
	if (!UPulseAssetManager::TryGetPrimaryAssetID(Type, CharacterId, AssetId))
		return false;
	if (!WorldContextObject)
		return false;
	if (!WorldContextObject->GetWorld())
		return false;
	if (!WorldContextObject->GetWorld()->GetGameInstance())
		return false;
	auto coreModule = WorldContextObject->GetWorld()->GetGameInstance()->GetSubsystem<UPulseCoreModule>();
	if (!coreModule)
		return false;
	if (auto saveMgr = coreModule->GetSubModule<UPulseSaveManager>())
	{
		UObject* saveData;
		if (saveMgr->ReadSavedValue(UCharactersSavedObject::StaticClass(), saveData, SaveUserIndex))
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
	}
	return false;
}

bool UCharactersSavedObject::TryWriteCharacterSavedDatas(UObject* WorldContextObject, TSubclassOf<UBasePulseCharacterAsset> Type, const int32 CharacterId,
                                                         const FCharacterSavedDatas CharacterSave, const int32 SaveUserIndex)
{
	FPrimaryAssetId AssetId;
	if (!UPulseAssetManager::TryGetPrimaryAssetID(Type, CharacterId, AssetId))
		return false;
	if (!WorldContextObject)
		return false;
	if (!WorldContextObject->GetWorld())
		return false;
	if (!WorldContextObject->GetWorld()->GetGameInstance())
		return false;
	auto coreModule = WorldContextObject->GetWorld()->GetGameInstance()->GetSubsystem<UPulseCoreModule>();
	if (!coreModule)
		return false;
	if (auto saveMgr = coreModule->GetSubModule<UPulseSaveManager>())
	{
		UObject* saveData;
		UCharactersSavedObject* CharSave = nullptr;
		if (saveMgr->ReadSavedValue(UCharactersSavedObject::StaticClass(), saveData, SaveUserIndex))
			CharSave = Cast<UCharactersSavedObject>(saveData);
		if (!CharSave)
			CharSave = NewObject<UCharactersSavedObject>();
		if (!UPulseSystemLibrary::AddOrReplace(CharSave->PerCharacterEvolution, AssetId, CharacterSave))
			return false;
		if (!saveMgr->WriteAsSavedValue(CharSave, SaveUserIndex))
			return false;
		return true;
	}
	return false;
}
