// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/OperationModifierSubModule/OperationModifierSubModule.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Core/PulseCoreModule.h"
#include "Core/PulseSystemLibrary.h"
#include "Core/PulseResourceManagement/Types/IIdentifiableActor.h"
#include "Core/SaveGameSubModule/PulseSaveManager.h"
#include "GameFramework/Character.h"


float FBuffOperationModifier::GetModifierDiff(const float Input, FGameplayTag InContext) const
{
	// Check context
	if (!Context.GetTagName().ToString().Contains(InContext.GetTagName().ToString()))
		return 0.0f;
	//Get working input value
	const float actualValue = FMath::Abs(bValueAsPercentage ? Input * Value * 0.01 : Value);
	switch (Operator)
	{
	case ENumericOperator::Set:
		return actualValue - Input;
	case ENumericOperator::Add:
		return actualValue;
	case ENumericOperator::Sub:
		return -actualValue;
	case ENumericOperator::Mul:
		return (Input * actualValue) - Input;
	case ENumericOperator::Div:
		return (Input / actualValue) - Input;
	case ENumericOperator::Mod:
		return (FMath::Fmod(Input, actualValue)) - Input;
	}
	return 0.0f;
}

bool FBuffOperationModifier::IsValidModifier() const
{
	auto str = UID;
	str.RemoveSpacesInline();
	return !str.IsEmpty();
}

float FBuffOperationPack::GetPackDiff(const float Input, FGameplayTag Context, TMap<EBuffStackingType, float>* PerStackingDiff) const
{
	TMap<EBuffStackingType, TArray<float>> pSTDiffs;
	TMap<EBuffStackingType, float> pSTResults;
	for (const auto& elem : Modifiers)
	{
		if (pSTDiffs.Contains(elem.StackingType))
			pSTDiffs[elem.StackingType].Add(elem.GetModifierDiff(Input, Context));
		else
			pSTDiffs.Add(elem.StackingType, {elem.GetModifierDiff(Input, Context)});
	}
	float combinedDiffs = 0.0f;
	for (const auto& elem : pSTDiffs)
	{
		switch (elem.Key)
		{
		case EBuffStackingType::None:
			pSTResults.Add(elem.Key, UPulseSystemLibrary::MaxInCollection(elem.Value));
			break;
		case EBuffStackingType::AdditiveValues:
			pSTResults.Add(elem.Key, UPulseSystemLibrary::SumCollection(elem.Value));
			break;
		case EBuffStackingType::AverageValues:
			pSTResults.Add(elem.Key, UPulseSystemLibrary::AverageCollection(elem.Value));
			break;
		}
		if (!pSTResults.Contains(elem.Key))
			continue;
		if (PerStackingDiff != nullptr)
		{
			UPulseSystemLibrary::AddOrReplace(*PerStackingDiff, elem.Key, pSTResults[elem.Key]);
		}
		combinedDiffs += pSTResults[elem.Key];
	}
	return combinedDiffs;
}

bool FBuffOperationPack::UpdateModifierPack(const float DeltaTime, TArray<FBuffOperationModifier>& ExpiredModifiers)
{
	for (int i = Modifiers.Num() - 1; i >= 0; i--)
	{
		// Unlimited modifiers
		if (Modifiers[i].Duration < 0.0f)
			continue;
		// Limited ones
		if (Modifiers[i].LifeTimeLeft > 0)
		{
			Modifiers[i].LifeTimeLeft -= DeltaTime;
			if (Modifiers[i].LifeTimeLeft <= 0.0f)
			{
				Modifiers[i].LifeTimeLeft = 0.0f;
				ExpiredModifiers.Add(Modifiers[i]);
				//Modifiers.RemoveAt(i);
			}
		}
	}
	if (Modifiers.Num() <= 0)
		return false;
	return true;
}

bool FBuffOperationPack::AddModifier(FBuffOperationModifier Modifier)
{
	if (Modifier.UID.IsEmpty())
		return false;
	auto uid = Modifier.UID;
	uid.RemoveSpacesInline();
	if (uid.IsEmpty())
		return false;
	if (Modifiers.IndexOfByPredicate([uid](const FBuffOperationModifier& mod) { return mod.UID == uid; }) != INDEX_NONE)
		return false;
	Modifier.UID = uid;
	Modifier.LifeTimeLeft = Modifier.Duration;
	Modifiers.Add(Modifier);
	return true;
}

bool FBuffOperationPack::RemoveModifier(const FString& ModifierUID, FBuffOperationModifier& OutModifier)
{
	if (ModifierUID.IsEmpty())
		return false;
	auto index = Modifiers.IndexOfByPredicate([ModifierUID](const FBuffOperationModifier& mod)-> bool { return mod.UID == ModifierUID; });
	if (!Modifiers.IsValidIndex(index))
		return false;
	OutModifier = Modifiers[index];
	Modifiers.RemoveAt(index);
	return true;
}


void UBuffModifierHandler::OnBuffChanged_Internal(FName Category, FBuffOperationModifier ModifierBuff, bool hadBeenRemoved)
{
	if (!IsValid())
		return;
	if (_category != Category)
		return;
	if (ModifierBuff.UID != _uid)
	{
		FBuffOperationModifier result;
		if (!UOperationModifierSubModule::ModifierDirectAccess(this, _type, _category, _index, result))
			if (!UOperationModifierSubModule::TryGetModifierRef(this, _uid, result, _type, _category, _index))
				Execute_ReturnToPool(this);
			else if (result.UID != _uid)
				Execute_ReturnToPool(this);
		return;
	}
	if (!hadBeenRemoved)
		return;
	Execute_ReturnToPool(this);
}

