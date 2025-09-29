// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/SaveGameSubModule/PulseSaveManager.h"

#include "JsonObjectConverter.h"
#include "Core/PulseCoreModule.h"
#include "Core/PulseSystemLibrary.h"
#include "DataWrappers/ChaosVDAccelerationStructureDataWrappers.h"
#include "Kismet/GameplayStatics.h"


bool UPulseSaveData::CheckCacheIntegrity(TSubclassOf<UObject> Type) const
{
	if (!ProgressionCachedDatas.Contains(Type))
		return true;
	TArray<uint8> _bytes;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(ProgressionCachedDatas[Type], _bytes))
		return false;
	if (!ProgressionSaveDatas.Contains(Type->GetFName()))
		return false;
	return UPulseSystemLibrary::CompareCollectionsElements(_bytes, ProgressionSaveDatas[Type->GetFName()].SaveByteArray);
}

void UPulseSaveData::InvalidateCache(TSubclassOf<UObject> Type)
{
	if (!ProgressionCachedDatas.Contains(Type))
		return;
	ProgressionCachedDatas.Remove(Type);
}

void UPulseSaveData::InvalidateAllCache()
{
	TArray<TObjectPtr<UClass>> Types;
	auto keyCount = ProgressionCachedDatas.GetKeys(Types);
	if (keyCount <= 0)
		return;
	for (const auto type : Types)
		InvalidateCache(type);
}



void UPulseSaveManagerParams::SetMetaCount(int32 ManualSlotCount, int32 AutoSaveMetaCount, int32 ManualSaveMetaCount)
{
	UPulseSystemLibrary::MatchCollectionSize(AutoSavesTambour, 1, false);
	UPulseSystemLibrary::MatchCollectionSize(AutoSavesTambour[0].SaveTambour, AutoSaveMetaCount, true);
	UPulseSystemLibrary::MatchCollectionSize(ManualSavesTambour, ManualSlotCount, false);
	for (int i = 0; i < ManualSlotCount; i++)
		UPulseSystemLibrary::MatchCollectionSize(ManualSavesTambour[i].SaveTambour, ManualSaveMetaCount, true);
}

void UPulseSaveManager::OnSavedGame_Internal(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	bool wasAutoSave = SlotName.Contains(_baseAutoSaveSlotName.ToString());
	SavingGame = FMath::Max(SavingParams - 1, 0);
	OnGameSaved.Broadcast(SlotName, UserIndex, bSuccess, wasAutoSave);
}

void UPulseSaveManager::OnLoadedGame_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
	bIsLoadingGame = false;
	ELoadSaveResponse response = LoadedGameData ? Success : NullSaveData;
	UPulseSaveData* loaded = Cast<UPulseSaveData>(LoadedGameData);
	if (loaded)
		loaded->InvalidateAllCache();
	if (response == Success)
	{
		response = loaded ? Success : UnknownSaveDataType;
		if (response == Success)
		{
			if (VerifyLoadSaveHash(SlotName, UserIndex, loaded))
			{
				if (PerUserSavedProgression.Contains(UserIndex))
					PerUserSavedProgression[UserIndex] = loaded;
				else
					PerUserSavedProgression.Add(UserIndex, loaded);
			}
			else
				response = HashMismatch;
		}
	}
	OnGameLoaded.Broadcast(response, UserIndex, loaded);
}

void UPulseSaveManager::OnLoadedParams_Internal(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData)
{
	UPulseSaveManagerParams* loadedParams = Cast<UPulseSaveManagerParams>(LoadedGameData);
	InitializationCountDown--;
	if (!loadedParams)
	{
		loadedParams = Cast<UPulseSaveManagerParams>(UGameplayStatics::CreateSaveGameObject(UPulseSaveManagerParams::StaticClass()));
		loadedParams->SetMetaCount(GetSaveSlotCount(false), GetPerTambourCount(true), GetPerTambourCount(false));
	}
	if (SaveParams.Contains(UserIndex))
		SaveParams[UserIndex] = loadedParams;
	else
		SaveParams.Add(UserIndex, loadedParams);
}

