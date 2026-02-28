// Copyright © by Tyni Boat. All Rights Reserved.


#include "SaveGame/PulseSaveManager.h"

#include "PulseGameFramework.h"
#include "Core/PulseSystemLibrary.h"
#include "SaveGame/IPulseSavableObject.h"
#include "SaveGame/LocalGameSaveProvider.h"
#include "Kismet/GameplayStatics.h"


void UPulseSaveManager::OnProviderEvent_Saved(TSubclassOf<UGameSaveProvider> Class, FSaveMetaData SaveMetaData, bool Success)
{
	// Save Game Response
	if (_savingClassSet.Contains(Class))
	{
		UE_LOG(LogPulseSave, Log, TEXT("Save Operation: Provider %s, slot %s"), *Class->GetName(), *SaveMetaData.GetSlotName());
		_savingClassSet.Remove(Class);
		if (_savingClassSet.IsEmpty())
		{
			OnGameSaved.Broadcast();
			OnGameSaved_Raw.Broadcast();
		}
	}
	else
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Save Operation: Unregistered saved operation: Provider %s, slot %s"), *Class->GetName(), *SaveMetaData.GetSlotName());
		if (Success)
		{
			OnGameSaved.Broadcast();
			OnGameSaved_Raw.Broadcast();
		}
	}
}

void UPulseSaveManager::OnProviderEvent_Loaded(TSubclassOf<UGameSaveProvider> Class, UPulseSaveData* PulseSaveData, FSaveMetaData SaveMetaData)
{
	UE_LOG(LogPulseSave, Log, TEXT("Loading Operation: Provider %s, Valid? %d, Slot %s"), *Class->GetName(), PulseSaveData != nullptr, *SaveMetaData.GetSlotName());
	if (!ProvidersMap.Contains(Class))
		return;
	if (!PulseSaveData)
		return;
	if (_useLoadHashVerification && PulseSaveData != nullptr && ProvidersMap[Class])
	{
		if (!VerifyLoadSaveHash(SaveMetaData, PulseSaveData))
		{
			UE_LOG(LogPulseSave, Log, TEXT("Loading Operation: Hash Mismatch Provider %s, slot %s"), *Class->GetName(), *SaveMetaData.GetSlotName());
			return;
		}
	}
	SavedProgression = PulseSaveData;

	// Notify all just loaded
	OnGameLoaded.Broadcast();
	OnGameLoaded_Raw.Broadcast();
	UPulseSystemLibrary::ForeachActorInterface(this, UIPulseSavableObject::StaticClass(), [w_this = MakeWeakObjectPtr(this)](AActor* actor)
	{
		IIPulseSavableObject::Execute_OnPostLoadEvent(actor);
		if (!w_this.IsValid())
			return;
		UObject* OutObj = nullptr;
		if (w_this->ReadSavedValue(IIPulseSavableObject::Execute_GetSaveObjectClass(actor), OutObj))
		{
			IIPulseSavableObject::Execute_OnLoadedSaveObject(actor, OutObj);
		}
	});
}

void UPulseSaveManager::OnProviderEvent_LoadedMetas(TSubclassOf<UGameSaveProvider> Class, FSaveMetaDataPack SaveMetaDataPack)
{
	UE_LOG(LogPulseSave, Log, TEXT("Meta Loading Operation: Provider %s, Metadata count? %d, valid provider? %d"), *Class->GetName(), SaveMetaDataPack.MetaDataList.Num(),
	       ProvidersMap.Contains(Class));
	if (!ProvidersMap.Contains(Class))
		return;
	// Notify all just loaded meta
	FSaveMetaDataBundle MetaBundle;
	MetaBundle.MetaDataBundle.Add(Class, SaveMetaDataPack);
	OnLoadedMetas.Broadcast(MetaBundle);
}

void UPulseSaveManager::OnProviderEvent_Deleted(TSubclassOf<UGameSaveProvider> Class, FSaveMetaData SaveMetaData, bool Success)
{
	UE_LOG(LogPulseSave, Log, TEXT("Deleting Operation: Deleted %s On provider %s, Success? %d, valid Provider? %d"), *SaveMetaData.GetSlotName(), *Class->GetName(), Success,
	       ProvidersMap.Contains(Class));
	if (!ProvidersMap.Contains(Class))
		return;
	// Notify all just Deleted
	OnDeleted.Broadcast(Class, SaveMetaData);
}


