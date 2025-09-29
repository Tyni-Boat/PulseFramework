// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PoolingSubModule/PulsePoolingManager.h"

#include "Core/PulseCoreModule.h"
#include "Core/PulseSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

bool FPoolingParams::IsValid() const
{
	if (!TransformParams.IsEmpty())
		return true;
	if (!VectorParams.IsEmpty())
		return true;
	if (!ColorParams.IsEmpty())
		return true;
	if (!RotationParams.IsEmpty())
		return true;
	if (!ValueParams.IsEmpty())
		return true;
	if (!AssetParams.IsEmpty())
		return true;
	if (!NamesParams.IsEmpty())
		return true;
	if (!CustomParams.IsEmpty())
		return true;
	return false;
}

bool IIPoolingObject::OnPoolingObjectSpawned_Implementation(const FPoolingParams SpawnData)
{
	return true;
}

void IIPoolingObject::OnPoolingObjectDespawned_Implementation()
{
}

EPoolQueryResult IIPoolingObject::ReturnToPool_Implementation()
{
	auto obj = Cast<UObject>(this);
	return UPulsePoolingManager::ReturnObjectToPool(obj, obj);
}


FName UPulsePoolingManager::GetSubModuleName() const
{
	return "PulsePoolingSystem";
}

bool UPulsePoolingManager::WantToTick() const
{
	return false;
}

bool UPulsePoolingManager::TickWhenPaused() const
{
	return false;
}

void UPulsePoolingManager::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	if (const auto module = Cast<UPulseCoreModule>(OwningModule))
	{
		_globalPoolLimit = module->GetProjectConfig() ? module->GetProjectConfig()->GlobalPoolLimit : _globalPoolLimit;
	}
}

void UPulsePoolingManager::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
	ClearPool(GetWorld());
}

void UPulsePoolingManager::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}


UObject* UPulsePoolingManager::CreateNewObject(TSoftClassPtr<UObject> ObjectClass)
{
	if (!ObjectClass.IsValid())
		return nullptr;
	UClass* Class = ObjectClass.Get();
	if (!Class)
		return nullptr;
	if (Class->IsChildOf(AActor::StaticClass()))
	{
		FTransform spawnTr = FTransform(FQuat::Identity, FVector::ZeroVector);
		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		return GetWorld()->SpawnActor(Class, &spawnTr, spawnParams);
	}
	return NewObject<UObject>(this, Class);
}

void UPulsePoolingManager::MoveObjectToLivePool(TObjectPtr<UObject> Object)
{
	if (!Object)
		return;
	TSoftClassPtr<UObject> ObjectClass = Object->GetClass();
	if (IsValid(Object))
	{
		if (PoolingLiveObjectMap.Contains(ObjectClass))
		{
			// If the object type is already in the live pool, Add this instance to his list.
			PoolingLiveObjectMap[ObjectClass].ObjectArray.AddUnique(Object);
		}
		else
		{
			// If not, Add it.
			PoolingLiveObjectMap.Add(ObjectClass, FPoolingTypeObjects({Object}));
		}
	}

	// Remove any reference to this object in the dormant pool.
	if (PoolingDormantObjectMap.Contains(ObjectClass))
	{
		PoolingDormantObjectMap[ObjectClass].ObjectArray.Remove(Object);

		// if the array is empty, remove the entry from the map
		if (PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() <= 0)
		{
			PoolingDormantObjectMap.Remove(ObjectClass);
		}
	}
}

void UPulsePoolingManager::MoveObjectToDormantPool(TObjectPtr<UObject> Object)
{
	if (!Object)
		return;
	TSoftClassPtr<UObject> ObjectClass = Object->GetClass();
	if (IsValid(Object))
	{
		if (PoolingDormantObjectMap.Contains(ObjectClass))
		{
			// If the object type is already in the Dormant pool, Add this instance to his list.
			PoolingDormantObjectMap[ObjectClass].ObjectArray.AddUnique(Object);
		}
		else
		{
			// If not, Add it.
			PoolingDormantObjectMap.Add(ObjectClass, FPoolingTypeObjects({Object}));
		}
	}

	// Remove any reference to this object in the live pool.
	if (PoolingLiveObjectMap.Contains(ObjectClass))
	{
		PoolingLiveObjectMap[ObjectClass].ObjectArray.Remove(Object);

		// if the array is empty, remove the entry from the map
		if (PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() <= 0)
		{
			PoolingLiveObjectMap.Remove(ObjectClass);
		}
	}
}