FString UPulseSaveManager::GetNextTambourSlotName(const int32 UserIndex, const int32 SlotIndex, int32& OutUsedTambourIndex, bool bIsAutoSave)
{
	if (!SaveParams.Contains(UserIndex) || !SaveParams[UserIndex])
		return "";
	int32 lastUsedTambourIndex = bIsAutoSave
		                             ? SaveParams[UserIndex]->LastAutoSaveTambourIndex
		                             : (SaveParams[UserIndex]->LastManualSaveSlotIndex == SlotIndex ? SaveParams[UserIndex]->LastManualSaveTambourIndex : -1);
	int32 tambourIndexToUse = (lastUsedTambourIndex + 1) % GetPerTambourCount(bIsAutoSave);
	FString slotName;
	if (!GetSlotName(SlotIndex, slotName, bIsAutoSave, tambourIndexToUse))
		return "";
	OutUsedTambourIndex = tambourIndexToUse;
	return slotName;
}

void UPulseSaveManager::LoadSaveParamForUser(int32 UserIndex)
{
	if (!IsValidSaveUser(UserIndex))
		return;
	InitializationCountDown++;
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	auto slot = FString::Printf(TEXT("SaveParams_%d"), UserIndex);
	LoadedDelegate.BindUObject(this, &UPulseSaveManager::OnLoadedParams_Internal);
	UGameplayStatics::AsyncLoadGameFromSlot(slot, 0, LoadedDelegate);
}

bool UPulseSaveManager::SaveParamsForUser(const int32 UserIndex, const int32 SlotIndex, const int32 TambourIndex, const FSaveMetaData& SaveMetaData, bool bIsAutoSave)
{
	if (!IsValidSaveSlot(UserIndex, SlotIndex, bIsAutoSave))
		return false;
	if (!SaveParams.Contains(UserIndex))
		return false;
	if (!SaveParams[UserIndex])
		return false;
	if (bIsAutoSave)
	{
		SaveParams[UserIndex]->LastAutoSaveTambourIndex = TambourIndex;
		UPulseSystemLibrary::MatchCollectionSize(SaveParams[UserIndex]->AutoSavesTambour, 1, false);
	}
	else
	{
		SaveParams[UserIndex]->LastManualSaveSlotIndex = SlotIndex;
		SaveParams[UserIndex]->LastManualSaveTambourIndex = TambourIndex;
		UPulseSystemLibrary::MatchCollectionSize(SaveParams[UserIndex]->ManualSavesTambour, SlotIndex + 1, false);
	}
	auto saveMeta = SaveMetaData;
	saveMeta.LastSaveDate = FDateTime::Now();
	UPulseSystemLibrary::SetAtIndexWhileAdding(
		bIsAutoSave ? SaveParams[UserIndex]->AutoSavesTambour[0].SaveTambour : SaveParams[UserIndex]->ManualSavesTambour[SlotIndex].SaveTambour, TambourIndex, saveMeta);
	auto slot = FString::Printf(TEXT("SaveParams_%d"), UserIndex);
	FAsyncSaveGameToSlotDelegate SavedDelegate;
	SavedDelegate.BindLambda([&](const FString& slotName, const int32 userIdx, bool bSuccess)-> void { SavingParams = FMath::Max(SavingParams - 1, 0); });
	SavingParams++;
	UGameplayStatics::AsyncSaveGameToSlot(SaveParams[UserIndex], slot, UserIndex, SavedDelegate);
	return true;
}

