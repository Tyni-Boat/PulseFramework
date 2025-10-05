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
	if (!_bHasAuthority)
		return;
	for (auto& elem : TargetPack)
	{
		TArray<FBuffOperationModifier> expiredOnes;
		if (!elem.Value.UpdateModifierPack(DeltaTime, expiredOnes))
			continue;
		for (const auto& buff : expiredOnes)
		{
			FString uid = buff.UID;
			RemoveModifier(this, uid);
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
		if (addCmd.Modifier.UID.IsEmpty())
		{
			// Make The UID
			auto guid = FGuid::NewGuid();
			addCmd.Modifier.UID = guid.ToString();
		}
		addCmd.RepCommand = addCmd.RepCommand.WithName(FName(addCmd.Modifier.UID));
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

		// Replicate Add command
		if (_bCanReplicate)
		{
			FString strTag = FString::Printf(TEXT("%s.%s"), *addCmd.RepCommand.Tag.ToString(), *addCmd.Modifier.UID);
			IIPulseNetObject::Execute_ReplicateValue(this, FName(strTag), addCmd.RepCommand);
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
		if (auto hdl = _modifierHandlers.Find(remCmd.UID))
		{
			if (hdl)
				if ((*hdl)->IsValid())
					(*hdl)->Execute_ReturnToPool(hdl->Get());
			_modifierHandlers.Remove(remCmd.UID);
		}
		
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
		
		// Replicate Add command
		if (_bCanReplicate)
		{
			FString strTag = FString::Printf(TEXT("%s.%s"), *remCmd.RepCommand.Tag.ToString(), *remCmd.Modifier.UID);
			IIPulseNetObject::Execute_RemoveReplicationTag(this, FName(strTag), remCmd.Actor.Get());
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
                                                                           AActor* Actor, bool _bOverrideNetAuth)
{
	OutCommand.CharacterID = Modifier.CharacterID;
	OutCommand.Modifier = Modifier;
	OutCommand.Actor = Actor;
	OutCommand.Category = Category;
	if (Modifier.CharacterID.IsValid() && Modifier.PlayerIndex >= 0)
	{
		OutCommand.catType = EBuffCategoryType::PlayerCharacter;
		OutCommand.RepCommand = EncodeFromModifier(OutCommand.Modifier, Category, OutCommand.catType, OutCommand.Actor.Get());
		return (_bHasAuthority || _bOverrideNetAuth)? &_playerCharacterAddCmd : nullptr;
	}
	if (Modifier.UserID.IsValid() && !Modifier.UserID.IsNone())
	{
		OutCommand.catType = EBuffCategoryType::User;
		OutCommand.RepCommand = EncodeFromModifier(OutCommand.Modifier, Category, OutCommand.catType, OutCommand.Actor.Get());
		return (_bHasAuthority || _bOverrideNetAuth)? &_playerUserAddCmd : nullptr;
	}
	if (Modifier.CharacterID.IsValid())
	{
		OutCommand.catType = EBuffCategoryType::NPCCharacter;
		OutCommand.RepCommand = EncodeFromModifier(OutCommand.Modifier, Category, OutCommand.catType, OutCommand.Actor.Get());
		return (_bIsServer || _bOverrideNetAuth)? &_npcCharacterAddCmd : nullptr;
	}
	if (Modifier.PlayerIndex >= 0)
	{
		OutCommand.catType = EBuffCategoryType::PlayerController;
		OutCommand.RepCommand = EncodeFromModifier(OutCommand.Modifier, Category, OutCommand.catType, OutCommand.Actor.Get());
		return (_bHasAuthority || _bOverrideNetAuth)? &_playerSessionAddCmd : nullptr;
	}
	if (Actor != nullptr)
	{
		OutCommand.catType = EBuffCategoryType::Actor;
		OutCommand.RepCommand = EncodeFromModifier(OutCommand.Modifier, Category, OutCommand.catType, OutCommand.Actor.Get());
		return (Actor->HasAuthority() || _bOverrideNetAuth)? &_actorAddCmd : nullptr;
	}
	return nullptr;
}

TQueue<FBuffRemoveOperationCommand>* UOperationModifierSubModule::GetRemoveQueue(const FString& ModifierUID, FBuffRemoveOperationCommand& OutCommand, bool _bOverrideNetAuth)
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
		OutCommand.RepCommand = EncodeFromModifier(_actorModifiers[category].Modifiers[index], category, OutCommand.CatType, actor.Get());
		return ((actor.Get() && actor->HasAuthority()) || _bOverrideNetAuth)? &_actorRemoveCmd : nullptr;
	}
	if (FindBuff(_npcCharacterModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::NPCCharacter;
		OutCommand.RepCommand = EncodeFromModifier(_npcCharacterModifiers[category].Modifiers[index], category, OutCommand.CatType, actor.Get());
		return (_bIsServer || _bOverrideNetAuth)? &_npcCharacterRemoveCmd : nullptr;
	}
	if (FindBuff(_playerCharacterModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::PlayerCharacter;
		OutCommand.RepCommand = EncodeFromModifier(_playerCharacterModifiers[category].Modifiers[index], category, OutCommand.CatType, actor.Get());
		return (_bHasAuthority || _bOverrideNetAuth)? &_playerCharacterRemoveCmd : nullptr;
	}
	if (FindBuff(_playerSessionModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::PlayerController;
		OutCommand.RepCommand = EncodeFromModifier(_playerSessionModifiers[category].Modifiers[index], category, OutCommand.CatType, actor.Get());
		return (_bHasAuthority || _bOverrideNetAuth)? &_playerSessionRemoveCmd : nullptr;
	}
	if (FindBuff(_playerUserModifiers, ModifierUID, category, index, actor))
	{
		OutCommand.Actor = actor;
		OutCommand.CatType = EBuffCategoryType::User;
		OutCommand.RepCommand = EncodeFromModifier(_playerUserModifiers[category].Modifiers[index], category, OutCommand.CatType, actor.Get());
		return (_bHasAuthority || _bOverrideNetAuth)? &_playerUserRemoveCmd : nullptr;
	}
	return nullptr;
}

FReplicatedEntry UOperationModifierSubModule::EncodeFromModifier(const FBuffOperationModifier& Modifier, const FName& Category, EBuffCategoryType CategoryType, AActor* Actor) const
{
	FName tag = "";
	switch (CategoryType)
	{
	case EBuffCategoryType::Actor: tag = _actorReplicationTag; break;
	case EBuffCategoryType::PlayerCharacter: tag = _characterReplicationTag; break;
	case EBuffCategoryType::PlayerController: tag = _sessionReplicationTag; break;
	case EBuffCategoryType::NPCCharacter: tag = _npcReplicationTag; break;
	case EBuffCategoryType::User: tag = _userReplicationTag; break;
	}
	FString AddData = FString::Printf(TEXT("%s|%s|%s"), *Modifier.UserID.ToString(), *Modifier.Context.GetTagName().ToString(), *Modifier.CharacterID.ToString());
	return FReplicatedEntry(tag).WithEnumValue(static_cast<uint8>(CategoryType))
	.WithName(FName(Modifier.UID), 0)
	.WithName(Category, 1)
	.WithName(FName(AddData), 2)
	.WithObject(Actor)
	.WithClass(Modifier.GameplayAbilityEffect)
	.WithDouble(Modifier.Duration)
	.WithVector(FVector(Modifier.Value, Modifier.bValueAsPercentage, static_cast<uint8>(Modifier.Operator)), 0)
	.WithVector(FVector(Modifier.PlayerIndex, static_cast<uint8>(Modifier.StackingType), Modifier.GameplayAbilityEffectLevel), 1)
	.WithVector(FVector(Modifier.LifeTimeLeft, 0,0), 2);
}

 bool UOperationModifierSubModule::DecodeToModifier(const FReplicatedEntry& RepEntry, FBuffOperationModifier& OutModifier, FName& OutCategory, EBuffCategoryType& OutCategoryType, AActor*& OutActor) const
{
	auto catStr = RepEntry.NameValue_1.ToString();
	auto uidStr = RepEntry.NameValue.ToString();
	uidStr.RemoveSpacesInline();
	catStr.RemoveSpacesInline();
	if (catStr.IsEmpty() || uidStr.IsEmpty())
		return false;
	OutCategory = FName(catStr);
	OutCategoryType = static_cast<EBuffCategoryType>(RepEntry.EnumValue);
	OutActor = Cast<AActor>(RepEntry.WeakObjectPtr.Get());
	FBuffOperationModifier Modifier;
	FString userId, rest, context, charID;
	auto addData = RepEntry.NameValue_2.ToString();
	addData.Split("|", &userId, &rest);
	rest.Split("|", &context, &charID);
	auto contextTag = FGameplayTag::RequestGameplayTag(FName(context), false);
	if (!contextTag.IsValid())
		return false;
	Modifier.UID = uidStr;
	Modifier.Context = contextTag;
	Modifier.UserID = FName(userId);
	Modifier.PlayerIndex = RepEntry.Float32Value.X;
	Modifier.CharacterID = FPrimaryAssetId::FromString(charID);
	Modifier.GameplayAbilityEffect = RepEntry.SoftClassPtr.Get();
	Modifier.Duration = RepEntry.DoubleValue;
	Modifier.Value = RepEntry.Float31Value.X;
	Modifier.bValueAsPercentage = RepEntry.Float31Value.Y > 0;
	Modifier.Operator = static_cast<ENumericOperator>(RepEntry.Float31Value.Z);
	Modifier.StackingType = static_cast<EBuffStackingType>(RepEntry.Float32Value.Y);
	Modifier.GameplayAbilityEffectLevel = RepEntry.Float32Value.Z;
	Modifier.LifeTimeLeft = RepEntry.Float33Value.X;
	OutModifier = Modifier;
	return true;
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


UClass* UOperationModifierSubModule::GetSaveClassType_Implementation()
{
	return USaveBuffOperationManagerData::StaticClass();
}

void UOperationModifierSubModule::OnLoadedObject_Implementation(UObject* LoadedObject)
{
	if (auto saveBuffs = Cast<USaveBuffOperationManagerData>(LoadedObject))
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
		
		// Replicate
		if (_bCanReplicate)
		{
			// User
			for (const auto item : _playerUserModifiers)
			{
				for (const auto mod : item.Value.Modifiers)
				{
					auto netEntry = EncodeFromModifier(mod, item.Key, EBuffCategoryType::User, nullptr);
					FString strTag = FString::Printf(TEXT("%s.%s"), *netEntry.Tag.ToString(), *mod.UID);
					IIPulseNetObject::Execute_ReplicateValue(this, FName(strTag), netEntry);
				}
			}
			// Session
			for (const auto item : _playerSessionModifiers)
			{
				for (const auto mod : item.Value.Modifiers)
				{
					auto netEntry = EncodeFromModifier(mod, item.Key, EBuffCategoryType::PlayerController, nullptr);
					FString strTag = FString::Printf(TEXT("%s.%s"), *netEntry.Tag.ToString(), *mod.UID);
					IIPulseNetObject::Execute_ReplicateValue(this, FName(strTag), netEntry);
				}
			}
			// P.Character
			for (const auto item : _playerCharacterModifiers)
			{
				for (const auto mod : item.Value.Modifiers)
				{
					auto netEntry = EncodeFromModifier(mod, item.Key, EBuffCategoryType::PlayerCharacter, nullptr);
					FString strTag = FString::Printf(TEXT("%s.%s"), *netEntry.Tag.ToString(), *mod.UID);
					IIPulseNetObject::Execute_ReplicateValue(this, FName(strTag), netEntry);
				}
			}
			// NPC
			for (const auto item : _npcCharacterModifiers)
			{
				for (const auto mod : item.Value.Modifiers)
				{
					auto netEntry = EncodeFromModifier(mod, item.Key, EBuffCategoryType::NPCCharacter, nullptr);
					FString strTag = FString::Printf(TEXT("%s.%s"), *netEntry.Tag.ToString(), *mod.UID);
					IIPulseNetObject::Execute_ReplicateValue(this, FName(strTag), netEntry);
				}
			}
		}
	}
}

UObject* UOperationModifierSubModule::OnSaveObject_Implementation(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave)
{
	auto saveBuffs = NewObject<USaveBuffOperationManagerData>(this);
	saveBuffs->PlayerUserModifiers = _playerUserModifiers;
	saveBuffs->PlayerSessionModifiers = _playerSessionModifiers;
	saveBuffs->PlayerCharacterModifiers = _playerCharacterModifiers;
	saveBuffs->NpcCharacterModifiers = _npcCharacterModifiers;
	return saveBuffs;
}


void UOperationModifierSubModule::OnNetInit_Implementation()
{
	if (auto coreModule = Cast<UPulseCoreModule>(_OwningModule))
	{
		if (auto netMgr = coreModule->GetNetManager())
		{
			_bHasAuthority = netMgr->HasAuthority();
			_bIsServer = netMgr->GetNetMode() < NM_Client;
		}
	}
	TArray<FReplicatedEntry> outReplicatedEntries;
	if (IIPulseNetObject::Execute_TryGetNetRepValues(this, _rootReplicationTag, outReplicatedEntries))
	{
		for (int i = 0; i < outReplicatedEntries.Num(); i++)
			IIPulseNetObject::Execute_OnNetValueReplicated(this, outReplicatedEntries[i].Tag, outReplicatedEntries[i], EReplicationEntryOperationType::AddNew);
	}
}

void UOperationModifierSubModule::OnNetValueReplicated_Implementation(const FName Tag, FReplicatedEntry Value, EReplicationEntryOperationType OpType)
{
	FString sTag = Tag.ToString();
	FBuffOperationModifier mod;
	FName Category = "";
	EBuffCategoryType CategoryType = EBuffCategoryType::Actor;
	AActor* Actor = nullptr;
	if (!sTag.Contains(_rootReplicationTag.ToString()))
		return;
	if (DecodeToModifier(Value, mod, Category, CategoryType, Actor))
	{
		switch (OpType)
		{
		case EReplicationEntryOperationType::Update:
			{
				TMap<FName, FBuffOperationPack>* targetContainer = nullptr;
				switch (CategoryType)
				{
				case EBuffCategoryType::Actor: targetContainer = &_actorModifiers;break;
				case EBuffCategoryType::PlayerCharacter: targetContainer = &_playerCharacterModifiers; break;
				case EBuffCategoryType::PlayerController: targetContainer = &_playerSessionModifiers; break;
				case EBuffCategoryType::NPCCharacter: targetContainer = &_npcCharacterModifiers; break;
				case EBuffCategoryType::User: targetContainer = &_playerUserModifiers; break;
				}
				if (targetContainer)
				{
					int32 index = INDEX_NONE;
					TWeakObjectPtr<AActor> weakActor = nullptr;
					if (FindBuff(*targetContainer, mod.UID, Category, index, weakActor))
					{
						(*targetContainer)[Category].Modifiers[index] = mod;
					}
				}
			}
			break;
		case EReplicationEntryOperationType::AddNew:
			AddModifier(this, mod, Category);
			break;
		case EReplicationEntryOperationType::Remove:
			RemoveModifier(this, mod.UID);
			break;
		}
	}
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
	if (auto settings = coreModule->GetProjectConfig())
	{
		_bCanSave = settings->bSaveAndLoadActiveBuffs;
		_bCanReplicate = settings->bReplicateActiveBuffs;
		_autoApplyGameplayEffects = settings->bAutoApplyGameplayEffects;
		UPulsePoolingManager::SetPoolLimitPerClass(this, UBuffModifierHandler::StaticClass(), settings->BuffHandlePoolingLimit);
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
	UKismetSystemLibrary::PrintString(WorldContext, FString::Printf( TEXT("Adding Buff: Override? %d"), WorldContext == mgr));
	if (auto queue = mgr->GetAddQueue(Modifier, Category, cmd, nullptr, WorldContext == mgr))
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
	if (auto queue = mgr->GetRemoveQueue(ModifierUID, cmd, WorldContext == mgr))
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
