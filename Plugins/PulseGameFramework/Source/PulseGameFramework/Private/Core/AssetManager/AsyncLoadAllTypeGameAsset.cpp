// Copyright � by Tyni Boat. All Rights Reserved.



#include "Core/AssetManager/AsyncLoadAllTypeGameAsset.h"
#include "Core/AssetManager/PulseAssetManager.h"
#include "Core/AssetManager/Types/BasePulseAsset.h"



UAsyncLoadAllTypeGameAsset* UAsyncLoadAllTypeGameAsset::LoadGameAssetAllOfType(UObject* WorldContextObject, const TSubclassOf<UBasePulseAsset> Type, int32 flag)
{
	UAsyncLoadAllTypeGameAsset* Node = NewObject<UAsyncLoadAllTypeGameAsset>();
	Node->_worldContext = WorldContextObject;
	Node->_Type = Type;
	Node->_assetBundles = UPulseBPAssetManager::GetDataBundleFromFlags(flag);
	Node->OnAssetsLoaded.AddDynamic(Node, &UAsyncLoadAllTypeGameAsset::OnAllAssetLoaded_Internal);
	// Register with the game instance to avoid being garbage collected
	Node->RegisterWithGameInstance(WorldContextObject);
	return Node;
}

void UAsyncLoadAllTypeGameAsset::Activate()
{
	if (!UPulseBPAssetManager::LoadAllPulseAssets(_Type, OnAssetsLoaded, _assetBundles))
		SetReadyToDestroy();
}

void UAsyncLoadAllTypeGameAsset::OnAllAssetLoaded_Internal(FAssetPack Assets)
{
	SetReadyToDestroy();
}