bool UPulseSaveManager::SaveGameInternal(const int32 UserIndex, const int32 SlotIndex, bool bIsAutoSave)
{
	if (SlotIndex < 0)
		return false;
	int32 tambourIndex = 0;
	auto slot = GetNextTambourSlotName(UserIndex, SlotIndex, tambourIndex, bIsAutoSave);
	if (slot.IsEmpty())
		return false;
	auto SaveGameInstance = PerUserSavedProgression.Contains(UserIndex) ? PerUserSavedProgression[UserIndex] : nullptr;
	if (!SaveGameInstance)
		SaveGameInstance = Cast<UPulseSaveData>(UGameplayStatics::CreateSaveGameObject(UPulseSaveData::StaticClass()));
	if (SaveGameInstance)
	{
		// Set as the Current user progression before calling presave
		if (PerUserSavedProgression.Contains(UserIndex))
			PerUserSavedProgression[UserIndex] = SaveGameInstance;
		else
			PerUserSavedProgression.Add(UserIndex, SaveGameInstance);
		// Set up the (optional) delegate.
		FAsyncSaveGameToSlotDelegate SavedDelegate;
		// USomeUObjectClass::SaveGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, bool bSuccess
		SavedDelegate.BindUObject(this, &UPulseSaveManager::OnSavedGame_Internal);
		// Notify all wew are about to save
		USaveMetaWrapper* SaveMetaWrapper = NewObject<USaveMetaWrapper>(this);
		OnGameAboutToSave.Broadcast(slot, UserIndex, SaveMetaWrapper, bIsAutoSave);
		SavingGame++;
		// Hashing
		TArray<uint8> _byteDatas;
		if (!UPulseSystemLibrary::SerializeObjectToBytes(SaveGameInstance, _byteDatas))
		{
			SavedDelegate.ExecuteIfBound(slot, UserIndex, false);
			return false;
		}
		auto hash = FMD5::HashBytes(&_byteDatas[0], _byteDatas.Num());
		SaveMetaWrapper->SaveMetaData.SaveHash = hash;
		// Start async save process.
		auto meta = SaveMetaWrapper->SaveMetaData;
		SaveParamsForUser(UserIndex, SlotIndex, tambourIndex, meta, bIsAutoSave);
		UGameplayStatics::AsyncSaveGameToSlot(SaveGameInstance, slot, UserIndex, SavedDelegate);
		return true;
	}
	return false;
}


FName UPulseSaveManager::GetSubModuleName() const
{
	return Super::GetSubModuleName();
}

bool UPulseSaveManager::WantToTick() const
{
	return InitializationCountDown <= 0 && _useAutoSave;
}

bool UPulseSaveManager::TickWhenPaused() const
{
	return false;
}

void UPulseSaveManager::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	if (UPulseCoreModule* CoreModule = Cast<UPulseCoreModule>(OwningModule))
	{
		if (auto config = CoreModule->GetProjectConfig())
		{
			_saveUserCount = config->SaveUserCount;
			_perManualSlotTambour = config->PerSlotTambour;
			_perAutoSlotTambour = config->PerSlotTambour;
			_manualSaveSlotCount = config->ManualSaveSlots;
			_baseAutoSaveSlotName = config->AutoSaveSlotBaseName;
			_baseManualSaveSlotName = config->ManualSaveSlotBaseName;
			_useAutoSave = config->bUseAutoSave;
			_autoSaveInterval = config->AutoSaveIntervalInSeconds;
			_useSaveCacheValidationOnRead = config->bUseSaveCacheInvalidationOnRead;
		}
	}
	_autoSaveCountDown = _autoSaveInterval;
	for (int i = 0; i < _saveUserCount; i++)
		LoadSaveParamForUser(i);
}

void UPulseSaveManager::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
}

void UPulseSaveManager::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
	// handle Auto save logic
	if (_autoSaveCountDown > 0)
	{
		_autoSaveCountDown -= DeltaTime;
		if (_autoSaveCountDown <= 0)
		{
			for (int i = 0; i < GetSaveUserCount(); i++)
				AutoSaveGame(i);
			_autoSaveCountDown = _autoSaveInterval;
		}
	}
}


bool UPulseSaveManager::AutoSaveGame(int32 UserIndex)
{
	return SaveGameInternal(UserIndex, 0, true);
}

bool UPulseSaveManager::ManualSaveGame(int32 UserIndex, int32 SaveSlot)
{
	return SaveGameInternal(UserIndex, SaveSlot, false);
}