void UBuffModifierHandler::OnBuffChangedActor_Internal(TWeakObjectPtr<AActor> Actor, FName Name, FBuffOperationModifier BuffOperationModifier, bool hadBeenRemoved)
{
	if (!IsValid())
		return;
	if (_type != EBuffCategoryType::Actor)
		return;
	if (BuffOperationModifier.UID != _uid)
	{
		FBuffOperationModifier result;
		if (!UOperationModifierSubModule::ModifierDirectAccess(this, _type, _category, _index, result))
			if (!UOperationModifierSubModule::TryGetModifierRef(this, _uid, result, _type, _category, _index))
				Execute_ReturnToPool(this);
			else if (result.UID != _uid)
				Execute_ReturnToPool(this);
		return;
	}
	if (!hadBeenRemoved)
		return;
	Execute_ReturnToPool(this);
}

bool UBuffModifierHandler::OnPoolingObjectSpawned_Implementation(const FPoolingParams SpawnData)
{
	auto spd = SpawnData;
	if (const auto uidPtr = UPulseSystemLibrary::TryGetItemAt(spd.NamesParams, 0))
	{
		FString uid = uidPtr->ToString();
		FBuffOperationModifier modifier;
		if (UOperationModifierSubModule::TryGetModifierRef(this, uid, modifier, _type, _category, _index))
		{
			_uid = uid;
			return true;
		}
	}
	return false;
}

void UBuffModifierHandler::OnPoolingObjectDespawned_Implementation()
{
	OnHandleReleased.Broadcast();
	_uid.Empty();
	_category = NAME_None;
	_index = INDEX_NONE;
	_type = EBuffCategoryType::Actor;
	UOperationModifierSubModule::UnbindHandle(this, this);
	IIPoolingObject::OnPoolingObjectDespawned_Implementation();
}

FBuffOperationModifier UBuffModifierHandler::GetModifier() const
{
	FBuffOperationModifier result;
	if (IsValid() && UOperationModifierSubModule::ModifierDirectAccess(this, _type, _category, _index, result))
		return result;
	return FBuffOperationModifier();
}

bool UBuffModifierHandler::IsValid() const
{
	return !_uid.IsEmpty() && !_category.IsNone() && _index != INDEX_NONE && _bIsBound;
}

void UBuffModifierHandler::BindManager(UOperationModifierSubModule* Mgr)
{
	if (_bIsBound)
		return;
	if (!Mgr)
		return;
	Mgr->OnActorBuffModifierChanged.AddDynamic(this, &UBuffModifierHandler::OnBuffChangedActor_Internal);
	Mgr->OnUserBuffModifierChanged.AddDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	Mgr->OnPlayerBuffModifierChanged.AddDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	Mgr->OnGlobalBuffModifierChanged.AddDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	Mgr->OnNpcBuffModifierChanged.AddDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	_bIsBound = true;
}

void UBuffModifierHandler::UnbindManager(UOperationModifierSubModule* Mgr)
{
	if (!_bIsBound)
		return;
	if (!Mgr)
		return;
	Mgr->OnActorBuffModifierChanged.RemoveDynamic(this, &UBuffModifierHandler::OnBuffChangedActor_Internal);
	Mgr->OnUserBuffModifierChanged.RemoveDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	Mgr->OnPlayerBuffModifierChanged.RemoveDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	Mgr->OnGlobalBuffModifierChanged.RemoveDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	Mgr->OnNpcBuffModifierChanged.RemoveDynamic(this, &UBuffModifierHandler::OnBuffChanged_Internal);
	_bIsBound = false;
}


void UOperationModifierSubModule::UpdatePack(TMap<FName, FBuffOperationPack>& TargetPack, const float DeltaTime, EBuffCategoryType Type)
{
	for (auto& elem : TargetPack)
	{
		TArray<FBuffOperationModifier> expiredOnes;
		if (!elem.Value.UpdateModifierPack(DeltaTime, expiredOnes))
			continue;
		for (const auto& buff : expiredOnes)
		{
			FString uid = buff.UID;
			switch (Type)
			{
			case EBuffCategoryType::Actor:
				for (const auto& link : _actorLinks)
				{
					if (link.Value.Modifiers.IndexOfByPredicate([buff](const FBuffOperationModifier& bff)-> bool { return buff.UID == bff.UID; }) == INDEX_NONE)
						continue;
					auto actorPtr = link.Key.Get();
					_actorRemoveCmd.Enqueue(FBuffRemoveOperationCommand(uid, Type, actorPtr));
				}
				break;
			case EBuffCategoryType::PlayerCharacter:
				_playerCharacterRemoveCmd.Enqueue(FBuffRemoveOperationCommand(uid, Type));
				break;
			case EBuffCategoryType::PlayerController:
				_playerSessionRemoveCmd.Enqueue(FBuffRemoveOperationCommand(uid, Type));
				break;
			case EBuffCategoryType::NPCCharacter:
				_npcCharacterRemoveCmd.Enqueue(FBuffRemoveOperationCommand(uid, Type));
				break;
			case EBuffCategoryType::User:
				_playerUserRemoveCmd.Enqueue(FBuffRemoveOperationCommand(uid, Type));
				break;
			}
		}
	}
}

void UOperationModifierSubModule::HandleEmpties(TMap<FName, FBuffOperationPack>& TargetPack)
{
	TArray<FName> empty;
	for (auto& elem : TargetPack)
	{
		if (elem.Value.Modifiers.IsEmpty())
			empty.Add(elem.Key);
	}
	for (const auto& key : empty)
		TargetPack.Remove(key);
}

