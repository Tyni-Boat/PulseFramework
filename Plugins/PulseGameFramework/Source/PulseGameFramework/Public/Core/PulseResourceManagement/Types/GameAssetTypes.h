// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "GameAssetTypes.generated.h"




#pragma region Enums

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class EDataBundleType : uint8
{
	None = 0 UMETA(Hidden),
	Infos = 1 << 0 UMETA(ToolTip = "For things like names, stats and all simple types"),
	UI = 1 << 1 UMETA(ToolTip = "For managed types related to UI like icons, widgets"),
	Spawn = 1 << 2 UMETA(ToolTip = "For managed types related to placement in the world like meshes, materials or anim bp"),
	Accessories = 1 << 3 UMETA(ToolTip = "For other managed types"),
};
ENUM_CLASS_FLAGS(EDataBundleType);

#pragma endregion Enums


#pragma region Structs

USTRUCT(BlueprintType)
struct FAssetPack
{
	GENERATED_BODY()
public:
	inline FAssetPack(){}
	inline FAssetPack(TArray<UPrimaryDataAsset*>& AssetsList) : Assets(AssetsList){}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Asset)
	TArray<UPrimaryDataAsset*> Assets;
};

#pragma endregion Structs


#pragma region Macros

#define BUNDLE_INFOS "BUNDLE_INFOS"
#define BUNDLE_UI "BUNDLE_UI"
#define BUNDLE_SPAWN "BUNDLE_SPAWN" 
#define BUNDLE_ACCESSORY "BUNDLE_ACCESSORY"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssetLoaded, UPrimaryDataAsset*, Asset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMultipleAssetsLoaded, FAssetPack, AssetPack);

#pragma endregion Macros

#pragma region Classes
#pragma endregion Classes

