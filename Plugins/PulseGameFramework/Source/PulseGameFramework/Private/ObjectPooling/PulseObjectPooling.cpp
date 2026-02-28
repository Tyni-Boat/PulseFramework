// Copyright © by Tyni Boat. All Rights Reserved.


#include "ObjectPooling/PulseObjectPooling.h"

#include "PulseGameFramework.h"
#include "Core/PulseSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ObjectPooling/IPulsePoolableObject.h"


void UPulseObjectPooling::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (const auto projectConfig = GetProjectSettings())
	{
		_globalPoolLimit = projectConfig->GlobalPoolLimit;
	}
	UE_LOG(LogPulseObjectPooling, Log, TEXT("Pooling sub-system initialized"));
}

void UPulseObjectPooling::Deinitialize()
{
	Super::Deinitialize();
	ClearPool();
}

bool UPulseObjectPooling::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::EditorPreview ||
		WorldType == EWorldType::Game ||
		WorldType == EWorldType::PIE ||
		WorldType == EWorldType::GamePreview ||
		WorldType == EWorldType::GameRPC;
}


UObject* UPulseObjectPooling::CreateNewObject(TSoftClassPtr<UObject> ObjectClass, UObject* Outer)
{
	if (!ObjectClass.IsValid())
		return nullptr;
	UClass* Class = ObjectClass.Get();
	UObject* Object = nullptr;
	if (!Class)
		return nullptr;
	if (Class->IsChildOf(AActor::StaticClass()))
	{
		FTransform spawnTr = FTransform(FQuat::Identity, FVector::ZeroVector);
		FActorSpawnParameters spawnParams;
		spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AActor* ActorObj = GetWorld()->SpawnActor(Class, &spawnTr, spawnParams);
		if (ActorObj->GetIsReplicated() && GetWorld()->GetNetMode() >= NM_Client)
		{
			UE_LOG(LogPulseObjectPooling, Error, TEXT("Spawning Replicated Actor is prohibited on clients"));
			ActorObj->Destroy();
			return nullptr;
		}
		Object = ActorObj;
	}
	Object = NewObject<UObject>(Outer ? Outer : this, Class);
	if (Object && Class->IsChildOf(UActorComponent::StaticClass()))
	{
		if (UActorComponent* Component = Cast<UActorComponent>(Object))
		{
			if (Component->GetIsReplicated() && GetWorld()->GetNetMode() >= NM_Client)
			{
				UE_LOG(LogPulseObjectPooling, Error, TEXT("Creating Replicated Components is prohibited on clients"));
				Component->DestroyComponent();
				return nullptr;
			}
		}
	}
	UE_LOG(LogPulseObjectPooling, Log, TEXT("Created new object of type %s"), *Class->GetName());
	return Object;
}

void UPulseObjectPooling::MoveObjectToLivePool(TObjectPtr<UObject> Object)
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

void UPulseObjectPooling::MoveObjectToDormantPool(TObjectPtr<UObject> Object)
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

void UPulseObjectPooling::CleanUpPools(TSoftClassPtr<UObject> Class)
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

void UPulseObjectPooling::OnDestroyLinkedActor_Internal(AActor* Actor)
{
	if (!Actor)
		return;
	if (!_linkedPoolObjectActors.Contains(Actor))
		return;
	auto pooledObjs = _linkedPoolObjectActors[Actor].ObjectArray;
	for (auto obj : pooledObjs)
	{
		DisposeObject(obj);
	}
	_linkedPoolObjectActors.Remove(Actor);
}

UPulseObjectPooling* UPulseObjectPooling::Get(const UObject* WorldContext)
{
	if (!WorldContext)
		return nullptr;
	const auto world = WorldContext->GetWorld();
	if (!world)
		return nullptr;
	return world->GetSubsystem<UPulseObjectPooling>();
}

