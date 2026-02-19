// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/SaveGame/GameSaveProvider.h"

#include "Core/PulseSystemLibrary.h"
#include "Core/SaveGame/PulseSaveManager.h"


bool UPulseSaveData::CheckCacheIntegrity(TSubclassOf<UObject> Type) const
{
	if (!ProgressionCachedData.Contains(Type))
		return true;
	TArray<uint8> _bytes;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(ProgressionCachedData[Type], _bytes))
		return false;
	if (!ProgressionSaveData.Contains(Type->GetFName()))
		return false;
	return UPulseSystemLibrary::ArrayCompareElements(_bytes, ProgressionSaveData[Type->GetFName()].SaveByteArray);
}

void UPulseSaveData::InvalidateCache(TSubclassOf<UObject> Type)
{
	if (!ProgressionCachedData.Contains(Type))
		return;
	ProgressionCachedData.Remove(Type);
}

void UPulseSaveData::InvalidateAllCache()
{
	TArray<TObjectPtr<UClass>> Types;
	auto keyCount = ProgressionCachedData.GetKeys(Types);
	if (keyCount <= 0)
		return;
	for (const auto& type : Types)
		InvalidateCache(type);
}


FString USaveMetaProcessor::BuildMetaDetails_Implementation(const FSaveMetaData Meta, const UPulseSaveData* SaveData) const
{
	return "";
}

void USaveMetaProcessor::SortMetaList_Implementation(TArray<FSaveMetaData>& MetaList) const
{
	if (MetaList.IsEmpty())
		return;
	MetaList.Sort([](const FSaveMetaData& A, const FSaveMetaData& B)-> bool { return A.LastSaveDate < B.LastSaveDate; });
}

void UGameSaveProvider::HandlePendingSaves()
{
	if (_pendingSaves.IsEmpty())
		return;
	FPendingSave Pending;
	if (!_pendingSaves.Dequeue(Pending))
		return;
	UObject* SavedObject = nullptr;
	if (!UPulseSystemLibrary::DeserializeObjectFromBytes(Pending.SaveDataPack.SaveByteArray, SavedObject))
		return;
	BeginSave_Internal(Pending.UserProfile, Pending.Meta, Cast<UPulseSaveData>(SavedObject));
}


void UGameSaveProvider::Initialization_Internal(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings)
{
	Initialization(SaveManager, ProjectSettings);
}

void UGameSaveProvider::Initialization_Implementation(UPulseSaveManager* SaveManager, UCoreProjectSetting* ProjectSettings)
{
}

void UGameSaveProvider::Deinitialization_Internal(UPulseSaveManager* SaveManager)
{
	Deinitialization(SaveManager);
}

void UGameSaveProvider::Deinitialization_Implementation(UPulseSaveManager* SaveManager)
{
}


void UGameSaveProvider::BeginSave_Internal(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData)
{
	if (!SaveData || User.LocalID.IsEmpty() || User.LocalID != SaveMeta.UserLocalID)
	{
		_saveMetaQueue.Enqueue(SaveMeta);
		EndSave(false);
		return;
	}
	if (SaveMeta.bIsAnAutoSave && !bAllowAutoSaves)
	{
		_saveMetaQueue.Enqueue(SaveMeta);
		EndSave(false);
		return;
	}
	if (GetStatus() != EPulseSaveStatus::Idle)
	{
		TArray<uint8> Bytes;
		if (UPulseSystemLibrary::SerializeObjectToBytes(SaveData, Bytes))
		{
			FPendingSave pending;
			pending.Meta = SaveMeta;
			pending.SaveDataPack = Bytes;
			pending.UserProfile = User;
			_pendingSaves.Enqueue(pending);
		}
		_saveMetaQueue.Enqueue(SaveMeta);
		EndSave(false);
		return;
	}
	_saveMetaQueue.Enqueue(SaveMeta);
	BeginSave(User, SaveMeta, SaveData);
}

void UGameSaveProvider::BeginSave_Implementation(const FUserProfile& User, const FSaveMetaData& SaveMeta, UPulseSaveData* SaveData)
{
}

void UGameSaveProvider::EndSave(bool bSuccess)
{
	FSaveMetaData SaveMeta;
	if (!_saveMetaQueue.Dequeue(SaveMeta))
	{
		OnSavedGame.Broadcast(this->GetClass(), SaveMeta, false);
		HandlePendingSaves();
		return;
	}
	if (bSuccess)
	{
		_lastSaveSlotName = SaveMeta.GetSlotName();
		if (SaveMeta.bIsAnAutoSave)
			UPulseSystemLibrary::MapAddOrUpdateValue(_autoSaveSlotBufferIndexes, SaveMeta.SlotIndex, SaveMeta.SlotBufferIndex);
		else
			UPulseSystemLibrary::MapAddOrUpdateValue(_manualSaveSlotBufferIndexes, SaveMeta.SlotIndex, SaveMeta.SlotBufferIndex);
	}
	OnSavedGame.Broadcast(this->GetClass(), SaveMeta, bSuccess);
}


