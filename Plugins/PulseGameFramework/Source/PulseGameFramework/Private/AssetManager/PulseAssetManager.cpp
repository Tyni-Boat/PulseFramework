// Copyright � by Tyni Boat. All Rights Reserved.


#include "AssetManager/PulseAssetManager.h"

#include "HAL/PlatformFilemanager.h"
#include "Core/PulseSystemLibrary.h"
#include "Engine/AssetManager.h"
#include "AssetManager/Types/IIdentifiableActor.h"
#include "AssetManager/Types/BasePulseAsset.h"
#include "PulseGameFramework.h"


void UPulseAssetManager::OnPostAssetAdded(const FAssetData& AssetData)
{
	FPrimaryAssetId id = AssetData.GetPrimaryAssetId();
	if (!id.IsValid())
		return;
	AssetSet.Add(id);
}

void UPulseAssetManager::OnPostAssetRemoved(const FAssetData& AssetData)
{
	FPrimaryAssetId id = AssetData.GetPrimaryAssetId();
	if (!id.IsValid())
		return;
	AssetSet.Remove(id);
}

void UPulseAssetManager::OnPostAssetUpdated(TArrayView<const FAssetData> AssetDatas)
{
	for (const auto& AssetData : AssetDatas)
	{
		FPrimaryAssetId id = AssetData.GetPrimaryAssetId();
		if (!id.IsValid())
			return;
		AssetSet.Add(id);
	}
}

void UPulseAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Pulse Asset Manager Initialization Started"));

	// Fill the asset check set
	TArray<FPrimaryAssetTypeInfo> AssetTypes;
	GetPrimaryAssetTypeInfoList(AssetTypes);
	TArray<FPrimaryAssetId> AssetIds;
	for (const FPrimaryAssetTypeInfo& AssetType : AssetTypes)
	{
		const auto Type = FPrimaryAssetType(AssetType.PrimaryAssetType);
		if (!Type.IsValid())
			continue;
		AssetIds.Reset();
		if (GetPrimaryAssetIdList(Type, AssetIds))
		{
			for (const auto& Id : AssetIds)
			{
				AssetSet.Add(Id);
			}
		}
	}

	// Register for asset registry changes
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().OnAssetAdded().AddUObject(this, &UPulseAssetManager::OnPostAssetAdded);
	ARM.Get().OnAssetRemoved().AddUObject(this, &UPulseAssetManager::OnPostAssetRemoved);
	ARM.Get().OnAssetsUpdated().AddUObject(this, &UPulseAssetManager::OnPostAssetUpdated);
	UE_LOG(LogPulseAssetManagement, Log, TEXT("Pulse Asset Manager Initialization Completed"));
}

bool UPulseAssetManager::AssetExist(const FPrimaryAssetId Id) const
{
	if (AssetSet.Contains(Id))
		return true;
	FAssetData data;
	return GetPrimaryAssetData(Id, data);
}

bool UPulseAssetManager::QueryPrimaryAssetID(const FPrimaryAssetType& Type, const int32& Id, FPrimaryAssetId& OutAssetID) const
{
	const auto pAssetID = FPrimaryAssetId(
		*FString::Printf(TEXT("%s"), *Type.ToString()),
		*FString::Printf(TEXT("%s_%d"), *Type.ToString(), Id));
	if (AssetExist(pAssetID))
	{
		OutAssetID = pAssetID;
		return true;
	}
	return false;
}

bool UPulseAssetManager::QueryPulseAssetID(const FPrimaryAssetId& PrimaryId, FPulseAssetId& OutAssetID) const
{
	if (AssetExist(PrimaryId))
	{
		OutAssetID = PrimaryId;
		return true;
	}
	return false;
}