void UPulsePoolingManager::CleanUpPools(TSoftClassPtr<UObject> Class)
{
	if (PoolingLiveObjectMap.Contains(Class))
	{
		PoolingLiveObjectMap[Class].ObjectArray.RemoveAll([](UObject* obj) { return !obj->IsValidLowLevelFast(); });
		if (PoolingLiveObjectMap[Class].ObjectArray.Num() <= 0)
		{
			PoolingLiveObjectMap.Remove(Class);
		}
	}

	if (PoolingDormantObjectMap.Contains(Class))
	{
		PoolingDormantObjectMap[Class].ObjectArray.RemoveAll([](UObject* obj) { return !obj->IsValidLowLevelFast(); });
		if (PoolingDormantObjectMap[Class].ObjectArray.Num() <= 0)
		{
			PoolingDormantObjectMap.Remove(Class);
		}
	}
}

void UPulsePoolingManager::OnDestroyLinkedActor_Internal(AActor* Actor)
{
	if (!Actor)
		return;
	if (!_linkedPoolObjectActors.Contains(Actor))
		return;
	auto pooledObjs = _linkedPoolObjectActors[Actor].ObjectArray;
	for (auto obj : pooledObjs)
	{
		ReturnObjectToPool(Actor, obj);
	}
	_linkedPoolObjectActors.Remove(Actor);
}

EPoolQueryResult UPulsePoolingManager::GetObjectFromPool(UObject* WorldContext, TSubclassOf<UObject> Class, UObject*& Output, FPoolingParams QueryParams)
{
	if (!Class)
		return EPoolQueryResult::BadOrNullObjectClass;
	if (!WorldContext)
		return EPoolQueryResult::BadOrNullWorldContext;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		UObject* Object = nullptr;
		TSoftClassPtr<UObject> ObjectClass = TSoftClassPtr<UObject>(Class);
		PoolingSystem->CleanUpPools(ObjectClass);

		if (PoolingSystem->PoolingDormantObjectMap.Contains(ObjectClass) && !PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray.IsEmpty())
		{
			// If the pool is not empty, get the first object from the dormant pool.
			Object = PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray[0];
		}
		else
		{
			// Get the limit value that will be used to check if we can create a new object.
			int32 PoolLimit = PoolingSystem->PerClassPoolLimit.Contains(ObjectClass)
				                  ? PoolingSystem->PerClassPoolLimit[ObjectClass]
				                  : PoolingSystem->_globalPoolLimit;
			// Get the number of objects in the live and dormant pool for this class.
			int32 LiveCount = PoolingSystem->PoolingLiveObjectMap.Contains(ObjectClass) ? PoolingSystem->PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() : 0;
			int32 DormantCount = PoolingSystem->PoolingDormantObjectMap.Contains(ObjectClass) ? PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() : 0;
			// If the total number of objects in the live and dormant pool is greater than or equal to the pool limit, we can't create a new object.
			if ((LiveCount + DormantCount) >= PoolLimit)
				return EPoolQueryResult::PoolLimitReached;
			// If the pool is empty or there are no object for this class yet, create a new object.
			Object = PoolingSystem->CreateNewObject(ObjectClass);
		}


		if (!Object)
			return EPoolQueryResult::Undefined;
		PoolingSystem->MoveObjectToLivePool(Object);
		AActor* owningActor = Cast<AActor>(WorldContext);

		if (auto asActor = Cast<AActor>(Object))
		{
			UPulseSystemLibrary::EnableActor(asActor, true);
		}
		else if (auto asComponent = Cast<UActorComponent>(Object))
		{
			// If the object is a component, we need to add it to an actor.
			if (!UPulseSystemLibrary::AddComponentAtRuntime(owningActor, asComponent))
			{
				// If we can't add the component, we can't move it to the active pool.
				ReturnObjectToPool(WorldContext, Object);
				return EPoolQueryResult::UnableToTransfertOwnership;
			}
		}

		// If it's an actor who requested an object, we need to track that actor life.
		if (owningActor)
		{
			auto compList = &PoolingSystem->_linkedPoolObjectActors.FindOrAdd(owningActor).ObjectArray;
			compList->AddUnique(Object);
			if (compList->Num() <= 1)
				owningActor->OnDestroyed.AddDynamic(PoolingSystem, &UPulsePoolingManager::OnDestroyLinkedActor_Internal);
		}

		if (QueryParams.IsValid())
		{
			// If QueryParams is provided, we can set some properties on the object if needed.
			if (Object->Implements<UIPoolingObject>())
			{
				if (!IIPoolingObject::Execute_OnPoolingObjectSpawned(Object, QueryParams))
				{
					// If the object does not want to be spawned, we can't use it.
					ReturnObjectToPool(WorldContext, Object);
					return EPoolQueryResult::InvalidParams;
				}
			}
		}
		Output = Object; // Return the object to the caller.
		return EPoolQueryResult::Success;
	}
	return EPoolQueryResult::BadOrNullWorldContext;
}