bool UPulseSaveManager::LoadGame(const FString SlotName, int32 UserIndex)
{
	if (bIsLoadingGame)
		return false;
	if (SlotName.IsEmpty())
		return false;
	if (!(SlotName.Contains(_baseAutoSaveSlotName.ToString()) || SlotName.Contains(_baseManualSaveSlotName.ToString())))
		return false;
	if (!IsValidSaveUser(UserIndex))
		return false;
	// Set up the delegate.
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	// USomeUObjectClass::LoadGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData
	LoadedDelegate.BindUObject(this, &UPulseSaveManager::OnLoadedGame_Internal);
	// Notify all we are about to load game
	OnGameAboutToLoad.Broadcast(SlotName, UserIndex);
	// Start the load process
	bIsLoadingGame = true;
	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, UserIndex, LoadedDelegate);
	return true;
}

bool UPulseSaveManager::DeleteSave(const int32 UserIndex, const int32 SlotIndex, bool bIsAutoSaveSlot)
{
	if (bIsLoadingGame || SavingParams > 0 || SavingGame > 0)
		return false;
	if (!IsValidSaveSlot(UserIndex, SlotIndex))
		return false;
	TArray<FString> deletedSlotsNames = {};
	for (int i = GetPerTambourCount(bIsAutoSaveSlot); i >= 0; i--)
	{
		FString SlotName;
		if (!GetSlotName(SlotIndex, SlotName, bIsAutoSaveSlot, i))
			continue;
		if (UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex))
			deletedSlotsNames.Add(SlotName);
	}
	if (deletedSlotsNames.Num() > 0)
	{
		if (SaveParams.Contains(UserIndex) && SaveParams[UserIndex])
		{
			if (bIsAutoSaveSlot)
			{
				UPulseSystemLibrary::MatchCollectionSize(SaveParams[UserIndex]->AutoSavesTambour, GetPerTambourCount(true));
				SaveParams[UserIndex]->LastAutoSaveTambourIndex = -1;
			}
			else
			{
				if (SaveParams[UserIndex]->LastManualSaveSlotIndex >= SlotIndex)
					SaveParams[UserIndex]->LastManualSaveSlotIndex = FMath::Max(SaveParams[UserIndex]->LastManualSaveSlotIndex - 1, 0);
				SaveParams[UserIndex]->ManualSavesTambour.Empty();
				UPulseSystemLibrary::MatchCollectionSize(SaveParams[UserIndex]->ManualSavesTambour, GetPerTambourCount(false));
				SaveParams[UserIndex]->LastManualSaveTambourIndex = -1;
			}
		}
		OnSaveDeleted.Broadcast(UserIndex, deletedSlotsNames);
	}
	return deletedSlotsNames.Num() > 0;
}

bool UPulseSaveManager::DeleteAllSaves()
{
	int32 deletedCount = 0;
	for (int i = GetSaveUserCount() - 1; i >= 0; i--)
	{
		deletedCount += DeleteSave(i, 0, true);
		for (int j = GetSaveSlotCount() - 1; j >= 0; j--)
			deletedCount += DeleteSave(i, j);
	}
	return deletedCount > 0;
}

bool UPulseSaveManager::ReadSavedValue(TSubclassOf<UObject> Type, UObject*& OutResult, int32 UserIndex)
{
	if (!PerUserSavedProgression.Contains(UserIndex))
		return false;
	if (!PerUserSavedProgression[UserIndex])
		return false;
	auto CachedProgression = &PerUserSavedProgression[UserIndex]->ProgressionCachedDatas;
	if (_useSaveCacheValidationOnRead && !PerUserSavedProgression[UserIndex]->CheckCacheIntegrity(Type))
		PerUserSavedProgression[UserIndex]->InvalidateCache(Type);
	if (!CachedProgression->IsEmpty() && CachedProgression->Contains(Type) && (*CachedProgression)[Type])
	{
		OutResult = (*CachedProgression)[Type];
		return true;
	}
	auto SavedProgression = &PerUserSavedProgression[UserIndex]->ProgressionSaveDatas;
	if (SavedProgression->IsEmpty())
		return false;
	if (!SavedProgression->Contains(Type->GetFName()))
		return false;
	OutResult = NewObject<UObject>(this, Type);
	auto byteAr = (*SavedProgression)[Type->GetFName()].SaveByteArray;
	if (!UPulseSystemLibrary::DeserializeObjectFromBytes(byteAr, OutResult))
		return false;
	UPulseSystemLibrary::AddOrReplace(*CachedProgression, TObjectPtr<UClass>(Type), TObjectPtr<UObject>(OutResult));
	return true;
}