void UOperationModifierSubModule::ExecuteAddCommands(TMap<FName, FBuffOperationPack>& TargetPack, TQueue<FBuffAddOperationCommand>& SourceQueue)
{
	FBuffAddOperationCommand addCmd;
	int64 seed = 0;
	while (SourceQueue.Dequeue(addCmd))
	{
		seed++;
		FString str = addCmd.Category.ToString();
		str.RemoveSpacesInline();
		if (str.IsEmpty())
			continue;
		if (addCmd.catType == EBuffCategoryType::Actor && !addCmd.Actor.IsValid())
			continue;
		addCmd.Category = FName(*str);
		// Make The UID
		auto guid = FGuid::NewGuid();
		addCmd.Modifier.UID = guid.ToString();
		//
		if (TargetPack.Contains(addCmd.Category))
		{
			if (!TargetPack[addCmd.Category].AddModifier(addCmd.Modifier))
				continue;
		}
		else
		{
			FBuffOperationPack newPack;
			if (!newPack.AddModifier(addCmd.Modifier))
				continue;
			TargetPack.Add(addCmd.Category, newPack);
		}
		FBuffOperationPack* packPtr = &TargetPack[addCmd.Category];
		switch (addCmd.catType)
		{
		case EBuffCategoryType::Actor:
			if (addCmd.Actor.IsValid() && packPtr)
			{
				addCmd.Actor.Get()->OnDestroyed.AddDynamic(this, &UOperationModifierSubModule::OnActorDestroyed_Internal);
				_actorLinks.Add(addCmd.Actor.Get(), *packPtr);
				//Launch Gameplay Effect on actor
				if (_autoApplyGameplayEffects)
					AddGameplayEffect(addCmd.Actor.Get(), addCmd.Modifier);
				OnActorBuffModifierChanged.Broadcast(addCmd.Actor, addCmd.Category, addCmd.Modifier, false);
			}
			break;
		case EBuffCategoryType::PlayerCharacter:
			{
				//Launch Gameplay Effect on pawn
				if (_autoApplyGameplayEffects)
					if (auto plCtrl = UGameplayStatics::GetPlayerController(this, addCmd.Modifier.PlayerIndex))
						if (auto plChar = plCtrl->GetPawn())
							AddGameplayEffect(plChar, addCmd.Modifier);
				OnPlayerBuffModifierChanged.Broadcast(addCmd.Category, addCmd.Modifier, false);
			}
			break;
		case EBuffCategoryType::PlayerController:
			{
				//Launch Gameplay Effect on Controller
				if (_autoApplyGameplayEffects)
					if (auto plCtrl = UGameplayStatics::GetPlayerController(this, addCmd.Modifier.PlayerIndex))
						AddGameplayEffect(plCtrl, addCmd.Modifier);
				OnGlobalBuffModifierChanged.Broadcast(addCmd.Category, addCmd.Modifier, false);
			}
			break;
		case EBuffCategoryType::NPCCharacter:
			{
				//Launch Gameplay Effect on pawn
				if (_autoApplyGameplayEffects)
				{
					TArray<AActor*> npcActors;
					UGameplayStatics::GetAllActorsWithInterface(this, UIIdentifiableActor::StaticClass(), npcActors);
					for (auto actor : npcActors)
						if (IIIdentifiableActor::Execute_GetID(actor) == addCmd.Modifier.CharacterID)
							AddGameplayEffect(actor, addCmd.Modifier);
				}
				OnNpcBuffModifierChanged.Broadcast(addCmd.Category, addCmd.Modifier, false);
			}
			break;
		case EBuffCategoryType::User:
			{
				OnUserBuffModifierChanged.Broadcast(addCmd.Category, addCmd.Modifier, false);
			}
			break;
		}
	}
}

void UOperationModifierSubModule::ExecuteRemoveCommands(TMap<FName, FBuffOperationPack>& TargetPack, TQueue<FBuffRemoveOperationCommand>& SourceQueue)
{
	FBuffRemoveOperationCommand remCmd;
	while (SourceQueue.Dequeue(remCmd))
	{
		FBuffOperationModifier mod;
		bool found = false;
		for (const auto& pair : TargetPack)
		{
			if (TargetPack[pair.Key].RemoveModifier(remCmd.UID, mod))
			{
				found = true;
				remCmd.Category = pair.Key;
				remCmd.Modifier = mod;
				break;
			}
		}
		if (!found)
			continue;
		if (remCmd.CatType == EBuffCategoryType::Actor && _actorLinks.Contains(remCmd.Actor))
		{
			_actorLinks.Remove(remCmd.Actor);
			if (remCmd.Actor.IsValid())
				remCmd.Actor->OnDestroyed.RemoveDynamic(this, &UOperationModifierSubModule::OnActorDestroyed_Internal);
		}
		_removeCmdBroadcastQueue.Enqueue(remCmd);
	}
}

void UOperationModifierSubModule::BroadcastRemoveCommands()
{
	FBuffRemoveOperationCommand remCmd;
	while (_removeCmdBroadcastQueue.Dequeue(remCmd))
	{
		switch (remCmd.CatType)
		{
		case EBuffCategoryType::Actor:
			OnActorBuffModifierChanged.Broadcast(remCmd.Actor, remCmd.Category, remCmd.Modifier, true);
			break;
		case EBuffCategoryType::PlayerCharacter:
			OnPlayerBuffModifierChanged.Broadcast(remCmd.Category, remCmd.Modifier, true);
			break;
		case EBuffCategoryType::PlayerController:
			OnGlobalBuffModifierChanged.Broadcast(remCmd.Category, remCmd.Modifier, true);
			break;
		case EBuffCategoryType::NPCCharacter:
			OnNpcBuffModifierChanged.Broadcast(remCmd.Category, remCmd.Modifier, true);
			break;
		case EBuffCategoryType::User:
			OnUserBuffModifierChanged.Broadcast(remCmd.Category, remCmd.Modifier, true);
			break;
		}
		if (_modifierGameplayEffectHandles.Contains(remCmd.UID))
		{
			RemoveGameplayEffect(_modifierGameplayEffectHandles[remCmd.UID]);
			_modifierGameplayEffectHandles.Remove(remCmd.UID);
		}
	}
}