bool UPulseSaveManager::VerifyLoadSaveHash(const FSaveMetaData& Meta, UPulseSaveData* LoadedData) const
{
	if (!LoadedData)
		return false;
	// Hash file
	TArray<uint8> _byteDatas;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(LoadedData, _byteDatas))
		return false;
	auto hash = FMD5::HashBytes(&_byteDatas[0], _byteDatas.Num());
	// Compare with meta data hash
	return Meta.SaveHash == hash;
}

void UPulseSaveManager::Save_Internal(const FUserProfile& User, UPulseSaveData* data, FDateTime Time, const int32& SlotIndex,
                                      const TArray<TSubclassOf<UGameSaveProvider>>& ProviderClasses,
                                      bool bAutoSave)
{
	if (!data)
		return;
	if (ProviderClasses.IsEmpty())
		return;
	bool haveAtLeastOneValid = false;
	for (int i = ProviderClasses.Num() - 1; i >= 0; i--)
	{
		if (ProvidersMap.Contains(ProviderClasses[i]) && ProvidersMap[ProviderClasses[i]])
		{
			if (!ProvidersMap[ProviderClasses[i]]->bAllowAutoSaves && bAutoSave)
				continue;
			haveAtLeastOneValid = true;
			break;
		}
	}
	if (!haveAtLeastOneValid)
		return;

	// Notify all we are about to save
	OnGameAboutToSave_Raw.Broadcast();
	OnGameAboutToSave.Broadcast();
	UPulseSystemLibrary::ForeachActorInterface(this, UIPulseSavableObject::StaticClass(), [w_this = MakeWeakObjectPtr(this)](AActor* actor)
	{
		IIPulseSavableObject::Execute_OnPreSaveEvent(actor);
		if (!w_this.IsValid())
			return;
		auto Class = IIPulseSavableObject::Execute_GetSaveObjectClass(actor);
		if (!Class)
			return;
		UObject* saveObj = IIPulseSavableObject::Execute_OnBuildSaveObject(actor, Class);
		if (!saveObj || !saveObj->IsA(IIPulseSavableObject::Execute_GetSaveObjectClass(actor)))
			return;
		w_this->WriteAsSavedValue(saveObj);
	});

	// Hashing
	TArray<uint8> _byteData;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(data, _byteData))
		return;
	FSaveMetaData Meta = {};
	Meta.LastSaveDate = Time;
	Meta.UserLocalID = User.LocalID;
	Meta.SlotIndex = SlotIndex;
	Meta.bIsAnAutoSave = bAutoSave;
	auto hash = FMD5::HashBytes(&_byteData[0], _byteData.Num());
	Meta.SaveHash = hash;
	if (MetaProcessor)
	{
		Meta.DetailsJson = MetaProcessor->BuildMetaDetails(Meta, data);
	}

	// Start async save process.	
	for (int i = 0; i < ProviderClasses.Num(); i++)
	{
		if (!ProviderClasses[i])
			continue;
		if (!ProvidersMap.Contains(ProviderClasses[i]))
			continue;
		const auto provider = ProvidersMap[ProviderClasses[i]];
		if (!provider)
			continue;
		_savingClassSet.Add(ProviderClasses[i]);
		UE_LOG(LogPulseSave, Log, TEXT("Save Operation: Started On Provider %s"), *provider->GetClass()->GetName());
		FSaveMetaData _meta = Meta;
		_meta.SlotBufferIndex = provider->GetBufferSlot(Meta.SlotIndex, Meta.bIsAnAutoSave);
		provider->BeginSave_Internal(User, _meta, data);
	}
}

