// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CoreTypes.generated.h"


#pragma region Enums

UENUM(BlueprintType)
enum class ELogicComparator : uint8
{
	Equal = 0 UMETA(DisplayName = "=="),
	NotEqual = 1 UMETA(DisplayName = "!="),
	GreaterThan = 2 UMETA(DisplayName = ">"),
	LessThan = 3 UMETA(DisplayName = "<"),
	GreaterThanOrEqualTo = 4 UMETA(DisplayName = ">="),
	LessThanOrEqualTo = 5 UMETA(DisplayName = "<=")
};

UENUM(BlueprintType)
enum class EBooleanOperator : uint8
{
	Not = 0 UMETA(DisplayName = "Invert logic"),
	Or = 1 UMETA(DisplayName = "One or the other"),
	And = 2 UMETA(DisplayName = "One and the other"),
	Xor = 3 UMETA(DisplayName = "Either one or the other"),
	Nor = 4 UMETA(DisplayName = "Inverted Or"),
	Nand = 5 UMETA(DisplayName = "Inverted And"),
	NXor = 6 UMETA(DisplayName = "Inverted Xor")
};


UENUM(BlueprintType)
enum class ENumericOperator : uint8
{
	Set = 0 UMETA(DisplayName = "->"),
	Add = 1 UMETA(DisplayName = "+"),
	Sub = 2 UMETA(DisplayName = "-"),
	Mul = 3 UMETA(DisplayName = "X"),
	Div = 4 UMETA(DisplayName = "/"),
	Mod = 5 UMETA(DisplayName = "%"),
};


#pragma endregion Enums


#pragma region Structs


USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FCodedOperation
{
	GENERATED_BODY()

public:
	FCodedOperation();

	virtual ~FCodedOperation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coded Operation")
	ELogicComparator Comparator = ELogicComparator::Equal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coded Operation")
	float AValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coded Operation")
	float BValue = 0;

	bool Evaluate() const;
};

#pragma endregion Structs

#pragma region Macros
#pragma endregion Macros

#pragma region Classes

UCLASS(Abstract)
class PULSEGAMEFRAMEWORK_API UBaseGameCondition : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	FCodedOperation ConditionParam;

	UFUNCTION(BlueprintPure, Category = "Condition")
	virtual bool EvaluateCondition(const int32 Code, bool bInvalidCodeFallbackResponse = false) const;
};

// Base class of buffs
UCLASS(Abstract)
class PULSEGAMEFRAMEWORK_API UBaseOperationModifier : public UObject
{
	GENERATED_BODY()

public:
	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OperationModifier", meta=(ClampMin=0, ClampMax=1, UIMin=0, UIMax=1))
	float ModificationPercentage = 0;

	UFUNCTION(BlueprintPure, Category = "Condition")
	virtual bool TryApplyModifier(const float InitialAmount, float& FinalAmount, UObject* FromObject = nullptr, UObject* ToObject = nullptr, FName Context = "") const;
};

#pragma endregion Classes
