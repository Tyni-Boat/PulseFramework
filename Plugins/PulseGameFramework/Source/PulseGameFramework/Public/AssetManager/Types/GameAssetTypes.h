// Copyright � by Tyni Boat. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "GameAssetTypes.generated.h"




#pragma region Enums

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = true))
enum class EPulseDataBundleType : uint8
{
	None = 0 UMETA(Hidden),
	Infos = 1 << 0 UMETA(ToolTip = "For things like names, stats and all simple types"),
	UI = 1 << 1 UMETA(ToolTip = "For managed types related to UI like icons, widgets"),
	Spawn = 1 << 2 UMETA(ToolTip = "For managed types related to placement in the world like meshes, materials or anim bp"),
	Accessories = 1 << 3 UMETA(ToolTip = "For other managed types"),
};
ENUM_CLASS_FLAGS(EPulseDataBundleType);

#pragma endregion Enums


#pragma region Structs

USTRUCT()
struct FPulseAssetId
{
	GENERATED_BODY()

	FPulseAssetId(){}
	FPulseAssetId(FPrimaryAssetId pId)
	{
		PrimaryAssetID = pId;
		AssetType = pId.PrimaryAssetType;
		FString lhs, rhs;
		if (!pId.PrimaryAssetName.ToString().Split("_", &lhs, &rhs))
			return;
		if (!FCString::IsNumeric(*rhs))
			return;
		Id = FCString::Atoi(*rhs);
	}

	UPROPERTY(EditAnywhere)
	FPrimaryAssetType AssetType = "";

	UPROPERTY(EditAnywhere)
	int32 Id = 0;

	UPROPERTY(EditAnywhere)
	FPrimaryAssetId PrimaryAssetID = {};

	FString ToString() const { return FString::Printf(TEXT("[%s-%d]"), *AssetType.ToString(), Id); }

	bool operator==(const FPulseAssetId& Other) const
	{
		return (AssetType == Other.AssetType && Id == Other.Id) || PrimaryAssetID == Other.PrimaryAssetID;
	}
};

FORCEINLINE uint32 GetTypeHash(const FPulseAssetId& Key)
{
	return HashCombine(HashCombine(GetTypeHash(Key.AssetType), GetTypeHash(Key.Id)), GetTypeHash(Key.PrimaryAssetID));
}

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