bool UPulseSaveManager::WriteAsSavedValue(UObject* Value, int32 UserIndex)
{
	if (!PerUserSavedProgression.Contains(UserIndex))
		return false;
	if (!PerUserSavedProgression[UserIndex])
		return false;
	auto Cache = &PerUserSavedProgression[UserIndex]->ProgressionCachedDatas;
	auto SavedProgression = &PerUserSavedProgression[UserIndex]->ProgressionSaveDatas;
	if (!Value)
		return false;
	TArray<uint8> _datas;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(Value, _datas))
		return false;
	if (SavedProgression->IsEmpty())
	{
		SavedProgression->Add(Value->GetClass()->GetFName(), FSaveDataPack(_datas));
		UPulseSystemLibrary::AddOrReplace(*Cache, TObjectPtr<UClass>(Value->GetClass()), TObjectPtr<UObject>(Value));
		return true;
	}
	else
	{
		if (!SavedProgression->Contains(Value->GetClass()->GetFName()))
			SavedProgression->Add(Value->GetClass()->GetFName(), FSaveDataPack(_datas));
		else
			(*SavedProgression)[Value->GetClass()->GetFName()] = FSaveDataPack(_datas);
		UPulseSystemLibrary::AddOrReplace(*Cache, TObjectPtr<UClass>(Value->GetClass()), TObjectPtr<UObject>(Value));
		return true;
	}
}

bool UPulseSaveManager::TryGetSaveMetaDatas(const int32 SlotIndex, TArray<FSaveMetaData>& OutMetaDatas, int32 UserIndex, bool bAutoSaveSlot, bool bOnlyValidSaveDateTime) const
{
	if (InitializationCountDown > 0)
		return false;
	if (!IsValidSaveSlot(UserIndex, SlotIndex, bAutoSaveSlot))
		return false;
	if (!SaveParams.Contains(UserIndex))
		return false;
	auto collection = bAutoSaveSlot ? &SaveParams[UserIndex]->AutoSavesTambour : &SaveParams[UserIndex]->ManualSavesTambour;
	if (!collection)
		return false;
	if (!collection->IsValidIndex(SlotIndex))
		return false;
	bool found = false;
	for (int i = GetPerTambourCount(bAutoSaveSlot) - 1; i >= 0; i--)
	{
		if (!(*collection)[SlotIndex].SaveTambour.IsValidIndex(i))
			continue;
		if (bOnlyValidSaveDateTime && (*collection)[SlotIndex].SaveTambour[i].LastSaveDate <= FDateTime::MinValue())
			continue;
		OutMetaDatas.Insert((*collection)[SlotIndex].SaveTambour[i], 0);
		found = true;
	}
	return found;
}

bool UPulseSaveManager::TryGetSaveMetaData(const FString& SlotName, const int32 UserIndex, FSaveMetaData& OutMetaData) const
{
	TEnumAsByte<ESaveSlotTypeInfos> slotTypeInfos = ESaveSlotTypeInfos::UnknownSaveSlot;
	int32 slotIndex = INDEX_NONE;
	int32 tambourIndex = INDEX_NONE;
	if (!TryGetSlotInfos(SlotName, slotTypeInfos, slotIndex, tambourIndex))
		return false;
	TArray<FSaveMetaData> savedMetaDatas;
	if (!TryGetSaveMetaDatas(slotIndex, savedMetaDatas, UserIndex, slotTypeInfos == AutoSaveSlot))
		return false;
	if (!savedMetaDatas.IsValidIndex(tambourIndex))
		return false;
	OutMetaData = savedMetaDatas[slotIndex];
	return true;
}