void UPulsePoolingManager::RegisterExistingObjectToPool(const UObject* WorldContext, UObject*& Object)
{
	if (!Object || !WorldContext)
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		PoolingSystem->MoveObjectToLivePool(Object);
	}
}

void UPulsePoolingManager::TryPreloadIntoPool(const UObject* WorldContext, TArray<TSoftClassPtr<UObject>> ObjectClasses, int32 CountPerObject)
{
	TArray<TSoftClassPtr<UObject>> workingClasses;
	for (auto classPtr : ObjectClasses)
	{
		if (classPtr.IsValid())
		{
			workingClasses.AddUnique(classPtr);
		}
	}
	if (workingClasses.IsEmpty())
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		for (auto classPtr : workingClasses)
		{
			int32 PoolLimit = PoolingSystem->PerClassPoolLimit.Contains(classPtr) ? PoolingSystem->PerClassPoolLimit[classPtr] : PoolingSystem->_globalPoolLimit;
			int32 DormantCount = PoolingSystem->PoolingDormantObjectMap.Contains(classPtr) ? PoolingSystem->PoolingDormantObjectMap[classPtr].ObjectArray.Num() : 0;
			int32 maxCount = FMath::Min(FMath::Max(PoolLimit - DormantCount, 0), CountPerObject); // Ensure we don't try to preload more than the limit.
			if (maxCount <= 0)
				continue; // If the pool is already full, we don't need to preload anything.
			for (int32 i = 0; i < maxCount; i++)
			{
				UObject* Object = PoolingSystem->CreateNewObject(classPtr);
				if (!Object)
					continue;
				RegisterExistingObjectToPool(WorldContext, Object);
				ReturnObjectToPool(WorldContext, Object);
			}
		}
	}
}

EPoolQueryResult UPulsePoolingManager::ReturnObjectToPool(const UObject* WorldContext, UObject* Object)
{
	if (!Object)
		return EPoolQueryResult::InvalidParams;
	if (!WorldContext)
		return EPoolQueryResult::BadOrNullWorldContext;
	TSoftClassPtr<UObject> ObjectClass = Object->GetClass();
	if (!ObjectClass.IsValid())
		return EPoolQueryResult::BadOrNullObjectClass;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		PoolingSystem->CleanUpPools(ObjectClass);
		int poolIndex = INDEX_NONE;
		if (PoolingSystem->PoolingLiveObjectMap.Contains(ObjectClass))
			poolIndex = PoolingSystem->PoolingLiveObjectMap[ObjectClass].ObjectArray.Find(Object);
		if (poolIndex == INDEX_NONE)
			return EPoolQueryResult::PoolLimitReached; // Object is not in the live pool, nothing to do.

		if (Object->Implements<UIPoolingObject>())
		{
			IIPoolingObject::Execute_OnPoolingObjectDespawned(Object);
		}
		if (auto asActor = Cast<AActor>(Object))
		{
			UPulseSystemLibrary::EnableActor(asActor, false);
		}
		else if (auto asComponent = Cast<UActorComponent>(Object))
		{
			auto owningActor = asComponent->GetOwner();
			if (!UPulseSystemLibrary::RemoveComponentAtRuntime(owningActor, asComponent))
				return EPoolQueryResult::UnableToTransfertOwnership; // If we can't remove the component, we can't return it to the pool.
		}

		// If the returned object was previously requested by an actor, we need to remove it from the linked actor list.
		TArray<TWeakObjectPtr<AActor>> actorKeys;
		PoolingSystem->_linkedPoolObjectActors.GetKeys(actorKeys);
		for (auto itemKey : actorKeys)
		{
			int32 idx = PoolingSystem->_linkedPoolObjectActors[itemKey].ObjectArray.IndexOfByPredicate([Object](const UObject* obj)-> bool { return obj == Object; });
			if (idx == INDEX_NONE)
				continue;
			PoolingSystem->_linkedPoolObjectActors[itemKey].ObjectArray.RemoveAt(idx);
			if (PoolingSystem->_linkedPoolObjectActors[itemKey].ObjectArray.Num() <= 0)
			{
				itemKey->OnDestroyed.RemoveDynamic(PoolingSystem, &UPulsePoolingManager::OnDestroyLinkedActor_Internal);
				PoolingSystem->_linkedPoolObjectActors.Remove(itemKey);
			}
		}

		// If the number of dormant objects for this class is greater than the limit, we need to destroy the object.
		int32 PoolLimit = PoolingSystem->PerClassPoolLimit.Contains(ObjectClass) ? PoolingSystem->PerClassPoolLimit[ObjectClass] : PoolingSystem->_globalPoolLimit;
		int32 DormantCount = PoolingSystem->PoolingDormantObjectMap.Contains(ObjectClass) ? PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() : 0;
		if (DormantCount >= PoolLimit)
		{
			// If the pool is full, we need to destroy the object.
			if (auto asActor = Cast<AActor>(Object))
			{
				asActor->Destroy();
			}
			else if (auto asComponent = Cast<UActorComponent>(Object))
			{
				asComponent->ConditionalBeginDestroy();
			}
			PoolingSystem->PoolingLiveObjectMap[ObjectClass].ObjectArray.RemoveAt(poolIndex);
			if (PoolingSystem->PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() <= 0)
			{
				// If the array is empty, remove the entry from the map
				PoolingSystem->PoolingLiveObjectMap.Remove(ObjectClass);
			}
			return EPoolQueryResult::Success;
		}
		PoolingSystem->MoveObjectToDormantPool(Object);
		return EPoolQueryResult::Success;
	}
	return EPoolQueryResult::BadOrNullWorldContext;
}