bool UOperationModifierSubModule::FindBuff(TMap<FName, FBuffOperationPack>& TargetPack, const FString& UID, FName& OutCategory, int32& OutIndex, TWeakObjectPtr<AActor>& OutActor)
{
	for (const auto& pair : TargetPack)
	{
		for (int i = 0; i < pair.Value.Modifiers.Num(); i++)
			if (pair.Value.Modifiers[i].UID == UID)
			{
				OutCategory = pair.Key;
				OutIndex = i;
				// Look for the actor
				for (const auto& actorPair : _actorLinks)
				{
					for (const auto& aBuff : actorPair.Value.Modifiers)
					{
						if (aBuff.UID == UID)
						{
							OutActor = actorPair.Key;
							break;
						}
					}
				}
				return true;
			}
	}
	return false;
}

void UOperationModifierSubModule::OnActorDestroyed_Internal(AActor* DestroyedActor)
{
	if (!DestroyedActor)
		return;
	if (!_actorLinks.Contains(DestroyedActor))
		return;
	for (const auto& mod : _actorLinks[DestroyedActor].Modifiers)
	{
		FString uid = mod.UID;
		_actorRemoveCmd.Enqueue(FBuffRemoveOperationCommand(uid, EBuffCategoryType::Actor, DestroyedActor));
	}
	_actorLinks.Remove(DestroyedActor);
	DestroyedActor->OnDestroyed.RemoveDynamic(this, &UOperationModifierSubModule::OnActorDestroyed_Internal);
}

void UOperationModifierSubModule::OnSaveBuffs_Internal(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave)
{
	auto coreModule = Cast<UPulseCoreModule>(_OwningModule);
	if (!coreModule)
		return;
	auto saveBuffs = NewObject<USaveBuffOperationManagerData>(this);
	saveBuffs->PlayerUserModifiers = _playerUserModifiers;
	saveBuffs->PlayerSessionModifiers = _playerSessionModifiers;
	saveBuffs->PlayerCharacterModifiers = _playerCharacterModifiers;
	saveBuffs->NpcCharacterModifiers = _npcCharacterModifiers;
	coreModule->GetSaveManager()->WriteAsSavedValue(saveBuffs, UserIndex);
}

void UOperationModifierSubModule::OnLoadGameBuffs_Internal(ELoadSaveResponse Response, int32 UserIndex, UPulseSaveData* LoadedSaveData)
{
	if (Response != ELoadSaveResponse::Success)
		return;
	if (LoadedSaveData == nullptr)
		return;
	auto coreModule = Cast<UPulseCoreModule>(_OwningModule);
	if (!coreModule)
		return;
	UObject* outObj = nullptr;
	if (!coreModule->GetSaveManager()->ReadSavedValue(USaveBuffOperationManagerData::StaticClass(), outObj, UserIndex))
		return;
	if (auto saveBuffs = Cast<USaveBuffOperationManagerData>(outObj))
	{
		// Notify removals
		for (const auto item : _playerUserModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, true);
		for (const auto item : _playerSessionModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, true);
		for (const auto item : _playerCharacterModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, true);
		for (const auto item : _npcCharacterModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, true);
		// Dispose of all handlers
		for (auto hdl : _modifierHandlers)
			if (hdl.Value.IsValid())
				hdl.Value->Execute_ReturnToPool(hdl.Value.Get());
		_modifierHandlers.Empty();
		// Set values
		_playerUserModifiers = saveBuffs->PlayerUserModifiers;
		_playerSessionModifiers = saveBuffs->PlayerSessionModifiers;
		_playerCharacterModifiers = saveBuffs->PlayerCharacterModifiers;
		_npcCharacterModifiers = saveBuffs->NpcCharacterModifiers;
		// Notify adds
		for (const auto item : _playerUserModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, false);
		for (const auto item : _playerSessionModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, false);
		for (const auto item : _playerCharacterModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, false);
		for (const auto item : _npcCharacterModifiers)
			for (const auto mod : item.Value.Modifiers)
				OnUserBuffModifierChanged.Broadcast(item.Key, mod, false);
	}
}

UOperationModifierSubModule* UOperationModifierSubModule::Get(const UObject* WorldContext)
{
	const auto gameInstance = UGameplayStatics::GetGameInstance(WorldContext);
	if (!gameInstance)
		return nullptr;
	auto coreModule = gameInstance->GetSubsystem<UPulseCoreModule>();
	if (!coreModule)
		return nullptr;
	return coreModule->GetBuffManager();
}

TQueue<FBuffAddOperationCommand>* UOperationModifierSubModule::GetAddQueue(const FBuffOperationModifier& Modifier, const FName& Category, FBuffAddOperationCommand& OutCommand,
                                                                           AActor* Actor)
{
	if (!Modifier.UID.IsEmpty())
		return nullptr;
	OutCommand.CharacterID = Modifier.CharacterID;
	OutCommand.Modifier = Modifier;
	OutCommand.Actor = Actor;
	OutCommand.Category = Category;
	if (Modifier.CharacterID.IsValid() && Modifier.PlayerIndex >= 0)
	{
		OutCommand.catType = EBuffCategoryType::PlayerCharacter;
		return &_playerCharacterAddCmd;
	}
	if (Modifier.UserID.IsValid() && !Modifier.UserID.IsNone())
	{
		OutCommand.catType = EBuffCategoryType::User;
		return &_playerUserAddCmd;
	}
	if (Modifier.CharacterID.IsValid())
	{
		OutCommand.catType = EBuffCategoryType::NPCCharacter;
		return &_npcCharacterAddCmd;
	}
	if (Modifier.PlayerIndex >= 0)
	{
		OutCommand.catType = EBuffCategoryType::PlayerController;
		return &_playerSessionAddCmd;
	}
	if (Actor != nullptr)
	{
		OutCommand.catType = EBuffCategoryType::Actor;
		return &_actorAddCmd;
	}
	return nullptr;
}