EPoolQueryResult UPulseObjectPooling::QueryObject(UObject* Owner, TSubclassOf<UObject> Class, UObject*& Output, FPoolingParams QueryParams)
{
	if (!Class)
	{
		UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot QueryObject: Null object class"));
		return EPoolQueryResult::BadOrNullObjectClass;
	}
	UObject* Object = nullptr;
	TSoftClassPtr<UObject> ObjectClass = TSoftClassPtr<UObject>(Class);
	CleanUpPools(ObjectClass);

	if (PoolingDormantObjectMap.Contains(ObjectClass) && !PoolingDormantObjectMap[ObjectClass].ObjectArray.IsEmpty())
	{
		// If the pool is not empty, get the first object from the dormant pool.
		Object = PoolingDormantObjectMap[ObjectClass].ObjectArray[0];
	}
	else
	{
		// Get the limit value that will be used to check if we can create a new object.
		int32 PoolLimit = PerClassPoolLimit.Contains(ObjectClass)
			                  ? PerClassPoolLimit[ObjectClass]
			                  : _globalPoolLimit;
		// Get the number of objects in the live and dormant pool for this class.
		int32 LiveCount = PoolingLiveObjectMap.Contains(ObjectClass) ? PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() : 0;
		int32 DormantCount = PoolingDormantObjectMap.Contains(ObjectClass) ? PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() : 0;
		// If the total number of objects in the live and dormant pool is greater than or equal to the pool limit, we can't create a new object.
		if ((LiveCount + DormantCount) >= PoolLimit)
			return EPoolQueryResult::PoolLimitReached;
		// If the pool is empty or there are no object for this class yet, create a new object.
		Object = CreateNewObject(ObjectClass, Owner);
		if (!Object && Class)
			return EPoolQueryResult::ProhibitedOnClient;
	}

	if (!Object)
	{
		UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot QueryObject of type %s: Object is null"), *Class->GetName());
		return EPoolQueryResult::Undefined;
	}
	auto OwningActor = Cast<AActor>(Owner);
	MoveObjectToLivePool(Object);
	if (auto asActor = Cast<AActor>(Object))
	{
		UPulseSystemLibrary::EnableActor(asActor, true);
	}
	else if (auto asComponent = Cast<UActorComponent>(Object))
	{
		// If the object is a component, we need to add it to an actor.
		if (OwningActor && !UPulseSystemLibrary::AddComponentAtRuntime(OwningActor, asComponent))
		{
			// If we can't add the component, we can't move it to the active pool.
			DisposeObject(Object);
			UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot QueryObject of type %s: Unable to transfer component ownership"), *Class->GetName());
			return EPoolQueryResult::UnableToTransferOwnership;
		}
	}

	// If it's an actor who requested an object, we need to track that actor life.
	if (OwningActor)
	{
		auto compList = &_linkedPoolObjectActors.FindOrAdd(OwningActor).ObjectArray;
		compList->AddUnique(Object);
		if (compList->Num() <= 1)
			OwningActor->OnDestroyed.AddDynamic(this, &UPulseObjectPooling::OnDestroyLinkedActor_Internal);
	}

	if (QueryParams.IsValid())
	{
		// If QueryParams is provided, we can set some properties on the object if needed.
		if (Object->Implements<UIPulsePoolableObject>())
		{
			if (!IIPulsePoolableObject::Execute_OnPoolQuery(Object, QueryParams))
			{
				// If the object does not want to be spawned, we can't use it.
				DisposeObject(Object);
				UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot QueryObject of type %s: Invalid Query params"), *Class->GetName());
				return EPoolQueryResult::InvalidParams;
			}
		}
	}
	Output = Object; // Return the object to the caller.
	UE_LOG(LogPulseObjectPooling, Log, TEXT("QueryObject of type %s"), *Class->GetName());
	return EPoolQueryResult::Success;
}

void UPulseObjectPooling::RegisterExistingObjectToPool(UObject*& Object)
{
	if (!Object)
		return;
	MoveObjectToLivePool(Object);
}

