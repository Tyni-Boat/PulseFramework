// Copyright � by Tyni Boat. All Rights Reserved.


#include "Core/AssetManager/AsyncLoadBatchGameAsset.h"
#include "Core/AssetManager/PulseAssetManager.h"
#include "Core/AssetManager/Types/BasePulseAsset.h"



UAsyncLoadBatchGameAsset* UAsyncLoadBatchGameAsset::LoadGameAssetBatch(UObject* WorldContextObject, const TSubclassOf<UBasePulseAsset> Type, const TArray<int32>& AssetIds,
                                                                       int32 flag)
{
	UAsyncLoadBatchGameAsset* Node = NewObject<UAsyncLoadBatchGameAsset>();
	Node->_worldContext = WorldContextObject;
	Node->_Type = Type;
	Node->_Ids = AssetIds;
	Node->_assetBundles = UPulseBPAssetManager::GetDataBundleFromFlags(flag);
	Node->OnAssetsLoaded.AddDynamic(Node, &UAsyncLoadBatchGameAsset::OnAllAssetLoaded_Internal);
	// Register with the game instance to avoid being garbage collected
	Node->RegisterWithGameInstance(WorldContextObject);
	return Node;
}

void UAsyncLoadBatchGameAsset::Activate()
{
	if (!UPulseBPAssetManager::LoadMultipleAssets(_Type, _Ids, OnAssetsLoaded, _assetBundles))
		SetReadyToDestroy();
}

void UAsyncLoadBatchGameAsset::OnAllAssetLoaded_Internal(FAssetPack Assets)
{
	SetReadyToDestroy();
}