TQueue<FBuffRemoveOperationCommand>* UOperationModifierSubModule::GetRemoveQueue(const FString& ModifierUID, FBuffRemoveOperationCommand& OutCommand)
{
	if (ModifierUID.IsEmpty())
		return nullptr;
	int32 index = INDEX_NONE;
	FName category;
	TWeakObjectPtr<AActor> actor = nullptr;
	OutCommand.UID = ModifierUID;
	if (FindBuff(_actorModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::Actor;
		return &_actorRemoveCmd;
	}
	if (FindBuff(_npcCharacterModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::NPCCharacter;
		return &_npcCharacterRemoveCmd;
	}
	if (FindBuff(_playerCharacterModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::PlayerCharacter;
		return &_playerCharacterRemoveCmd;
	}
	if (FindBuff(_playerSessionModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::PlayerController;
		return &_playerSessionRemoveCmd;
	}
	if (FindBuff(_playerUserModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::User;
		return &_playerUserRemoveCmd;
	}
	return nullptr;
}

bool UOperationModifierSubModule::AddGameplayEffect(UObject* AbilitySystemTarget, FBuffOperationModifier Buff)
{
	if (!_autoApplyGameplayEffects)
		return false;
	// if (addCmd.Modifier.GameplayAbilityEffect && addCmd.Actor->Implements<UAbilitySystemInterface>())
	// {
	// if (UAbilitySystemComponent* abilityComp = Cast<IAbilitySystemInterface>(addCmd.Actor)->GetAbilitySystemComponent())
	// {
	// 	FGameplayEffectSpec spc;
	// 	auto hdl = abilityComp->ApplyGameplayEffectSpecToSelf()
	// }
	// }
	//_modifierGameplayEffectHandles.Add()
	return false;
}

bool UOperationModifierSubModule::RemoveGameplayEffect(FGameplayEffectSpecHandle OutSpec)
{
	if (!_autoApplyGameplayEffects)
		return false;
	return false;
}


FName UOperationModifierSubModule::GetSubModuleName() const
{
	return Super::GetSubModuleName();
}

bool UOperationModifierSubModule::WantToTick() const
{
	return true;
}

bool UOperationModifierSubModule::TickWhenPaused() const
{
	return false;
}

void UOperationModifierSubModule::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	auto coreModule = Cast<UPulseCoreModule>(OwningModule);
	if (!coreModule)
		return;
	bool bindSaveMgr = false;
	if (auto settings = coreModule->GetProjectConfig())
	{
		bindSaveMgr = settings->bSaveAndLoadActiveBuffsAutomatically;
		_autoApplyGameplayEffects = settings->bAutoApplyGameplayEffects;
		UPulsePoolingManager::SetPoolLimitPerClass(this, UBuffModifierHandler::StaticClass(), settings->BuffHandlePoolingLimit);
	}
	if (bindSaveMgr)
	{
		if (auto saveMgr = coreModule->GetSaveManager())
		{
			saveMgr->OnGameAboutToSave.AddDynamic(this, &UOperationModifierSubModule::OnSaveBuffs_Internal);
			saveMgr->OnGameLoaded.AddDynamic(this, &UOperationModifierSubModule::OnLoadGameBuffs_Internal);
		}
	}
}

void UOperationModifierSubModule::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
}

void UOperationModifierSubModule::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
	// Handle Updates
	UpdatePack(_actorModifiers, DeltaTime, EBuffCategoryType::Actor);
	UpdatePack(_npcCharacterModifiers, DeltaTime, EBuffCategoryType::NPCCharacter);
	UpdatePack(_playerCharacterModifiers, DeltaTime, EBuffCategoryType::PlayerCharacter);
	UpdatePack(_playerSessionModifiers, DeltaTime, EBuffCategoryType::PlayerController);
	UpdatePack(_playerUserModifiers, DeltaTime, EBuffCategoryType::User);
	// Handle removes
	ExecuteRemoveCommands(_actorModifiers, _actorRemoveCmd);
	ExecuteRemoveCommands(_npcCharacterModifiers, _npcCharacterRemoveCmd);
	ExecuteRemoveCommands(_playerCharacterModifiers, _playerCharacterRemoveCmd);
	ExecuteRemoveCommands(_playerSessionModifiers, _playerSessionRemoveCmd);
	ExecuteRemoveCommands(_playerUserModifiers, _playerUserRemoveCmd);
	//Handle Empty categories
	HandleEmpties(_actorModifiers);
	HandleEmpties(_npcCharacterModifiers);
	HandleEmpties(_playerCharacterModifiers);
	HandleEmpties(_playerSessionModifiers);
	HandleEmpties(_playerUserModifiers);
	// Broadcast removes
	BroadcastRemoveCommands();
	// Handle Adds
	ExecuteAddCommands(_actorModifiers, _actorAddCmd);
	ExecuteAddCommands(_npcCharacterModifiers, _npcCharacterAddCmd);
	ExecuteAddCommands(_playerCharacterModifiers, _playerCharacterAddCmd);
	ExecuteAddCommands(_playerSessionModifiers, _playerSessionAddCmd);
	ExecuteAddCommands(_playerUserModifiers, _playerUserAddCmd);
}


float UOperationModifierSubModule::ApplyAllModifiers(const UObject* WorldContext, const float Input, FGameplayTag Context)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return 0.0f;
	TMap<FName, FBuffOperationPack> modifiers;
	GetAllModifiers(WorldContext, Context, modifiers);
	float result = 0.0f;
	for (const auto& pair : modifiers)
		result += pair.Value.GetPackDiff(Input, Context);
	return Input + result;
}

float UOperationModifierSubModule::ApplyCharacterModifiers(const UObject* WorldContext, const float Input, FPrimaryAssetId CharacterID, FGameplayTag Context)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return 0.0f;
	TMap<FName, FBuffOperationPack> modifiers;
	GetNPCCharacterModifiers(WorldContext, Context, CharacterID, modifiers);
	GetGlobalModifiers(WorldContext, Context, modifiers);
	GetUserModifiers(WorldContext, Context, modifiers);
	float result = 0.0f;
	for (const auto& pair : modifiers)
		result += pair.Value.GetPackDiff(Input, Context);
	return Input + result;
}

