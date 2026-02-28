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
	BadOrNullObjectClass = 4 UMETA(DisplayName = "The class query is Bad"),
	UnableToTransferOwnership = 5 UMETA(DisplayName = "The class query is Bad"),
	ProhibitedOnClient = 6 UMETA(DisplayName = "The class query is Bad"),
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