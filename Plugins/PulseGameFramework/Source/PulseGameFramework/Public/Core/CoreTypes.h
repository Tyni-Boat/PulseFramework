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
struct PULSEGAMEFRAMEWORK_API FUInt82
{
	GENERATED_BODY()

public:
	FUInt82(){}
	FUInt82(int x, int y): X(x), Y(y){}
	FUInt82(int a): X(a), Y(a){}
	uint8 Lenght() const { return FMath::Sqrt(static_cast<float>((X * X) + (Y * Y))); }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 X = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 Y = 0;
	
	bool operator==(const FUInt82& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};
FORCEINLINE uint32 GetTypeHash(const FUInt82& Data)
{
	return HashCombine(::GetTypeHash(Data.X), ::GetTypeHash(Data.Y));
}


USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUInt83
{
	GENERATED_BODY()

public:
	FUInt83(){}
	FUInt83(int x, int y, int z): X(x), Y(y), Z(z){}
	FUInt83(int a): X(a), Y(a), Z(a) {}
	FUInt83(FUInt82 A, int z = 0): X(A.X), Y(A.Y), Z(z) {}
	uint8 Lenght() const { return FMath::Sqrt(static_cast<float>((X * X) + (Y * Y) + (Z * Z))); }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 X = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 Y = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 Z = 0;
	
	bool operator==(const FUInt83& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z;
	}
};

FORCEINLINE uint32 GetTypeHash(const FUInt83& Data)
{
	return HashCombine(HashCombine(::GetTypeHash(Data.X), ::GetTypeHash(Data.Y)), GetTypeHash(Data.Z));
}

USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUInt84
{
	GENERATED_BODY()

public:
	FUInt84(){}
	FUInt84(int x, int y, int z, int w): X(x), Y(y), Z(z), W(w) {}
	FUInt84(int a): X(a), Y(a), Z(a), W(a) {}
	FUInt84(FUInt82 A): X(A.X), Y(A.Y), Z(0) {}
	FUInt84(FUInt82 A, FUInt82 B): X(A.X), Y(A.Y), Z(B.X), W(B.Y) {}
	FUInt84(FUInt83 A, int w = 0): X(A.X), Y(A.Y), Z(A.Z), W(w) {}
	uint8 Lenght() const { return FMath::Sqrt(static_cast<float>((X * X) + (Y * Y) + (Z * Z) + (W * W))); }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 X = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 Y = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 Z = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UInt2")
	uint8 W = 0;
	
	bool operator==(const FUInt84& Other) const
	{
		return X == Other.X && Y == Other.Y && Z == Other.Z && W == Other.W;
	}
};
FORCEINLINE uint32 GetTypeHash(const FUInt84& Data)
{
	return HashCombine(HashCombine(::GetTypeHash(Data.X), ::GetTypeHash(Data.Y)), HashCombine(::GetTypeHash(Data.Z), ::GetTypeHash(Data.W)));
}

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