float UOperationModifierSubModule::ApplyPlayerCharacterModifiers(const UObject* WorldContext, const float Input, FPrimaryAssetId CharacterID, FGameplayTag Context)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return 0.0f;
	TMap<FName, FBuffOperationPack> modifiers;
	GetNPCCharacterModifiers(WorldContext, Context, CharacterID, modifiers);
	GetPlayerCharacterModifiers(WorldContext, Context, CharacterID, modifiers);
	GetGlobalModifiers(WorldContext, Context, modifiers);
	GetUserModifiers(WorldContext, Context, modifiers);
	float result = 0.0f;
	for (const auto& pair : modifiers)
		result += pair.Value.GetPackDiff(Input, Context);
	return Input + result;
}

float UOperationModifierSubModule::ApplyActorModifiers(const UObject* WorldContext, const float Input, AActor* Actor, FGameplayTag Context, int32 PlayerIndex)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return 0.0f;
	TMap<FName, FBuffOperationPack> modifiers;
	bool isPlayerCharacter = Cast<AActor>(UGameplayStatics::GetPlayerCharacter(WorldContext, PlayerIndex)) == Actor;
	FPrimaryAssetId CharacterID = (Actor && Actor->Implements<UIIdentifiableActor>()) ? IIIdentifiableActor::Execute_GetID(Actor) : FPrimaryAssetId();
	GetGlobalModifiers(WorldContext, Context, modifiers);
	GetUserModifiers(WorldContext, Context, modifiers);
	if (CharacterID.IsValid())
		GetNPCCharacterModifiers(WorldContext, Context, CharacterID, modifiers);
	if (isPlayerCharacter && CharacterID.IsValid())
		GetPlayerCharacterModifiers(WorldContext, Context, CharacterID, modifiers);
	float result = 0.0f;
	for (const auto& pair : modifiers)
		result += pair.Value.GetPackDiff(Input, Context);
	return Input + result;
}

void UOperationModifierSubModule::GetAll(const UObject* WorldContext, TMap<FName, FBuffOperationPack>& OutGlobalCategoryModifiers,
                                         TMap<FName, FBuffOperationPack>& OutUserCategoryModifiers, TMap<FName, FBuffOperationPack>& OutPlayerCategoryModifiers,
                                         TMap<FName, FBuffOperationPack>& OutNpcCategoryModifiers, TMap<FName, FBuffOperationPack>& OutActorCategoryModifiers)
{
	OutUserCategoryModifiers.Empty();
	OutGlobalCategoryModifiers.Empty();
	OutPlayerCategoryModifiers.Empty();
	OutNpcCategoryModifiers.Empty();
	OutActorCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	TArray<TPair<FName, FBuffOperationPack>> AllModifiers;
	for (const auto& elem : mgr->_playerUserModifiers)
		OutUserCategoryModifiers.Add(elem);
	for (const auto& elem : mgr->_playerSessionModifiers)
		OutGlobalCategoryModifiers.Add(elem);
	for (const auto& elem : mgr->_playerCharacterModifiers)
		OutPlayerCategoryModifiers.Add(elem);
	for (const auto& elem : mgr->_npcCharacterModifiers)
		OutNpcCategoryModifiers.Add(elem);
	for (const auto& elem : mgr->_actorModifiers)
		OutActorCategoryModifiers.Add(elem);
}

void UOperationModifierSubModule::GetAllModifiers(const UObject* WorldContext, FGameplayTag Context, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers)
{
	OutPerCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	TArray<TPair<FName, FBuffOperationPack>> AllModifiers;
	for (const auto& elem : mgr->_playerUserModifiers)
		AllModifiers.Add(elem);
	for (const auto& elem : mgr->_playerSessionModifiers)
		AllModifiers.Add(elem);
	for (const auto& elem : mgr->_playerCharacterModifiers)
		AllModifiers.Add(elem);
	for (const auto& elem : mgr->_npcCharacterModifiers)
		AllModifiers.Add(elem);
	for (const auto& elem : mgr->_actorModifiers)
		AllModifiers.Add(elem);
	for (const auto& elem : AllModifiers)
	{
		FBuffOperationPack temp;
		for (const auto buff : elem.Value.Modifiers)
		{
			if (buff.Context.GetTagName().ToString().Contains(Context.GetTagName().ToString()))
				temp.AddModifier(buff);
		}
		if (!temp.Modifiers.IsEmpty())
		{
			if (OutPerCategoryModifiers.Contains(elem.Key))
			{
				for (const auto& item : temp.Modifiers)
					OutPerCategoryModifiers[elem.Key].AddModifier(item);
			}
			else
			{
				OutPerCategoryModifiers.Add(elem.Key, temp);
			}
		}
	}
}

