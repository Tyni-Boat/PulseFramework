// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/SaveGame/LocalGameSaveProvider.h"

#include "Core/PulseSystemLibrary.h"
#include "Core/SaveGame/PulseSaveManager.h"
#include "Kismet/GameplayStatics.h"


void ULocalGameSaveProvider::OnSavedGame_Internal(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	if (bSuccess)
		_lastSaveSlotName = SlotName;
	if (_saved_GameX_MetaY.Y <= 0)
	{
		_saved_GameX_MetaY.X = 1;
		return;
	}
	_saved_GameX_MetaY = FVector2D(0, 0);
	EndSave(bSuccess);
}

void ULocalGameSaveProvider::OnLoadedGame_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
	EndLoadGame(Cast<UPulseSaveData>(LoadedGameData));
}

void ULocalGameSaveProvider::OnSavedMeta_Internal(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	if (_saved_GameX_MetaY.X <= 0)
	{
		_saved_GameX_MetaY.Y = 1;
		return;
	}
	_saved_GameX_MetaY = FVector2D(0, 0);
	EndSave(bSuccess);
}

void ULocalGameSaveProvider::OnLoadedMeta_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedMetaData)
{
	FString slotQueueItem = "";
	if (!_metaLoadSlotsQueue.Dequeue(slotQueueItem))
		return;
	if (const auto& metaObj = Cast<ULocalSaveMeta>(LoadedMetaData))
	{
		_loadedMetaDataList.Add(metaObj->SavedMetaData);
	}
	if (_metaLoadSlotsQueue.IsEmpty())
	{
		EndLoadMeta(_loadedMetaDataList);
		_loadedMetaDataList.Empty();
	}
}


ULocalGameSaveProvider::ULocalGameSaveProvider()
{
	bAllowAutoSaves = true;
}

void ULocalGameSaveProvider::Initialization_Implementation(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings)
{
	Super::Initialization_Implementation(SaveManager, ProjectSettings);
	if (ProjectSettings)
	{
		_saveSlotCount = ProjectSettings->LocalSaveSlotCount;
		_bufferIndexesSize = ProjectSettings->LocalPerSlotBufferCount;
	}
}

void ULocalGameSaveProvider::Deinitialization_Implementation(UPulseSaveManager* SaveManager)
{
	Super::Deinitialization_Implementation(SaveManager);
}

void ULocalGameSaveProvider::BeginSave_Implementation(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData)
{
	if (SaveMeta.SlotIndex >= _saveSlotCount)
	{
		EndSave(false);
		return;
	}
	Super::BeginSave_Implementation(User, SaveMeta, SaveData);
	FAsyncSaveGameToSlotDelegate SavedDelegate;
	SavedDelegate.BindUObject(this, &ULocalGameSaveProvider::OnSavedGame_Internal);
	UGameplayStatics::AsyncSaveGameToSlot(SaveData, SaveMeta.GetSlotName(), 0, SavedDelegate);

	auto metaSave = Cast<ULocalSaveMeta>(UGameplayStatics::CreateSaveGameObject(ULocalSaveMeta::StaticClass()));
	metaSave->SavedMetaData = SaveMeta;
	FAsyncSaveGameToSlotDelegate MetaDelegate;
	MetaDelegate.BindUObject(this, &ULocalGameSaveProvider::OnSavedMeta_Internal);
	UGameplayStatics::AsyncSaveGameToSlot(metaSave, SaveMeta.GetSlotName(true), 0, MetaDelegate);
}

void ULocalGameSaveProvider::BeginLoadMeta_Implementation(const FUserProfile& Userprofile)
{
	Super::BeginLoadMeta_Implementation(Userprofile);
	TArray<FString> metaPaths;
	FSaveMetaData manualMeta;
	manualMeta.UserLocalID = Userprofile.LocalID;
	FSaveMetaData autoMeta = manualMeta;
	autoMeta.bIsAnAutoSave = true;
	for (int i = 0; i < _saveSlotCount; i++)
	{
		manualMeta.SlotIndex = i;
		autoMeta.SlotIndex = i;
		for (int j = 0; j < _bufferIndexesSize; j++)
		{
			manualMeta.SlotBufferIndex = j;
			autoMeta.SlotBufferIndex = j;
			if (UGameplayStatics::DoesSaveGameExist(manualMeta.GetSlotName(true), 0))
			{
				metaPaths.Add(manualMeta.GetSlotName(true));
			}
			if (UGameplayStatics::DoesSaveGameExist(autoMeta.GetSlotName(true), 0))
			{
				metaPaths.Add(autoMeta.GetSlotName(true));
			}
		}
	}
	if (metaPaths.IsEmpty())
	{
		EndLoadMeta({});
		return;
	}
	_metaLoadSlotsQueue.Empty();
	_loadedMetaDataList.Empty();
	// Set up the delegate.
	FAsyncLoadGameFromSlotDelegate MetaLoadedDelegate;
	// USomeUObjectClass::LoadGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData
	MetaLoadedDelegate.BindUObject(this, &ULocalGameSaveProvider::OnLoadedMeta_Internal);
	// Start the load process
	for (int i = 0; i < metaPaths.Num(); i++)
	{
		_metaLoadSlotsQueue.Enqueue(metaPaths[i]);
		UGameplayStatics::AsyncLoadGameFromSlot(metaPaths[i], 0, MetaLoadedDelegate);
	}
}

void ULocalGameSaveProvider::BeginLoadGame_Implementation(const FUserProfile& Userprofile, const FSaveMetaData& Meta)
{
	Super::BeginLoadGame_Implementation(Userprofile, Meta);
	// Set up the delegate.
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	// USomeUObjectClass::LoadGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData
	LoadedDelegate.BindUObject(this, &ULocalGameSaveProvider::OnLoadedGame_Internal);
	// Start the load process
	UGameplayStatics::AsyncLoadGameFromSlot(Meta.GetSlotName(), 0, LoadedDelegate);
}

void ULocalGameSaveProvider::BeginDeleteGame_Implementation(const FUserProfile& User, const FSaveMetaData& Meta)
{
	Super::BeginDeleteGame_Implementation(User, Meta);
	// Start the Deletion process
	bool deleted = false;
	if (UGameplayStatics::DeleteGameInSlot(Meta.GetSlotName(), 0))
		deleted = true;
	if (UGameplayStatics::DeleteGameInSlot(Meta.GetSlotName(true), 0))
		deleted = true;
	EndDeleteGame(deleted);
}

bool ULocalGameSaveProvider::IsLastSavedMeta_Implementation(const FSaveMetaData& Meta) const
{
	return Meta.GetSlotName() == _lastSaveSlotName;
}

int32 ULocalGameSaveProvider::GetBufferSlot_Implementation(const int32 SlotIndex, bool bIsAutoSaveSlot)
{
	int32 index = 0;
	if (bIsAutoSaveSlot)
	{
		if (!_autoSaveSlotBufferIndexes.Contains(SlotIndex))
			_autoSaveSlotBufferIndexes.Add(SlotIndex, -1);
		index = FMath::Modulo(_autoSaveSlotBufferIndexes[SlotIndex] + 1, _bufferIndexesSize);
		_autoSaveSlotBufferIndexes[SlotIndex] = index;
	}
	else
	{
		if (!_manualSaveSlotBufferIndexes.Contains(SlotIndex))
			_manualSaveSlotBufferIndexes.Add(SlotIndex, -1);
		index = FMath::Modulo(_manualSaveSlotBufferIndexes[SlotIndex] + 1, _bufferIndexesSize);
		_manualSaveSlotBufferIndexes[SlotIndex] = index;
	}
	return index;
}
