// Copyright � by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CharacterTypes.h"
#include "Core/PulseResourceManagement/Types/GameAssetTypes.h"
#include "Core/PulseResourceManagement/Types/BasePulseAsset.h"
#include "BasePulseCharacterAsset.generated.h"




/**
 * Serve as a base class for all Character game Data types
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseCharacterAsset : public UBasePulseAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_SPAWN, meta = (AssetBundles = BUNDLE_SPAWN))
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_SPAWN, meta = (AssetBundles = BUNDLE_SPAWN))
	TSoftClassPtr<class UAnimInstance> AnimBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_SPAWN, meta = (AssetBundles = BUNDLE_SPAWN))
	TSoftObjectPtr<UAnimMontage> SpawnMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BUNDLE_SPAWN, meta = (AssetBundles = BUNDLE_SPAWN))
	FCharacterBaseStats BaseCharacterStats; 
};