void UPulsePoolingManager::ClearPoolType(const UObject* WorldContext, TSoftClassPtr<UObject> ObjectClass)
{
	if (!WorldContext)
		return;
	if (!ObjectClass.IsValid())
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		if (PoolingSystem->PoolingLiveObjectMap.Contains(ObjectClass))
		{
			for (int i = PoolingSystem->PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() - 1; i >= 0; i--)
			{
				ReturnObjectToPool(WorldContext, PoolingSystem->PoolingLiveObjectMap[ObjectClass].ObjectArray[i]);
			}
			// Clear the live pool for this object class.
			PoolingSystem->PoolingLiveObjectMap.Remove(ObjectClass);
		}
		if (PoolingSystem->PoolingDormantObjectMap.Contains(ObjectClass))
		{
			// Destroy all dormant object of this type
			for (int32 i = PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() - 1; i >= 0; i--)
			{
				if (PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray.IsValidIndex(i) && PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray[i])
				{
					if (PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray[i] == nullptr)
						continue;
					if (PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray[i]->IsUnreachable())
						continue;

					if (auto asActor = Cast<AActor>(PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray[i]))
						asActor->Destroy();
					else if (auto asComponent = Cast<UActorComponent>(PoolingSystem->PoolingDormantObjectMap[ObjectClass].ObjectArray[i]))
						asComponent->ConditionalBeginDestroy();
				}
			}
			// Clear the dormant pool for this object class.
			PoolingSystem->PoolingDormantObjectMap.Remove(ObjectClass);
		}
	}
}

void UPulsePoolingManager::ClearPool(const UObject* WorldContext)
{
	if (!WorldContext)
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		TArray<TSoftClassPtr<UObject>> allDormantClasses;
		TArray<TSoftClassPtr<UObject>> allLiveClasses;
		PoolingSystem->PoolingDormantObjectMap.GetKeys(allDormantClasses);
		PoolingSystem->PoolingLiveObjectMap.GetKeys(allLiveClasses);
		for (auto clas : allLiveClasses)
			allDormantClasses.AddUnique(clas);
		for (auto clas : allDormantClasses)
			ClearPoolType(WorldContext, clas);
	}
}