void UPulseAssetManager::AsyncLoadPulseAssets(const TSubclassOf<UBasePulseAsset> Class, const TSet<int32>& Ids, const TArray<FName>& LoadBundles,
                                              TFunction<void(TArray<UPrimaryDataAsset*>&)> OnSuccess, TFunction<void()> OnFailed)
{
	if (!Class)
	{
		if (OnFailed != nullptr)
			OnFailed();
		return;
	}
	if (Ids.IsEmpty())
	{
		if (OnFailed != nullptr)
			OnFailed();
		return;
	}
	TArray<int32> IdList = Ids.Array();
	const FName className = Class->GetFName();
	TMap<FPrimaryAssetId, int32> IndexMap;
	TMap<FPrimaryAssetId, TObjectPtr<UBasePulseAsset>> AssetMap;
	// Fill assets ids
	for (int i = 0; i < IdList.Num(); ++i)
	{
		FPrimaryAssetId assetID;
		if (QueryPrimaryAssetID(className, IdList[i], assetID))
			UPulseSystemLibrary::MapAddOrUpdateValue(IndexMap, assetID, i);
	}
	// No need to continue if nothing in Index map
	if (IndexMap.IsEmpty())
	{
		if (OnFailed != nullptr)
			OnFailed();
		return;
	}
	// Prepare delegate
	FStreamableDelegate streamableDelegate;
	streamableDelegate.BindLambda([this, IndexMap, OnSuccess, OnFailed]()-> void
	{
		TArray<UPrimaryDataAsset*> _assets;
		UPrimaryDataAsset* nullItem = nullptr;
		UPulseSystemLibrary::ArrayMatchSize(_assets, IndexMap.Num(), true, nullItem);
		TArray<FPrimaryAssetId> _assetIds;
		IndexMap.GetKeys(_assetIds);
		bool atLeastOne = false;
		for (const auto& assetPair : IndexMap)
		{
			if (auto asset = Cast<UPrimaryDataAsset>(GetPrimaryAssetObject(assetPair.Key)))
			{
				const auto& index = IndexMap[assetPair.Key];
				if (!_assets.IsValidIndex(index))
					continue;
				atLeastOne = true;
				_assets[index] = asset;
			}
		}
		if (!atLeastOne)
		{
			if (OnFailed != nullptr)
				OnFailed();
			return;
		}
		if (OnSuccess)
			OnSuccess(_assets);
	});

	// Load assets
	TArray<FPrimaryAssetId> assetIds;
	IndexMap.GetKeys(assetIds);
	LoadPrimaryAssets(assetIds, LoadBundles, streamableDelegate);
}

void UPulseAssetManager::BeginDestroy()
{
	Super::BeginDestroy();
}


TArray<FName> UPulseBPAssetManager::GetDataBundleFromFlags(int32 flag)
{
	TArray<FName> result;
	if (flag & static_cast<int32>(EPulseDataBundleType::Infos))
		result.Add(BUNDLE_INFOS);
	if (flag & static_cast<int32>(EPulseDataBundleType::UI))
		result.Add(BUNDLE_UI);
	if (flag & static_cast<int32>(EPulseDataBundleType::Spawn))
		result.Add(BUNDLE_SPAWN);
	if (flag & static_cast<int32>(EPulseDataBundleType::Accessories))
		result.Add(BUNDLE_ACCESSORY);
	return result;
}

bool UPulseBPAssetManager::GetAssetPulseID(UObject* Object, int32& OutID)
{
	if (!Object)
		return false;
	auto AssetManager = UPulseAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	auto assetID = AssetManager->GetPrimaryAssetIdForObject(Object);
	if (!assetID.IsValid())
		return false;
	TSoftClassPtr<> ptr;
	FName name;
	return GetPulseAssetInfos(assetID, ptr, name, OutID);
}

bool UPulseBPAssetManager::GetClassPrimaryAssetType(const TSubclassOf<UBasePulseAsset> Type, FPrimaryAssetType& OutAssetType)
{
	if (!Type)
		return false;
	auto AssetManager = UPulseAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	FPrimaryAssetType type = FPrimaryAssetType(Type->GetFName());
	FPrimaryAssetTypeInfo typeInfos;
	if (!AssetManager->GetPrimaryAssetTypeInfo(type, typeInfos))
		return false;
	if (typeInfos.GetAssetBaseClass() != TSoftClassPtr(Type))
		return false;
	OutAssetType = type;
	return true;
}

bool UPulseBPAssetManager::GetPrimaryTypeClass(const FPrimaryAssetType& AssetType, TSoftClassPtr<UObject>& OutClass)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	FPrimaryAssetTypeInfo typeInfos;
	if (!AssetManager->GetPrimaryAssetTypeInfo(AssetType, typeInfos))
		return false;
	OutClass = typeInfos.GetAssetBaseClass();
	return true;
}

bool UPulseBPAssetManager::GetPrimaryAssetID(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FPrimaryAssetId& OutAssetID)
{
	if (!Type)
		return false;
	auto AssetManager = UPulseAssetManager::Get();
	if (!AssetManager)
		return false;
	FPrimaryAssetType pType;
	if (!GetClassPrimaryAssetType(Type, pType))
		return false;
	return AssetManager->QueryPrimaryAssetID(pType, Id, OutAssetID);
}