void UOperationModifierSubModule::GetUserModifiers(const UObject* WorldContext, FGameplayTag Context, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers)
{
	OutPerCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	for (const auto& elem : mgr->_playerUserModifiers)
	{
		FBuffOperationPack temp;
		for (const auto& buff : elem.Value.Modifiers)
		{
			if (buff.Context.GetTagName().ToString().Contains(Context.GetTagName().ToString()))
				temp.AddModifier(buff);
		}
		if (!temp.Modifiers.IsEmpty())
		{
			if (OutPerCategoryModifiers.Contains(elem.Key))
			{
				for (const auto& item : temp.Modifiers)
					OutPerCategoryModifiers[elem.Key].AddModifier(item);
			}
			else
			{
				OutPerCategoryModifiers.Add(elem.Key, temp);
			}
		}
	}
}

void UOperationModifierSubModule::GetGlobalModifiers(const UObject* WorldContext, FGameplayTag Context, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers)
{
	OutPerCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	for (const auto& elem : mgr->_playerSessionModifiers)
	{
		FBuffOperationPack temp;
		for (const auto& buff : elem.Value.Modifiers)
		{
			if (buff.Context.GetTagName().ToString().Contains(Context.GetTagName().ToString()))
				temp.AddModifier(buff);
		}
		if (!temp.Modifiers.IsEmpty())
		{
			if (OutPerCategoryModifiers.Contains(elem.Key))
			{
				for (const auto& item : temp.Modifiers)
					OutPerCategoryModifiers[elem.Key].AddModifier(item);
			}
			else
			{
				OutPerCategoryModifiers.Add(elem.Key, temp);
			}
		}
	}
}

void UOperationModifierSubModule::GetPlayerCharacterModifiers(const UObject* WorldContext, FGameplayTag Context, FPrimaryAssetId ID,
                                                              TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers)
{
	OutPerCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	for (const auto& elem : mgr->_playerCharacterModifiers)
	{
		FBuffOperationPack temp;
		for (const auto& buff : elem.Value.Modifiers)
		{
			if (buff.Context.GetTagName().ToString().Contains(Context.GetTagName().ToString()) && buff.CharacterID == ID)
				temp.AddModifier(buff);
		}
		if (!temp.Modifiers.IsEmpty())
		{
			if (OutPerCategoryModifiers.Contains(elem.Key))
			{
				for (const auto& item : temp.Modifiers)
					OutPerCategoryModifiers[elem.Key].AddModifier(item);
			}
			else
			{
				OutPerCategoryModifiers.Add(elem.Key, temp);
			}
		}
	}
}

void UOperationModifierSubModule::GetNPCCharacterModifiers(const UObject* WorldContext, FGameplayTag Context, FPrimaryAssetId ID,
                                                           TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers)
{
	OutPerCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	for (const auto& elem : mgr->_npcCharacterModifiers)
	{
		FBuffOperationPack temp;
		for (const auto& buff : elem.Value.Modifiers)
		{
			if (buff.Context.GetTagName().ToString().Contains(Context.GetTagName().ToString()) && buff.CharacterID == ID)
				temp.AddModifier(buff);
		}
		if (!temp.Modifiers.IsEmpty())
		{
			if (OutPerCategoryModifiers.Contains(elem.Key))
			{
				for (const auto& item : temp.Modifiers)
					OutPerCategoryModifiers[elem.Key].AddModifier(item);
			}
			else
			{
				OutPerCategoryModifiers.Add(elem.Key, temp);
			}
		}
	}
}

void UOperationModifierSubModule::GetActorModifiers(const UObject* WorldContext, FGameplayTag Context, AActor* Actor, TMap<FName, FBuffOperationPack>& OutPerCategoryModifiers)
{
	OutPerCategoryModifiers.Empty();
	auto mgr = Get(WorldContext);
	if (!mgr)
		return;
	for (const auto& elem : mgr->_actorLinks)
	{
		if (elem.Key != Actor)
			continue;
		for (const auto& buff : elem.Value.Modifiers)
		{
			for (const auto& corresp : mgr->_actorModifiers)
			{
				for (const auto& bff : corresp.Value.Modifiers)
				{
					FName cat = corresp.Key;
					if (bff.UID == buff.UID && buff.Context.GetTagName().ToString().Contains(Context.GetTagName().ToString()))
					{
						if (OutPerCategoryModifiers.Contains(cat))
						{
							OutPerCategoryModifiers[cat].AddModifier(buff);
						}
						else
						{
							FBuffOperationPack temp;
							temp.AddModifier(buff);
							OutPerCategoryModifiers.Add(cat, temp);
						}
					}
				}
			}
		}
	}
}


bool UOperationModifierSubModule::AddModifier(const UObject* WorldContext, const FBuffOperationModifier& Modifier, const FName& Category)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return false;
	FBuffAddOperationCommand cmd;
	if (auto queue = mgr->GetAddQueue(Modifier, Category, cmd))
	{
		queue->Enqueue(cmd);
		return true;
	}
	return false;
}

bool UOperationModifierSubModule::RemoveModifier(const UObject* WorldContext, const FString& ModifierUID)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return false;
	FBuffRemoveOperationCommand cmd;
	if (auto queue = mgr->GetRemoveQueue(ModifierUID, cmd))
	{
		queue->Enqueue(cmd);
		return true;
	}
	return false;
}

bool UOperationModifierSubModule::VerifyModifier(const UObject* WorldContext, const FString& ModifierUID)
{
	EBuffCategoryType typeBuff = EBuffCategoryType::Actor;
	FName category;
	int32 index = INDEX_NONE;
	FBuffOperationModifier result;
	return TryGetModifierRef(WorldContext, ModifierUID, result, typeBuff, category, index);
}