void UGameSaveProvider::BeginLoadMeta_Internal(const FUserProfile& User)
{
	if (User.LocalID.IsEmpty() || GetStatus() != EPulseSaveStatus::Idle)
	{
		_loadMetaDataQueue.Enqueue(true);
		EndLoadMeta({});
		return;
	}
	_loadMetaDataQueue.Enqueue(true);
	BeginLoadMeta(User);
}

void UGameSaveProvider::BeginLoadMeta_Implementation(const FUserProfile& Userprofile)
{
}

void UGameSaveProvider::EndLoadMeta(const TArray<FSaveMetaData>& MetaDataList)
{
	bool result = false;
	TArray<FSaveMetaData> MetaList;
	if (!_loadMetaDataQueue.Dequeue(result) || MetaDataList.IsEmpty())
	{
		OnLoadedMetas.Broadcast(this->GetClass(), {});
		return;
	}	
	if (MetaDataList.Num() > 0)
	{
		FDateTime autoTime = FDateTime::MinValue();
		FDateTime manualTime = FDateTime::MinValue();
		FDateTime cumulTime = FDateTime::MinValue();
		for (int i = MetaDataList.Num() - 1; i >= 0; i--)
		{
			const auto meta = MetaDataList[i];
			if (meta.bIsAnAutoSave && meta.LastSaveDate > autoTime)
			{
				UPulseSystemLibrary::MapAddOrUpdateValue(_autoSaveSlotBufferIndexes, meta.SlotIndex, meta.SlotBufferIndex);
				autoTime = meta.LastSaveDate;
			}
			else if (!meta.bIsAnAutoSave && meta.LastSaveDate > manualTime)
			{
				UPulseSystemLibrary::MapAddOrUpdateValue(_manualSaveSlotBufferIndexes, meta.SlotIndex, meta.SlotBufferIndex);
				manualTime = meta.LastSaveDate;
			}
			if (meta.LastSaveDate > cumulTime)
			{
				_lastSaveSlotName = meta.GetSlotName();
				cumulTime = meta.LastSaveDate;
			}
		}
	}
	FSaveMetaDataPack pack;
	pack.MetaDataList = MetaDataList;
	OnLoadedMetas.Broadcast(this->GetClass(), pack);
}


void UGameSaveProvider::BeginLoadGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta)
{
	if (User.LocalID.IsEmpty() || User.LocalID != Meta.UserLocalID || Meta.GetSlotName().IsEmpty())
	{
		_loadMetaQueue.Enqueue(Meta);
		EndLoadGame(nullptr);
		return;
	}
	_loadMetaQueue.Enqueue(Meta);
	BeginLoadGame(User, Meta);
}

void UGameSaveProvider::BeginLoadGame_Implementation(const FUserProfile& Userprofile, const FSaveMetaData& Meta)
{
}

void UGameSaveProvider::EndLoadGame(UPulseSaveData* LoadedData)
{
	FSaveMetaData Meta;
	if (!_loadMetaQueue.Dequeue(Meta))
	{
		OnLoadedGame.Broadcast(this->GetClass(), nullptr, {});
		return;
	}
	OnLoadedGame.Broadcast(this->GetClass(), LoadedData, Meta);
}


void UGameSaveProvider::BeginDeleteGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta)
{
	if (User.LocalID.IsEmpty() || User.LocalID != Meta.UserLocalID)
	{
		_deleteMetaQueue.Enqueue(Meta);
		EndDeleteGame(false);
		return;
	}
	_deleteMetaQueue.Enqueue(Meta);
	BeginDeleteGame(User, Meta);
}

void UGameSaveProvider::BeginDeleteGame_Implementation(const FUserProfile& User, const FSaveMetaData& Meta)
{
}

void UGameSaveProvider::EndDeleteGame(bool bSuccess)
{
	FSaveMetaData Meta;
	if (!_deleteMetaQueue.Dequeue(Meta))
	{
		OnDeletedGame.Broadcast(this->GetClass(), Meta, false);
		return;
	}
	OnDeletedGame.Broadcast(this->GetClass(), Meta, bSuccess);
}

bool UGameSaveProvider::GetSavingMeta(FSaveMetaData& OutMeta) const
{
	if (_saveMetaQueue.IsEmpty())
		return false;
	if (!_saveMetaQueue.Peek(OutMeta))
		return false;
	return true;
}


FString UGameSaveProvider::GetSlotName(const FSaveMetaData& Meta, bool bIsMetaSlot)
{
	return Meta.GetSlotName(bIsMetaSlot);
}

EPulseSaveStatus UGameSaveProvider::GetStatus() const
{
	if (!_loadMetaQueue.IsEmpty())
		return EPulseSaveStatus::Loading;
	else if (!_loadMetaDataQueue.IsEmpty())
		return EPulseSaveStatus::LoadingMetas;
	else if (!_deleteMetaQueue.IsEmpty())
		return EPulseSaveStatus::Deleting;
	else if (!_saveMetaQueue.IsEmpty())
		return EPulseSaveStatus::Saving;
	return EPulseSaveStatus::Idle;
}

int32 UGameSaveProvider::GetBufferSlot_Implementation(const int32 SlotIndex, bool bIsAutoSaveSlot)
{
	return 0;
}

bool UGameSaveProvider::IsLastSavedMeta_Implementation(const FSaveMetaData& Meta) const
{
	return false;
}