bool UPulseSaveManager::TryGetSlotInfos(const FString& SlotName, TEnumAsByte<ESaveSlotTypeInfos>& OutSlotType, int32& OutSlotIndex, int32& OutTambourIndex) const
{
	if (SlotName.IsEmpty())
		return false;
	FString name;
	FString values;
	if (!SlotName.Split("_", &name, &values))
		return false;
	FString slotI;
	FString tambourI;
	if (!values.Split("_", &slotI, &tambourI))
		return false;
	OutSlotType = name == _baseManualSaveSlotName ? ManualSaveSlot : (name == _baseAutoSaveSlotName ? AutoSaveSlot : UnknownSaveSlot);
	if (!slotI.IsNumeric())
		return false;
	OutSlotIndex = FCString::Atoi(*slotI);
	if (tambourI.IsNumeric())
	{
		OutTambourIndex = FCString::Atoi(*tambourI);
	}
	else
	{
		OutTambourIndex = 0;
	}
	return true;
}

bool UPulseSaveManager::GetSlotName(const int32 SlotIndex, FString& OutSlotName, bool bAutoSaveSlot, const int32 TambourIndex) const
{
	if (SlotIndex < 0)
		return false;
	if (!(TambourIndex >= 0 && TambourIndex < GetPerTambourCount(bAutoSaveSlot)))
		return false;
	OutSlotName = FString::Printf(TEXT("%s_%d_%d"), *(bAutoSaveSlot ? _baseAutoSaveSlotName : _baseManualSaveSlotName).ToString(), SlotIndex, TambourIndex);
	return true;
}

int32 UPulseSaveManager::GetPerTambourCount(bool bAutoSaveSlot) const
{
	return bAutoSaveSlot ? _perAutoSlotTambour : _perManualSlotTambour;
}

int32 UPulseSaveManager::GetSaveSlotCount(bool bAutoSaveSlot) const
{
	return bAutoSaveSlot ? 1 : _manualSaveSlotCount;
}

bool UPulseSaveManager::IsValidSaveSlot(const int32 UserIndex, const int32 SlotIndex, bool bAutoSaveSlot) const
{
	return IsValidSaveUser(UserIndex) && SlotIndex >= 0 && SlotIndex < GetSaveSlotCount(bAutoSaveSlot);
}

FVector2D UPulseSaveManager::GetLastUsedSaveUID(const int32 UserIndex, bool bAutoSaveSlot) const
{
	if (!SaveParams.Contains(UserIndex) || !SaveParams[UserIndex])
		return FVector2D(-1);
	int32 lastSlotIndexUsed = bAutoSaveSlot ? 0 : SaveParams[UserIndex]->LastManualSaveSlotIndex;
	int32 lastTambourIndexUsed = bAutoSaveSlot ? SaveParams[UserIndex]->LastAutoSaveTambourIndex : SaveParams[UserIndex]->LastManualSaveTambourIndex;
	return FVector2D(lastSlotIndexUsed, lastTambourIndexUsed);
}

bool UPulseSaveManager::IsValidSaveUser(int32 UserIndex) const
{
	return UserIndex >= 0 && UserIndex < GetSaveUserCount();
}

int32 UPulseSaveManager::GetSaveUserCount() const
{
	return _saveUserCount;
}

bool UPulseSaveManager::VerifyLoadSaveHash(const FString& SlotName, const int32 UserIndex, UPulseSaveData* Data) const
{
	if (!Data)
		return false;
	// Get Meta data
	FSaveMetaData meta;
	if (!TryGetSaveMetaData(SlotName, UserIndex, meta))
		return false;
	// Hash file
	TArray<uint8> _byteDatas;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(Data, _byteDatas))
		return false;
	auto hash = FMD5::HashBytes(&_byteDatas[0], _byteDatas.Num());
	// Compare with meta data hash
	return meta.SaveHash == hash;
}