void UPulseObjectPooling::PreFillPool(TArray<TSoftClassPtr<UObject>> ObjectClasses, int32 CountPerObject)
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
	for (auto classPtr : workingClasses)
	{
		int32 PoolLimit = PerClassPoolLimit.Contains(classPtr) ? PerClassPoolLimit[classPtr] : _globalPoolLimit;
		int32 DormantCount = PoolingDormantObjectMap.Contains(classPtr) ? PoolingDormantObjectMap[classPtr].ObjectArray.Num() : 0;
		int32 maxCount = FMath::Min(FMath::Max(PoolLimit - DormantCount, 0), CountPerObject); // Ensure we don't try to preload more than the limit.
		if (maxCount <= 0)
			continue; // If the pool is already full, we don't need to preload anything.
		for (int32 i = 0; i < maxCount; i++)
		{
			UObject* Object = CreateNewObject(classPtr);
			if (!Object)
				continue;
			RegisterExistingObjectToPool(Object);
			DisposeObject(Object);
		}
	}
}

EPoolQueryResult UPulseObjectPooling::DisposeObject(UObject* Object)
{
	if (!Object)
	{
		UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot Dispose: Object is null"));
		return EPoolQueryResult::InvalidParams;
	}
	TSoftClassPtr<UObject> ObjectClass = Object->GetClass();
	if (!ObjectClass.IsValid())
	{
		UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot Dispose: Class is somehow not valid"));
		return EPoolQueryResult::BadOrNullObjectClass;
	}
	CleanUpPools(ObjectClass);
	int poolIndex = INDEX_NONE;
	if (PoolingLiveObjectMap.Contains(ObjectClass))
		poolIndex = PoolingLiveObjectMap[ObjectClass].ObjectArray.Find(Object);
	if (poolIndex == INDEX_NONE)
	{
		UE_LOG(LogPulseObjectPooling, Error, TEXT("Cannot Dispose: Object doesn't belong to any pool"));
		return EPoolQueryResult::PoolLimitReached; // Object is not in the live pool, nothing to do.
	}

	if (Object->Implements<UIPulsePoolableObject>())
	{
		IIPulsePoolableObject::Execute_OnPoolDispose(Object);
	}
	if (auto asActor = Cast<AActor>(Object))
	{
		UPulseSystemLibrary::EnableActor(asActor, false);
	}
	else if (auto asComponent = Cast<UActorComponent>(Object))
	{
		auto owningActor = asComponent->GetOwner();
		if (!UPulseSystemLibrary::RemoveComponentAtRuntime(owningActor, asComponent))
			return EPoolQueryResult::UnableToTransferOwnership; // If we can't remove the component, we can't return it to the pool.
	}

	// If the returned object was previously requested by an actor, we need to remove it from the linked actor list.
	TArray<TWeakObjectPtr<AActor>> actorKeys;
	_linkedPoolObjectActors.GetKeys(actorKeys);
	for (auto itemKey : actorKeys)
	{
		int32 idx = _linkedPoolObjectActors[itemKey].ObjectArray.IndexOfByPredicate([Object](const UObject* obj)-> bool { return obj == Object; });
		if (idx == INDEX_NONE)
			continue;
		_linkedPoolObjectActors[itemKey].ObjectArray.RemoveAt(idx);
		if (_linkedPoolObjectActors[itemKey].ObjectArray.Num() <= 0)
		{
			itemKey->OnDestroyed.RemoveDynamic(this, &UPulseObjectPooling::OnDestroyLinkedActor_Internal);
			_linkedPoolObjectActors.Remove(itemKey);
		}
	}

	// If the number of dormant objects for this class is greater than the limit, we need to destroy the object.
	int32 PoolLimit = PerClassPoolLimit.Contains(ObjectClass) ? PerClassPoolLimit[ObjectClass] : _globalPoolLimit;
	int32 DormantCount = PoolingDormantObjectMap.Contains(ObjectClass) ? PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() : 0;
	UE_LOG(LogPulseObjectPooling, Log, TEXT("Object %s Disposed Successfully"), *Object->GetName());
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
		PoolingLiveObjectMap[ObjectClass].ObjectArray.RemoveAt(poolIndex);
		if (PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() <= 0)
		{
			// If the array is empty, remove the entry from the map
			PoolingLiveObjectMap.Remove(ObjectClass);
		}
		return EPoolQueryResult::Success;
	}
	MoveObjectToDormantPool(Object);
	return EPoolQueryResult::Success;
}

