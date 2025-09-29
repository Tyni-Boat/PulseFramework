// Copyright � by Tyni Boat. All Rights Reserved.


#include "Core/PulseResourceManagement/AsyncLoadBatchGameAsset.h"
#include "Core/PulseResourceManagement/PulseAssetManager.h"
#include "Core/PulseResourceManagement/Types/BasePulseAsset.h"



UAsyncLoadBatchGameAsset* UAsyncLoadBatchGameAsset::LoadGameAssetBatch(UObject* WorldContextObject, const TSubclassOf<UBasePulseAsset> Type, const TArray<int32>& AssetIds,
                                                                       int32 flag)
{
	UAsyncLoadBatchGameAsset* Node = NewObject<UAsyncLoadBatchGameAsset>();
	Node->_worldContext = WorldContextObject;
	Node->_Type = Type;
	Node->_Ids = AssetIds;
	Node->_assetBundles = UPulseAssetManager::GetDataBundleFromFlags(flag);
	Node->OnAssetsLoaded.AddDynamic(Node, &UAsyncLoadBatchGameAsset::OnAllAssetLoaded_Internal);
	// Register with the game instance to avoid being garbage collected
	Node->RegisterWithGameInstance(WorldContextObject);
	return Node;
}

void UAsyncLoadBatchGameAsset::Activate()
{
	TMap<TSubclassOf<UBasePulseAsset>, TArray<int32>> Map;
	Map.Add(_Type, _Ids);
	if (!UPulseAssetManager::LoadMultipleAssets(Map, OnAssetsLoaded, _assetBundles))
		SetReadyToDestroy();
}

void UAsyncLoadBatchGameAsset::OnAllAssetLoaded_Internal(FAssetPack Assets)
{
	SetReadyToDestroy();
}