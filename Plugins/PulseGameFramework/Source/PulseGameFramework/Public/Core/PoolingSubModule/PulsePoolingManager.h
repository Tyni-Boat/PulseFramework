// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/CoreTypes.h"
#include "Core/PulseSubModuleBase.h"
#include "PulsePoolingManager.generated.h"


#pragma region SubTypes

UENUM(BlueprintType)
enum class EPoolQueryResult : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Success = 1 UMETA(DisplayName = "Success"),
	BadOrNullWorldContext = 2 UMETA(DisplayName = "The world context provided is Bad"),
	InvalidParams = 3 UMETA(DisplayName = "Invalid Params"),
	PoolLimitReached = 4 UMETA(DisplayName = "The pool reached its limit"),
	BadOrNullObjectClass = 5 UMETA(DisplayName = "The class query is Bad"),
	UnableToTransfertOwnership = 6 UMETA(DisplayName = "The class query is Bad"),
	Undefined = 7 UMETA(DisplayName = "Unknow error happened")
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UIPoolingObject : public UInterface
{
	GENERATED_BODY()
};


USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FPoolingParams
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<FTransform> TransformParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<FVector> VectorParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<FLinearColor> ColorParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<FRotator> RotationParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<float> ValueParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<FPrimaryAssetId> AssetParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<FName> NamesParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<TObjectPtr<UObject>> CustomParams;

	bool IsValid() const;
};


USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FPoolingTypeObjects
{
	GENERATED_BODY()

public:

	FPoolingTypeObjects() { ObjectArray = {}; }

	FPoolingTypeObjects(TArray<TObjectPtr<UObject>>  array) { ObjectArray = array; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pooling System")
	TArray<TObjectPtr<UObject>> ObjectArray;

	bool IsValid() const { return !ObjectArray.IsEmpty(); }
};

/**
 * Implement by any object that want to be pooled.
 */
class PULSEGAMEFRAMEWORK_API IIPoolingObject
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling System")
	bool OnPoolingObjectSpawned(const FPoolingParams SpawnData);

	virtual bool OnPoolingObjectSpawned_Implementation(const FPoolingParams SpawnData);


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling System")
	void OnPoolingObjectDespawned();

	virtual void OnPoolingObjectDespawned_Implementation();


	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pooling System")
	EPoolQueryResult ReturnToPool();

	virtual EPoolQueryResult ReturnToPool_Implementation();
};

#pragma endregion

/**
 * The Pulse Pooling System
 */
UCLASS(BlueprintType)
class PULSEGAMEFRAMEWORK_API UPulsePoolingManager : public UPulseSubModuleBase
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
public:
	
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
	
protected:

	// Function to create a new object of the specified class
	UObject* CreateNewObject(TSoftClassPtr<UObject> ObjectClass);

	// Function to move an object from the dormant pool to the live pool
	void MoveObjectToLivePool(TObjectPtr<UObject> Object);

	// Function to move an object from the live pool to the dormant pool
	void MoveObjectToDormantPool(TObjectPtr<UObject> Object);

	// Function to remove an now invalid objects from the pools
	void CleanUpPools(TSoftClassPtr<UObject> Class);

	UFUNCTION()
	void OnDestroyLinkedActor_Internal(AActor* Actor);

public:

	// Function to get an object from the pool. If the object is not found, it will create a new one and return it.
	// be aware that for actor component pooling, the world context will be use as the owner of the component
	// , so make sure to call this function from an actor or manually set the world context to the actor.
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext", AdvancedDisplay = 2, DeterminesOutputType="Class", DynamicOutputParam="Output"))
	static EPoolQueryResult GetObjectFromPool(UObject* WorldContext, TSubclassOf<UObject> Class, UObject*& Output, FPoolingParams QueryParams);

	// Register an existing object in the pool (live pool)
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext"))
	static void RegisterExistingObjectToPool(const UObject* WorldContext, UObject*& Object);

	// Preload objects of the specified class into the pool. Only the first array element of a set class are preloaded. Those are automatically set dormants
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext"))
	static void TryPreloadIntoPool(const UObject* WorldContext, TArray<TSoftClassPtr<UObject>> ObjectClasses, int32 CountPerObject = 5);

	// Function to return an object to the pool
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext"))
	static EPoolQueryResult ReturnObjectToPool(const UObject* WorldContext, UObject* Object);


	// Function to clear the pool form the specified class
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext"))
	static void ClearPoolType(const UObject* WorldContext, TSoftClassPtr<UObject> ObjectClass);

	// Function to clear the entire pool
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext"))
	static void ClearPool(const UObject* WorldContext);

	// Function to Limit the number of objects in the pool for a specific class. Setting the limit to <= 0 will remove the limit for that class.
	// Note: This will not affect the already existing objects in the pool, only the new ones created after setting the limit.
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext", AdvancedDisplay = 2))
	static void SetPoolLimitPerClass(const UObject* WorldContext, TSoftClassPtr<UObject> ObjectClass, int32 PoolLimit, ENumericOperator Operation = ENumericOperator::Set);

	// Function to Limit the number of objects in the pool for unspecified classes. Setting the limit to <= 0 will remove the limit..
	// Note: This will not affect the already existing objects in the pool, only the new ones created after setting the limit.
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext", AdvancedDisplay = 2))
	static void SetGlobalPoolLimit(const UObject* WorldContext, int32 PoolLimit, ENumericOperator Operation = ENumericOperator::Set);

	// Function debug the pooling system, it will print the current state of the pool to the screen. Use Log param to also print into the log
	UFUNCTION(BlueprintCallable, Category = "Pooling System", meta = (WorldContext = "WorldContext", AdvancedDisplay = 1))
	static void DebugPoolingSystem(const UObject* WorldContext, FLinearColor TextColor = FLinearColor::Blue, float Duration = 2, FName Key = NAME_None, bool Log = false);
	
};