void UPulseObjectPooling::ClearPoolType(TSoftClassPtr<UObject> ObjectClass)
{
	if (!ObjectClass.IsValid())
		return;
	if (PoolingLiveObjectMap.Contains(ObjectClass))
	{
		for (int i = PoolingLiveObjectMap[ObjectClass].ObjectArray.Num() - 1; i >= 0; i--)
		{
			DisposeObject(PoolingLiveObjectMap[ObjectClass].ObjectArray[i]);
		}
		// Clear the live pool for this object class.
		PoolingLiveObjectMap.Remove(ObjectClass);
	}
	if (PoolingDormantObjectMap.Contains(ObjectClass))
	{
		// Destroy all dormant object of this type
		for (int32 i = PoolingDormantObjectMap[ObjectClass].ObjectArray.Num() - 1; i >= 0; i--)
		{
			if (PoolingDormantObjectMap[ObjectClass].ObjectArray.IsValidIndex(i) && PoolingDormantObjectMap[ObjectClass].ObjectArray[i])
			{
				if (PoolingDormantObjectMap[ObjectClass].ObjectArray[i] == nullptr)
					continue;
				if (PoolingDormantObjectMap[ObjectClass].ObjectArray[i]->IsUnreachable())
					continue;

				if (auto asActor = Cast<AActor>(PoolingDormantObjectMap[ObjectClass].ObjectArray[i]))
					asActor->Destroy();
				else if (auto asComponent = Cast<UActorComponent>(PoolingDormantObjectMap[ObjectClass].ObjectArray[i]))
					asComponent->ConditionalBeginDestroy();
			}
		}
		// Clear the dormant pool for this object class.
		PoolingDormantObjectMap.Remove(ObjectClass);
	}
	UE_LOG(LogPulseObjectPooling, Log, TEXT("Class %s Pool Cleared"), *ObjectClass->GetName());
}

void UPulseObjectPooling::ClearPool()
{
	TArray<TSoftClassPtr<UObject>> allDormantClasses;
	TArray<TSoftClassPtr<UObject>> allLiveClasses;
	PoolingDormantObjectMap.GetKeys(allDormantClasses);
	PoolingLiveObjectMap.GetKeys(allLiveClasses);
	for (auto clas : allLiveClasses)
		allDormantClasses.AddUnique(clas);
	for (auto clas : allDormantClasses)
		ClearPoolType(clas);
}

void UPulseObjectPooling::AffectObjectTypePoolLimit(TSoftClassPtr<UObject> ObjectClass, int32 PoolLimit, ENumericOperator Operation)
{
	if (!ObjectClass.IsValid())
		return;
	int32 value = PerClassPoolLimit.Contains(ObjectClass) ? PerClassPoolLimit[ObjectClass] : _globalPoolLimit;
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
	UE_LOG(LogPulseObjectPooling, Log, TEXT("Class %s Pool limit set to %d"), *ObjectClass->GetName(), value);
	if (PerClassPoolLimit.Contains(ObjectClass))
	{
		// If the object class is already in the per-class pool limit, update the value.
		PerClassPoolLimit[ObjectClass] = value;
	}
	else
	{
		// If not, Add it.
		PerClassPoolLimit.Add(ObjectClass, value);
	}
}

void UPulseObjectPooling::AffectGlobalObjectPoolLimit(int32 PoolLimit, ENumericOperator Operation)
{
	int32 value = _globalPoolLimit;
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
	_globalPoolLimit = value;
	UE_LOG(LogPulseObjectPooling, Log, TEXT("Global Pool limit set to %d"), value);
}

void UPulseObjectPooling::DebugPoolingSystem(const UObject* WorldContext, FLinearColor TextColor, float Duration, FName Key, bool Log)
{
	if (!WorldContext)
		return;
	if (auto PoolingSystem = Get(WorldContext))
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
