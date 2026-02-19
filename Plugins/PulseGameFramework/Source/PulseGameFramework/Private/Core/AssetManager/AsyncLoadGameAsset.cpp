// Copyright � by Tyni Boat. All Rights Reserved.


#include "Core/AssetManager/AsyncLoadGameAsset.h"
#include "Core/AssetManager/PulseAssetManager.h"
#include "Core/AssetManager/Types/BasePulseAsset.h"



UAsyncLoadGameAsset* UAsyncLoadGameAsset::LoadGameAsset(UObject* WorldContextObject, const TSubclassOf<UBasePulseAsset> Type, const int32 Id, int32 flag)
{
	UAsyncLoadGameAsset* Node = NewObject<UAsyncLoadGameAsset>();
	Node->_worldContext = WorldContextObject;
	Node->_Type = Type;
	Node->_ID = Id;
	Node->_assetBundles = UPulseBPAssetManager::GetDataBundleFromFlags(flag);
	Node->OnAssetLoaded.AddDynamic(Node, &UAsyncLoadGameAsset::OnAssetLoaded_Internal);
	// Register with the game instance to avoid being garbage collected
	Node->RegisterWithGameInstance(WorldContextObject);
	return Node;
}

void UAsyncLoadGameAsset::Activate()
{
	if (!UPulseBPAssetManager::LoadPulseAsset(_Type, _ID, OnAssetLoaded, _assetBundles))
		SetReadyToDestroy();
}

void UAsyncLoadGameAsset::OnAssetLoaded_Internal(UPrimaryDataAsset* Asset)
{
	SetReadyToDestroy();
}