void UPulseSaveManager::LoadGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta, const TSubclassOf<UGameSaveProvider>& ProviderClass)
{
	if (!ProviderClass)
		return;
	if (Meta.GetSlotName().IsEmpty())
		return;
	if (User.LocalID.IsEmpty())
		return;
	const auto status = GetSaveStatus();
	if (status >= EPulseSaveStatus::Loading)
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Loading Operation: Cannot load game because There is an ongoing Loading operation"));
		return;
	}
	if (!ProvidersMap.Contains(ProviderClass))
		return;
	const auto provider = ProvidersMap[ProviderClass];
	UE_LOG(LogPulseSave, Log, TEXT("Loading Operation: Started On Provider %s"), *provider->GetClass()->GetName());
	provider->BeginLoadGame_Internal(User, Meta);
}

void UPulseSaveManager::LoadMetas_Internal(const FUserProfile& User, const TSubclassOf<UGameSaveProvider>& ProviderClass)
{
	if (!ProviderClass)
	{
		UE_LOG(LogPulseSave, Error, TEXT("Meta Loading Operation: Cannot load Meta: Invalid provider Class"));
		return;
	}
	if (User.LocalID.IsEmpty())
	{
		UE_LOG(LogPulseSave, Error, TEXT("Meta Loading Operation: Cannot load Meta: User Local ID is Empty"));
		return;
	}
	if (!ProvidersMap.Contains(ProviderClass))
	{
		UE_LOG(LogPulseSave, Error, TEXT("Meta Loading Operation: Cannot load Meta: Provider Map doesn't contains this one. Provider: %s"), *ProviderClass->GetName());
		return;
	}
	const auto provider = ProvidersMap[ProviderClass];
	UE_LOG(LogPulseSave, Log, TEXT("Meta Loading Operation: Started On Provider %s"), *provider->GetClass()->GetName());
	provider->BeginLoadMeta_Internal(User);
}

void UPulseSaveManager::DeleteGame_Internal(const FUserProfile& User, const FSaveMetaData& Meta, const TSubclassOf<UGameSaveProvider>& ProviderClass)
{
	if (!ProviderClass)
		return;
	if (Meta.GetSlotName().IsEmpty())
		return;
	if (User.LocalID.IsEmpty())
		return;
	if (!ProvidersMap.Contains(ProviderClass))
		return;
	const auto provider = ProvidersMap[ProviderClass];
	UE_LOG(LogPulseSave, Log, TEXT("Deleting Operation: Started On Provider %s"), *provider->GetClass()->GetName());
	provider->BeginDeleteGame_Internal(User, Meta);
}


void UPulseSaveManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPulseSave, Log, TEXT("Save sub-system initialization started"));
	if (auto config = GetProjectSettings())
	{
		//Configs
		_useSaveCacheValidationOnRead = config->bUseSaveCacheInvalidationOnRead;
		_useLoadHashVerification = config->bUseLoadHashVerification;

		// Meta processor
		USaveMetaProcessor* metaProcessor = nullptr;
		if (config->SaveMetaProcessorClass)
		{
			metaProcessor = NewObject<USaveMetaProcessor>(this, config->SaveMetaProcessorClass);
		}
		if (metaProcessor)
		{
			MetaProcessor = metaProcessor;
			UE_LOG(LogPulseSave, Log, TEXT("Meta Processor set to %s"), *metaProcessor->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogPulseSave, Warning, TEXT("An Error occured when trying to create the Save Meta Processor"));
		}

		// Local provider
		UGameSaveProvider* localProvider = nullptr;
		if (config->LocalSaveProviderClass)
		{
			localProvider = NewObject<UGameSaveProvider>(this, config->LocalSaveProviderClass);
		}
		if (localProvider)
		{
			LocalProviderClass = localProvider->GetClass();
			ProvidersMap.Add(LocalProviderClass, localProvider);
			localProvider->OnSavedGame.AddUObject(this, &UPulseSaveManager::OnProviderEvent_Saved);
			localProvider->OnLoadedGame.AddUObject(this, &UPulseSaveManager::OnProviderEvent_Loaded);
			localProvider->OnLoadedMetas.AddUObject(this, &UPulseSaveManager::OnProviderEvent_LoadedMetas);
			localProvider->OnDeletedGame.AddUObject(this, &UPulseSaveManager::OnProviderEvent_Deleted);
			localProvider->Initialization_Internal(this, config);
			UE_LOG(LogPulseSave, Log, TEXT("Local save provider %s Created"), *localProvider->GetName());
		}
		else
		{
			UE_LOG(LogPulseSave, Warning, TEXT("An Error occured when trying to create the local save provider"));
		}
		// Other providers
		for (int i = 0; i < config->SaveProviderClasses.Num(); i++)
		{
			if (!config->SaveProviderClasses[i])
				continue;
			auto provider = NewObject<UGameSaveProvider>(this, config->SaveProviderClasses[i]);
			if (!provider)
				continue;
			if (ProvidersMap.Contains(provider->GetClass()))
				continue;
			ProvidersMap.Add(provider->GetClass(), provider);
			provider->OnSavedGame.AddUObject(this, &UPulseSaveManager::OnProviderEvent_Saved);
			provider->OnLoadedGame.AddUObject(this, &UPulseSaveManager::OnProviderEvent_Loaded);
			provider->OnLoadedMetas.AddUObject(this, &UPulseSaveManager::OnProviderEvent_LoadedMetas);
			provider->OnDeletedGame.AddUObject(this, &UPulseSaveManager::OnProviderEvent_Deleted);
			provider->Initialization_Internal(this, config);
			UE_LOG(LogPulseSave, Log, TEXT("Save provider %s Created"), *provider->GetName());
		}
	}
}