bool UPulseBPAssetManager::GetPulseAssetInfos(const FPrimaryAssetId& AssetID, TSoftClassPtr<UObject>& OutClass, FName& OutAssetName, int32& OutAssetId)
{
	if (!AssetID.IsValid())
		return false;
	auto AssetManager = UPulseAssetManager::Get();
	if (!AssetManager)
		return false;
	FPulseAssetId PulseAssetID = {};
	if (!AssetManager->QueryPulseAssetID(AssetID, PulseAssetID))
		return false;
	FAssetData AssetData;
	if (!AssetManager->GetPrimaryAssetData(AssetID, AssetData))
		return false;
	OutClass = AssetData.GetClass(EResolveClass::Yes);
	OutAssetName = AssetData.AssetName;
	OutAssetId = PulseAssetID.Id;
	return true;
}

TArray<FPrimaryAssetId> UPulseBPAssetManager::GetAllAssetsOfType(const TSubclassOf<UObject> Type)
{
	if (!Type)
		return {};
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> assets;
		FPrimaryAssetType type = FPrimaryAssetType(Type->GetFName());
		mgr->GetPrimaryAssetIdList(type, assets);
		return assets;
	}
	return {};
}

bool UPulseBPAssetManager::IsAssetLoaded(const FPrimaryAssetId& AssetID)
{
	auto mgr = UPulseAssetManager::Get();
	if (!mgr)
		return false;
	return mgr->GetPrimaryAssetObject(AssetID) != nullptr;
}

bool UPulseBPAssetManager::LoadPulseAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FOnAssetLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UPulseAssetManager::Get();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(nullptr);
		return false;
	}
	mgr->AsyncLoadPulseAssets(Type, {Id}, Bundles, [CallBack](TArray<UPrimaryDataAsset*>& Result)-> void
	                          {
		                          //On success
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(!Result.IsEmpty() ? Result[0] : nullptr);
	                          }, [CallBack]()-> void
	                          {
		                          // On failure
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(nullptr);
	                          });
	return true;
}

bool UPulseBPAssetManager::LoadAllPulseAssets(const TSubclassOf<UBasePulseAsset> Type, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UAssetManager::GetIfInitialized();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	auto allAssets = GetAllAssetsOfType(Type);
	if (allAssets.IsEmpty())
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	auto strCallBack = FStreamableDelegate::CreateLambda([CallBack, allAssets]()-> void
	{
		if (auto mgr = UAssetManager::GetIfInitialized())
		{
			TArray<UPrimaryDataAsset*> assets;
			for (const auto& assetId : allAssets)
			{
				if (auto assetData = mgr->GetPrimaryAssetObject<UBasePulseAsset>(assetId))
					assets.Insert(assetData, 0);
			}
			if (CallBack.IsBound())
				CallBack.Broadcast(FAssetPack(assets));
			return;
		}
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
	});
	mgr->LoadPrimaryAssets(allAssets, Bundles, strCallBack);
	return true;
}

bool UPulseBPAssetManager::LoadMultipleAssets(const TSubclassOf<UBasePulseAsset> Type, TSet<int32> Ids, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UPulseAssetManager::Get();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	mgr->AsyncLoadPulseAssets(Type, Ids, Bundles, [CallBack](TArray<UPrimaryDataAsset*>& Result)-> void
	                          {
		                          //On success
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(FAssetPack(Result));
	                          }, [CallBack]()-> void
	                          {
		                          // On failure
		                          if (CallBack.IsBound())
			                          CallBack.Broadcast(FAssetPack());
	                          });
	return true;
}


void UPulseBPAssetManager::UnloadGameAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id)
{
	FPrimaryAssetId assetID;
	if (!GetPrimaryAssetID(Type, Id, assetID))
		return;
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		mgr->UnloadPrimaryAsset(assetID);
	}
}

void UPulseBPAssetManager::UnloadAllGameAssets(const TSubclassOf<UBasePulseAsset> Type)
{
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		FPrimaryAssetType type;
		if (!GetClassPrimaryAssetType(Type, type))
			return;
		mgr->UnloadPrimaryAssetsWithType(type);
	}
}

bool UPulseBPAssetManager::GetActorAssetID(AActor* Actor, FPrimaryAssetId& OutActorID)
{
	if (!Actor)
		return false;
	if (!Actor->Implements<UIIdentifiableActor>())
		return false;
	OutActorID = IIIdentifiableActor::Execute_GetID(Actor);
	return true;
}
