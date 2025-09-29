// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CharacterTypes.generated.h"


#pragma region Macros

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCharacterDataLoaded, AActor*, Character, FPrimaryAssetId, Id);

#pragma endregion Macros


#pragma region Enums


#pragma endregion Enums


#pragma region Structs


USTRUCT(BlueprintType)
struct FCharacterBaseStats
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth = 100;

	// Walk, run, sprint speeds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FVector OnGroundSpeeds = {250, 450, 600};
};

USTRUCT(BlueprintType)
struct FCharacterSavedDatas
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evolution")
	FCharacterBaseStats SavedStats;
};

#pragma endregion Structs


#pragma region Classes


class UBasePulseCharacterAsset;

UCLASS(Blueprintable, BlueprintType)
class UCharactersSavedObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = CharacterSave)
	TMap<FPrimaryAssetId, FCharacterSavedDatas> PerCharacterEvolution;

	UFUNCTION(BlueprintCallable, Category = CharacterSave, meta=(WorldContext="WorldContextObject"))
	static bool TryReadCharacterSavedDatas(UObject* WorldContextObject, TSubclassOf<UBasePulseCharacterAsset> Type, const int32 CharacterId, FCharacterSavedDatas& OutCharacterSaved,
	                                      const int32 SaveUserIndex = 0);

	UFUNCTION(BlueprintCallable, Category = CharacterSave, meta=(WorldContext="WorldContextObject"))
	static bool TryWriteCharacterSavedDatas(UObject* WorldContextObject, TSubclassOf<UBasePulseCharacterAsset> Type, const int32 CharacterId, const FCharacterSavedDatas CharacterSave,
	                                      const int32 SaveUserIndex = 0);
};

#pragma endregion Classes
