// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulsePoolingTypes.generated.h"


UENUM(BlueprintType)
enum class EPoolQueryResult : uint8
{
	None = 0 UMETA(DisplayName = "None"),
	Success = 1 UMETA(DisplayName = "Success"),
	InvalidParams = 2 UMETA(DisplayName = "Invalid Params"),
	PoolLimitReached = 3 UMETA(DisplayName = "The pool reached its limit"),
	BadOrNullObjectClass = 4 UMETA(DisplayName = "The Object Class Is Bad or Null"),
	UnableToTransferOwnership = 5 UMETA(DisplayName = "Cannot Transfer Ownership"),
	ProhibitedOnClient = 6 UMETA(DisplayName = "Prohibited On Client"),
	Undefined = 7 UMETA(DisplayName = "Unknow error happened")
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


USTRUCT(NotBlueprintType)
struct PULSEGAMEFRAMEWORK_API FPoolingTypeObjects
{
	GENERATED_BODY()

private:
	
	UPROPERTY()
	TArray<TObjectPtr<UObject>> ObjectSet;

public:
	
	FPoolingTypeObjects() { ObjectSet = {}; }

	FPoolingTypeObjects(TArray<UObject*> array)
	{
		ObjectSet.Empty();
		for (UObject* obj : array)
			ObjectSet.AddUnique(obj);
	}

	bool IsValid() const { return !ObjectSet.IsEmpty() && ObjectSet[0]; }

	int32 Count() const { return ObjectSet.Num(); }

	UObject* GetFirst() const
	{
		if (!IsValid())
			return nullptr;
		return ObjectSet[0].Get();
	}

	bool Contains(UObject* obj) const { return ObjectSet.Contains(obj); }

	bool Add(UObject* obj)
	{
		ObjectSet.AddUnique(obj);
		return true;
	}

	bool Remove(UObject* obj)
	{
		const int32 remCount = ObjectSet.Remove(obj);
		return remCount > 0;
	}

	bool Replace(UObject* Old, UObject* New, bool bAddIfNotExist = true)
	{
		if (!ObjectSet.Contains(Old))
		{
			if (!bAddIfNotExist)
				return false;
			if (!New)
				return false;
			ObjectSet.Add(New);
			return true;
		}
		ObjectSet.Remove(Old);
		if (New)
			ObjectSet.AddUnique(New);
		return true;
	}

	void Clean(bool bDestroy = false)
	{
		const auto array = ObjectSet;
		for (auto obj : array)
		{
			if (obj)
			{
				if (bDestroy)
				{
					if (auto asActor = Cast<AActor>(obj))
						asActor->Destroy();
					else if (auto asComponent = Cast<UActorComponent>(obj))
						asComponent->ConditionalBeginDestroy();
					else
						obj->MarkAsGarbage();
					ObjectSet.Remove(obj);
				}
				continue;
			}
			ObjectSet.Remove(obj);
		}
		if (bDestroy)
			ObjectSet.Empty();
	}

	void ForAllValid(TFunction<void(UObject*)> Action)
	{
		if (!Action)
			return;
		const auto array = ObjectSet;
		for (auto obj : array)
		{
			if (!obj)
				continue;
			Action(obj.Get());
		}
	}
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPulsePoolingEvent, TSubclassOf<UObject>, Type);