// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseCoreTypes.h"
#include "ObjectPooling/PulsePoolingTypes.h"
#include "PulseObjectPooling.generated.h"



/**
 * The Pulse Pooling System
 */
UCLASS(BlueprintType)
class PULSEGAMEFRAMEWORK_API UPulseObjectPooling : public UWorldSubsystem, public IIPulseCore
{
	GENERATED_BODY()
	
private:
	UPROPERTY()
	TMap<TSoftClassPtr<UObject>, FPoolingTypeObjects> PoolingLiveObjectMap;
	UPROPERTY()
	TMap<TSoftClassPtr<UObject>, FPoolingTypeObjects> PoolingDormantObjectMap;
	UPROPERTY()
	TMap<TSoftClassPtr<UObject>, int32> PerClassPoolLimit;
	UPROPERTY()
	TMap<TWeakObjectPtr<AActor>, FPoolingTypeObjects> _linkedPoolObjectActors;
	int32 _globalPoolLimit = 100;
	FName _poolRepTag = "PulseCore.Pooling";
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

protected:
	
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	// Function to create a new object of the specified class
	UObject* CreateNewObject(TSoftClassPtr<UObject> ObjectClass, UObject* Outer = nullptr);

	// Function to move an object from the dormant pool to the live pool
	void MoveObjectToLivePool(TObjectPtr<UObject> Object);

	// Function to move an object from the live pool to the dormant pool
	void MoveObjectToDormantPool(TObjectPtr<UObject> Object);

	// Function to remove an now invalid objects from the pools
	void CleanUpPools(TSoftClassPtr<UObject> Class);

	UFUNCTION()
	void OnDestroyLinkedActor_Internal(AActor* Actor);

public:

	static UPulseObjectPooling* Get(const UObject* WorldContext);

	// Function to get an object from the pool. If the object is not found, it will create a new one and return it.
	// be aware that for actor component pooling, the world context will be use as the owner of the component
	// , so make sure to call this function from an actor or manually set the world context to the actor.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling", meta = (AdvancedDisplay = 3, DeterminesOutputType="Class", DynamicOutputParam="Output"))
	EPoolQueryResult QueryObject(UObject* Owner, TSubclassOf<UObject> Class, UObject*& Output, FPoolingParams QueryParams);

	// Register an existing object in the pool (live pool)
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling")
	void RegisterExistingObjectToPool(UObject*& Object);

	// Preload objects of the specified class into the pool. Only the first array element of a set class are preloaded. Those are automatically set dormants
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling")
	void PreFillPool(TArray<TSoftClassPtr<UObject>> ObjectClasses, int32 CountPerObject = 5);

	// Function to return an object to the pool
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling")
	EPoolQueryResult DisposeObject(UObject* Object);


	// Function to clear the pool form the specified class
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling")
	void ClearPoolType(TSoftClassPtr<UObject> ObjectClass);

	// Function to clear the entire pool
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling")
	void ClearPool();

	// Function to Limit the number of objects in the pool for a specific class. Setting the limit to <= 0 will remove the limit for that class.
	// Note: This will not affect the already existing objects in the pool, only the new ones created after setting the limit.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling", meta = (AdvancedDisplay = 2))
	void AffectObjectTypePoolLimit(TSoftClassPtr<UObject> ObjectClass, int32 PoolLimit, ENumericOperator Operation = ENumericOperator::Set);

	// Function to Limit the number of objects in the pool for unspecified classes. Setting the limit to <= 0 will remove the limit..
	// Note: This will not affect the already existing objects in the pool, only the new ones created after setting the limit.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling", meta = (AdvancedDisplay = 2))
	void AffectGlobalObjectPoolLimit(int32 PoolLimit, ENumericOperator Operation = ENumericOperator::Set);

	// Function debug the pooling system, it will print the current state of the pool to the screen. Use Log param to also print into the log
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Pooling", meta = (WorldContext = "WorldContext", AdvancedDisplay = 1))
	static void DebugPoolingSystem(const UObject* WorldContext, FLinearColor TextColor = FLinearColor::Blue, float Duration = 2, FName Key = NAME_None, bool Log = false);
};