bool UOperationModifierSubModule::TryGetModifierRef(const UObject* WorldContext, const FString& ModifierUID, FBuffOperationModifier& OutModifier, EBuffCategoryType& OutType,
                                                    FName& OutCategory, int32& OutIndex)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return false;
	for (auto& elem : mgr->_playerUserModifiers)
	{
		const int32 index = elem.Value.Modifiers.IndexOfByPredicate([ModifierUID](const auto& item) { return item.UID == ModifierUID; });
		if (index != INDEX_NONE)
		{
			OutModifier = elem.Value.Modifiers[index];
			OutType = EBuffCategoryType::User;
			OutCategory = elem.Key;
			OutIndex = index;
			return true;
		}
	}
	for (auto& elem : mgr->_playerSessionModifiers)
	{
		const int32 index = elem.Value.Modifiers.IndexOfByPredicate([ModifierUID](const auto& item) { return item.UID == ModifierUID; });
		if (index != INDEX_NONE)
		{
			OutModifier = elem.Value.Modifiers[index];
			OutType = EBuffCategoryType::PlayerController;
			OutCategory = elem.Key;
			OutIndex = index;
			return true;
		}
	}
	for (auto& elem : mgr->_playerCharacterModifiers)
	{
		const int32 index = elem.Value.Modifiers.IndexOfByPredicate([ModifierUID](const auto& item) { return item.UID == ModifierUID; });
		if (index != INDEX_NONE)
		{
			OutModifier = elem.Value.Modifiers[index];
			OutType = EBuffCategoryType::PlayerCharacter;
			OutCategory = elem.Key;
			OutIndex = index;
			return true;
		}
	}
	for (auto& elem : mgr->_npcCharacterModifiers)
	{
		const int32 index = elem.Value.Modifiers.IndexOfByPredicate([ModifierUID](const auto& item) { return item.UID == ModifierUID; });
		if (index != INDEX_NONE)
		{
			OutModifier = elem.Value.Modifiers[index];
			OutType = EBuffCategoryType::NPCCharacter;
			OutCategory = elem.Key;
			OutIndex = index;
			return true;
		}
	}
	for (auto& elem : mgr->_actorModifiers)
	{
		const int32 index = elem.Value.Modifiers.IndexOfByPredicate([ModifierUID](const auto& item) { return item.UID == ModifierUID; });
		if (index != INDEX_NONE)
		{
			OutModifier = elem.Value.Modifiers[index];
			OutType = EBuffCategoryType::Actor;
			OutCategory = elem.Key;
			OutIndex = index;
			return true;
		}
	}
	return false;
}

bool UOperationModifierSubModule::ModifierDirectAccess(const UObject* WorldContext, EBuffCategoryType Type, const FName& Category, int32 Index, FBuffOperationModifier& OutModifier)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return false;
	TMap<FName, FBuffOperationPack>* _packPtr = nullptr;
	switch (Type)
	{
	case EBuffCategoryType::Actor: _packPtr = &mgr->_actorModifiers;
		break;
	case EBuffCategoryType::PlayerCharacter: _packPtr = &mgr->_playerCharacterModifiers;
		break;
	case EBuffCategoryType::PlayerController: _packPtr = &mgr->_playerSessionModifiers;
		break;
	case EBuffCategoryType::NPCCharacter: _packPtr = &mgr->_npcCharacterModifiers;
		break;
	case EBuffCategoryType::User: _packPtr = &mgr->_playerUserModifiers;
		break;
	}
	if (_packPtr == nullptr)
		return false;
	if (!_packPtr->Contains(Category))
		return false;
	if (!(*_packPtr)[Category].Modifiers.IsValidIndex(Index))
		return false;
	OutModifier = (*_packPtr)[Category].Modifiers[Index];
	return true;
}

bool UOperationModifierSubModule::TryGetModifierHandler(UObject* WorldContext, const FString& ModifierUID, UBuffModifierHandler*& OutHandler)
{
	auto mgr = Get(WorldContext);
	if (!mgr)
		return false;
	if (mgr->_modifierHandlers.Contains(ModifierUID))
	{
		OutHandler = mgr->_modifierHandlers[ModifierUID].Get();
		return true;
	}
	FPoolingParams SpawnData;
	SpawnData.NamesParams.Add(FName(*ModifierUID));
	UObject* hdl;
	if (UPulsePoolingManager::GetObjectFromPool(WorldContext, UBuffModifierHandler::StaticClass(), hdl, SpawnData) == EPoolQueryResult::Success)
	{
		UBuffModifierHandler* mod = Cast<UBuffModifierHandler>(hdl);
		if (mod)
		{
			mgr->_modifierHandlers.Add(ModifierUID, mod);
			mod->BindManager(mgr);
			OutHandler = mod;
			return true;
		}
	}
	return false;
}

void UOperationModifierSubModule::UnbindHandle(const UObject* WorldContext, UBuffModifierHandler* Handler)
{
	if (!Handler)
		return;
	auto Mgr = Get(WorldContext);
	if (!Mgr)
		return;
	auto modifier = Handler->GetModifier();
	if (modifier.IsValidModifier())
	{
		Mgr->_modifierHandlers.Remove(modifier.UID);
	}
	Handler->UnbindManager(Mgr);
}

void UOperationModifierSubModule::GroupByContext(const FBuffOperationPack& Pack, TMap<FGameplayTag, FBuffOperationPack>& OutPerContextPacks)
{
	OutPerContextPacks.Empty();
	for (const auto item : Pack.Modifiers)
	{
		if (OutPerContextPacks.Contains(item.Context))
			OutPerContextPacks[item.Context].Modifiers.Add(item);
		else
			OutPerContextPacks.Add(item.Context, FBuffOperationPack({item}));
	}
}
