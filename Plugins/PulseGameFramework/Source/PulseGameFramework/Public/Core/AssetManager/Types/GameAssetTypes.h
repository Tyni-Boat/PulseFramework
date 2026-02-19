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

	UPROPERTY(EditAnywhere)
	FName AssetClassFName = "";

	UPROPERTY(EditAnywhere)
	int32 Id = 0;

	FString ToString() const { return FString::Printf(TEXT("[%s-%d]"), *AssetClassFName.ToString(), Id); }

	bool operator==(const FPulseAssetId& Other) const
	{
		return AssetClassFName == Other.AssetClassFName && Id == Other.Id;
	}
};

FORCEINLINE uint32 GetTypeHash(const FPulseAssetId& Key)
{
	return HashCombine(GetTypeHash(Key.AssetClassFName), GetTypeHash(Key.Id));
}

USTRUCT()
struct FPulseAssetsManifestEntry
{
	GENERATED_BODY()
public:	
	UPROPERTY()
	FPrimaryAssetId PrimaryAssetId = {};
	
	UPROPERTY()
	FVector Version = FVector::ZeroVector;
	
	UPROPERTY()
	bool bIsPlaceHolderAsset = false;

	inline bool Compare(FPulseAssetsManifestEntry Other) const
	{
		if (bIsPlaceHolderAsset != Other.bIsPlaceHolderAsset)
		{
			if (bIsPlaceHolderAsset && !Other.bIsPlaceHolderAsset)
				return true;
			if (!bIsPlaceHolderAsset && Other.bIsPlaceHolderAsset)
				return false;
		}
		if (Version == Other.Version)
			return false;
		if (Version.X != Other.Version.X)
			return Version.X < Other.Version.X;
		if (Version.Y != Other.Version.Y)
			return Version.Y < Other.Version.Y;
		else
			return Version.Z < Other.Version.Z;
	}
};

USTRUCT()
struct FPulseAssetsManifestEntryPack
{
	GENERATED_BODY()
public:	
	UPROPERTY()
	TArray<FPulseAssetsManifestEntry> EntryPack;
};

USTRUCT()
struct FPulseAssetsManifest
{
	GENERATED_BODY()
public:	
	UPROPERTY()
	TMap<FPulseAssetId, FPulseAssetsManifestEntry> AssetsRegistry;
};

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

UCLASS(BlueprintType, NotBlueprintable)
class UPulseLocalBakedAssetManifest : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPulseAssetsManifest AssetsManifest;
};

#pragma endregion Classes