void UPulsePoolingManager::SetPoolLimitPerClass(const UObject* WorldContext, TSoftClassPtr<UObject> ObjectClass, int32 PoolLimit, ENumericOperator Operation)
{
	if (!WorldContext)
		return;
	if (!ObjectClass.IsValid())
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		int32 value = PoolingSystem->PerClassPoolLimit.Contains(ObjectClass) ? PoolingSystem->PerClassPoolLimit[ObjectClass] : PoolingSystem->_globalPoolLimit;
		switch (Operation)
		{
		case ENumericOperator::Set:
			value = PoolLimit;
			break;
		case ENumericOperator::Add:
			value += PoolLimit;
			break;
		case ENumericOperator::Sub:
			value -= PoolLimit;
			break;
		case ENumericOperator::Mul:
			value *= PoolLimit;
			break;
		case ENumericOperator::Div:
			value /= PoolLimit;
			break;
		case ENumericOperator::Mod:
			value %= PoolLimit;
			break;
		default:
			value = PoolLimit;
			break;
		}
		value = FMath::Max(value, 0); // Ensure the value is not negative.
		if (PoolingSystem->PerClassPoolLimit.Contains(ObjectClass))
		{
			// If the object class is already in the per-class pool limit, update the value.
			PoolingSystem->PerClassPoolLimit[ObjectClass] = value;
		}
		else
		{
			// If not, Add it.
			PoolingSystem->PerClassPoolLimit.Add(ObjectClass, value);
		}
	}
}

void UPulsePoolingManager::SetGlobalPoolLimit(const UObject* WorldContext, int32 PoolLimit, ENumericOperator Operation)
{
	if (!WorldContext)
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		int32 value = PoolingSystem->_globalPoolLimit;
		switch (Operation)
		{
		case ENumericOperator::Set:
			value = PoolLimit;
			break;
		case ENumericOperator::Add:
			value += PoolLimit;
			break;
		case ENumericOperator::Sub:
			value -= PoolLimit;
			break;
		case ENumericOperator::Mul:
			value *= PoolLimit;
			break;
		case ENumericOperator::Div:
			value /= PoolLimit;
			break;
		case ENumericOperator::Mod:
			value %= PoolLimit;
			break;
		default:
			value = PoolLimit;
			break;
		}
		value = FMath::Max(value, 0); // Ensure the value is not negative.
		PoolingSystem->_globalPoolLimit = value;
	}
}

void UPulsePoolingManager::DebugPoolingSystem(const UObject* WorldContext, FLinearColor TextColor, float Duration, FName Key, bool Log)
{
	if (!WorldContext)
		return;
	if (auto PoolingSystem = UPulseSystemLibrary::GetPulseSubModule<UPulseCoreModule, UPulsePoolingManager>(WorldContext))
	{
		FString text = FString::Printf(TEXT("Pooling System Debug:\n"));
		text.Append(FString::Printf(TEXT("\tGlobal Limit: %d\n"), PoolingSystem->_globalPoolLimit));
		text.Append(FString::Printf(TEXT("\n\tLive Pool:\n")));
		if (PoolingSystem->PoolingLiveObjectMap.IsEmpty())
		{
			text.Append(TEXT("\t\t[Empty]\n"));
		}
		else
		{
			for (auto item : PoolingSystem->PoolingLiveObjectMap)
			{
				int32 limit = PoolingSystem->PerClassPoolLimit.Contains(item.Key) ? PoolingSystem->PerClassPoolLimit[item.Key] : PoolingSystem->_globalPoolLimit;
				text.Append(FString::Printf(TEXT("\t\t[%s: %d, of %d limit]\n"), *item.Key.ToString(), item.Value.ObjectArray.Num(), limit));
			}
		}
		text.Append(FString::Printf(TEXT("\n\tDormant Pool:\n")));
		if (PoolingSystem->PoolingDormantObjectMap.IsEmpty())
		{
			text.Append(TEXT("\t\t[Empty]\n"));
		}
		else
		{
			for (auto item : PoolingSystem->PoolingDormantObjectMap)
			{
				int32 limit = PoolingSystem->PerClassPoolLimit.Contains(item.Key) ? PoolingSystem->PerClassPoolLimit[item.Key] : PoolingSystem->_globalPoolLimit;
				text.Append(FString::Printf(TEXT("\t\t[%s: %d, of %d limit]\n"), *item.Key.ToString(), item.Value.ObjectArray.Num(), limit));
			}
		}
		text.Append(FString::Printf(TEXT("\n\tLinked Actor-Object:\n")));
		if (PoolingSystem->_linkedPoolObjectActors.IsEmpty())
		{
			text.Append(TEXT("\t\t[Empty]\n"));
		}
		else
		{
			for (auto item : PoolingSystem->_linkedPoolObjectActors)
			{
				if (auto key = item.Key.Get())
					text.Append(FString::Printf(TEXT("\t\t[%s Requested %d Objects]\n"), *key->GetFName().ToString(), item.Value.ObjectArray.Num()));
			}
		}
		text.Append(TEXT("\n"));
		UKismetSystemLibrary::PrintString(WorldContext, text, true, Log, TextColor, Duration, Key);
	}
}