void UPulseSaveManager::Deinitialize()
{
	Super::Deinitialize();
	for (const auto& providerPair : ProvidersMap)
	{
		if (!providerPair.Value)
			continue;
		providerPair.Value->OnSavedGame.RemoveAll(this);
		providerPair.Value->OnLoadedGame.RemoveAll(this);
		providerPair.Value->OnLoadedMetas.RemoveAll(this);
		providerPair.Value->OnDeletedGame.RemoveAll(this);
		providerPair.Value->Deinitialization_Internal(this);
	}
}

EPulseSaveStatus UPulseSaveManager::GetSaveStatus() const
{
	EPulseSaveStatus status = EPulseSaveStatus::Idle;
	for (const auto& providerPair : ProvidersMap)
	{
		if (!providerPair.Value)
			continue;
		if (providerPair.Value->GetStatus() <= status)
			continue;
		status = providerPair.Value->GetStatus();
	}
	return status;
}

bool UPulseSaveManager::GetSaveProvider(TSubclassOf<UGameSaveProvider> Type, UGameSaveProvider*& OutInstance)
{
	if (!ProvidersMap.Contains(Type))
		return false;
	OutInstance = ProvidersMap[Type];
	return true;
}

bool UPulseSaveManager::GetSaveProviderClasses(TArray<TSubclassOf<UGameSaveProvider>>& OutType)
{
	if (ProvidersMap.IsEmpty())
		return false;
	ProvidersMap.GetKeys(OutType);
	return OutType.Num() > 0;
}

bool UPulseSaveManager::GetMetaProcessor(USaveMetaProcessor*& OutInstance)
{
	if (!MetaProcessor)
		return false;
	OutInstance = MetaProcessor;
	return true;
}

bool UPulseSaveManager::ReadSavedValue(TSubclassOf<UObject> Type, UObject*& OutResult)
{
	if (!Type)
		return false;
	if (!SavedProgression)
		return false;
	auto CachedProgression = &SavedProgression->ProgressionCachedData;
	if (_useSaveCacheValidationOnRead && !SavedProgression->CheckCacheIntegrity(Type))
		SavedProgression->InvalidateCache(Type);
	if (!CachedProgression->IsEmpty() && CachedProgression->Contains(Type) && (*CachedProgression)[Type])
	{
		OutResult = (*CachedProgression)[Type];
		return true;
	}
	auto Progression = &SavedProgression->ProgressionSaveData;
	if (Progression->IsEmpty())
		return false;
	if (!Progression->Contains(Type->GetFName()))
		return false;
	OutResult = NewObject<UObject>(this, Type);
	auto byteAr = (*Progression)[Type->GetFName()].SaveByteArray;
	if (!UPulseSystemLibrary::DeserializeObjectFromBytes(byteAr, OutResult))
		return false;
	UPulseSystemLibrary::MapAddOrUpdateValue(*CachedProgression, TObjectPtr<UClass>(Type), TObjectPtr<UObject>(OutResult));
	return true;
}

bool UPulseSaveManager::WriteAsSavedValue(UObject* Value)
{
	if (!Value)
		return false;
	auto Cache = &SavedProgression->ProgressionCachedData;
	auto Progression = &SavedProgression->ProgressionSaveData;
	TArray<uint8> _data;
	if (!UPulseSystemLibrary::SerializeObjectToBytes(Value, _data))
		return false;
	if (Progression->IsEmpty())
	{
		Progression->Add(Value->GetClass()->GetFName(), FSaveDataPack(_data));
		UPulseSystemLibrary::MapAddOrUpdateValue(*Cache, TObjectPtr<UClass>(Value->GetClass()), TObjectPtr<UObject>(Value));
		return true;
	}
	else
	{
		if (!Progression->Contains(Value->GetClass()->GetFName()))
			Progression->Add(Value->GetClass()->GetFName(), FSaveDataPack(_data));
		else
			(*Progression)[Value->GetClass()->GetFName()] = FSaveDataPack(_data);
		UPulseSystemLibrary::MapAddOrUpdateValue(*Cache, TObjectPtr<UClass>(Value->GetClass()), TObjectPtr<UObject>(Value));
		return true;
	}
}


void UPulseSaveManager::SaveGame(const int32 SaveIndex, const TArray<TSubclassOf<UGameSaveProvider>>& ExceptionList, bool bAutoSave)
{
	FUserProfile profile;
	if (!GetCurrentUserProfile(profile))
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Saving Operation: Cannot Save game: Invalid Current User Profile"));
		return;
	}

	TArray<TSubclassOf<UGameSaveProvider>> effectiveOnes;

	// Count operation amount 
	for (const auto& providerPair : ProvidersMap)
	{
		if (ExceptionList.Contains(providerPair.Key))
			continue;
		if (!providerPair.Value)
			continue;
		effectiveOnes.Add(providerPair.Key);
	}

	auto SaveGameInstance = SavedProgression;
	if (!SaveGameInstance)
		SaveGameInstance = Cast<UPulseSaveData>(UGameplayStatics::CreateSaveGameObject(UPulseSaveData::StaticClass()));
	if (SaveGameInstance)
	{
		// Set as the Current user progression before calling pre-save
		SavedProgression = SaveGameInstance;
		UE_LOG(LogPulseSave, Log, TEXT("Saving Operation: Saving Started"));
		Save_Internal(profile, SaveGameInstance, FDateTime::Now(), SaveIndex, effectiveOnes, bAutoSave);
	}
}

void UPulseSaveManager::LoadGame(TSubclassOf<UGameSaveProvider> Provider, const FSaveMetaData& Meta)
{
	FUserProfile user;
	if (!GetCurrentUserProfile(user))
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Loading Operation: Cannot load game: Invalid Current User Profile"));
		return;
	}
	if (user.LocalID != Meta.UserLocalID)
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Loading Operation: Cannot load game: The user (%s) cannot load save from user (%s)"), *user.LocalID, *Meta.UserLocalID);
		return;
	}
	LoadGame_Internal(user, Meta, Provider);
}

void UPulseSaveManager::LoadMetas(const FUserProfile& User, TSubclassOf<UGameSaveProvider> Provider)
{
	LoadMetas_Internal(User, Provider);
}

void UPulseSaveManager::DeleteSave(TSubclassOf<UGameSaveProvider> Provider, const FSaveMetaData& Meta)
{
	FUserProfile user;
	if (!GetCurrentUserProfile(user))
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Deleting Operation: Cannot Delete game: Invalid Current User Profile"));
		return;
	}
	if (user.LocalID != Meta.UserLocalID)
	{
		UE_LOG(LogPulseSave, Warning, TEXT("Deleting Operation: Cannot Delete game: The user (%s) cannot delete save from user (%s)"), *user.LocalID, *Meta.UserLocalID);
		return;
	}
	DeleteGame_Internal(user, Meta, Provider);
}

UPulseSaveManager* UPulseSaveManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;
	const auto gi = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!gi)
		return nullptr;
	return gi->GetSubsystem<UPulseSaveManager>();
}
