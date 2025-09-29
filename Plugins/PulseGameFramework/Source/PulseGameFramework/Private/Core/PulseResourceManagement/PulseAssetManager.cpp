// Copyright � by Tyni Boat. All Rights Reserved.


#include "Core/PulseResourceManagement/PulseAssetManager.h"

#include "BlueprintActionDatabase.h"
#include "Core/PulseCoreModule.h"
#include "Core/PulseSystemLibrary.h"
#include "Engine/AssetManager.h"
#include "Core/PulseResourceManagement/Types/IIdentifiableActor.h"
#include "Core/PulseResourceManagement/Types/BasePulseAsset.h"


FName UPulseAssetManagerSubModule::GetSubModuleName() const
{
	return Super::GetSubModuleName();
}

bool UPulseAssetManagerSubModule::WantToTick() const
{
	return false;
}

bool UPulseAssetManagerSubModule::TickWhenPaused() const
{
	return false;
}

void UPulseAssetManagerSubModule::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	auto coreModule = Cast<UPulseCoreModule>(OwningModule);
	if (!coreModule)
		return;
	if (auto config = coreModule->GetProjectConfig())
	{
		// if (auto mgr = UAssetManager::GetIfInitialized())
		// {
		// 	for (const auto item : config->PrimaryAssetDirectoryMap)
		// 		mgr->ScanPathForPrimaryAssets(item.Key, item.Value.Path, UBasePulseAsset::StaticClass(), false, false, false);
		// }
	}
}

void UPulseAssetManagerSubModule::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
}

void UPulseAssetManagerSubModule::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}


TArray<FName> UPulseAssetManager::GetDataBundleFromFlags(int32 flag)
{
	TArray<FName> result;
	if (flag & static_cast<int32>(EDataBundleType::Infos))
		result.Add(BUNDLE_INFOS);
	if (flag & static_cast<int32>(EDataBundleType::UI))
		result.Add(BUNDLE_UI);
	if (flag & static_cast<int32>(EDataBundleType::Spawn))
		result.Add(BUNDLE_SPAWN);
	if (flag & static_cast<int32>(EDataBundleType::Accessories))
		result.Add(BUNDLE_ACCESSORY);
	return result;
}

bool UPulseAssetManager::TryGetObjectAssetID(UObject* Object, int32& OutID)
{
	if (!Object)
		return false;
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	auto assetID = AssetManager->GetPrimaryAssetIdForObject(Object);
	if (!assetID.IsValid())
		return false;
	TSoftClassPtr<> ptr;
	FName name;
	return TryGetPrimaryAssetInfos(assetID, ptr, name, OutID);
}

bool UPulseAssetManager::TryGetPrimaryAssetType(const TSubclassOf<UBasePulseAsset> Type, FPrimaryAssetType& OutAssetType)
{
	if (!Type)
		return false;
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	FString typeName;
	FString rest;
	FString path = Type->GetFName().ToString();
	if (!path.Split("_", &typeName, &rest))
		typeName = path;
	FPrimaryAssetType type = FPrimaryAssetType(FName(typeName));
	FPrimaryAssetTypeInfo typeInfos;
	if (!AssetManager->GetPrimaryAssetTypeInfo(type, typeInfos))
		return false;
	if (typeInfos.GetAssetBaseClass() != TSoftClassPtr(Type))
		return false;
	OutAssetType = type;
	return true;
}

bool UPulseAssetManager::TryGetClassAssetType(const FPrimaryAssetType& AssetType, TSoftClassPtr<UObject>& OutClass)
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

bool UPulseAssetManager::TryGetPrimaryAssetID(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FPrimaryAssetId& OutAssetID)
{
	if (!Type)
		return false;
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
		return false;
	FPrimaryAssetType type;
	if (!TryGetPrimaryAssetType(Type, type))
		return false;
	FString assetName = FString::Printf(TEXT("%s_%d"), *type.ToString(), Id);
	FPrimaryAssetId assetID = FPrimaryAssetId(type, FName(assetName));
	if (!assetID.IsValid())
		return false;
	FAssetData assetData;
	if (!AssetManager->GetPrimaryAssetData(assetID, assetData))
		return false;
	OutAssetID = assetID;
	return true;
}

bool UPulseAssetManager::TryGetPrimaryAssetInfos(FPrimaryAssetId AssetID, TSoftClassPtr<UObject>& OutClass, FName& OutAssetName, int32& OutAssetId)
{
	if (!AssetID.IsValid())
		return false;
	if (!TryGetClassAssetType(AssetID.PrimaryAssetType, OutClass))
		return false;
	FString name;
	FString id;
	if (!(AssetID.PrimaryAssetName).ToString().Split("_", &name, &id))
		return false;
	if (!id.IsNumeric())
		return false;
	OutAssetName = AssetID.PrimaryAssetName;
	OutAssetId = FCString::Atoi(*id);
	return true;
}

TArray<FPrimaryAssetId> UPulseAssetManager::GetAllGameAssets(const TSubclassOf<UBasePulseAsset> Type)
{
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		TArray<FPrimaryAssetId> assets;
		FPrimaryAssetType type;
		if (!TryGetPrimaryAssetType(Type, type))
			return {};
		mgr->GetPrimaryAssetIdList(type, assets);
		return assets;
	}
	return TArray<FPrimaryAssetId>();
}

bool UPulseAssetManager::LoadAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id, FOnAssetLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UAssetManager::GetIfInitialized();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(nullptr);
		return false;
	}
	FPrimaryAssetId assetID;
	if (!TryGetPrimaryAssetID(Type, Id, assetID))
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(nullptr);
		return false;
	}
	auto strCallBack = FStreamableDelegate::CreateLambda([CallBack, assetID]()-> void
	{
		if (auto mgr = UAssetManager::GetIfInitialized())
		{
			if (auto assetData = mgr->GetPrimaryAssetObject<UBasePulseAsset>(assetID))
			{
				if (CallBack.IsBound())
					CallBack.Broadcast(assetData);
				return;
			}
		}
		if (CallBack.IsBound())
			CallBack.Broadcast(nullptr);
	});
	mgr->LoadPrimaryAsset(assetID, Bundles, strCallBack);
	return true;
}

bool UPulseAssetManager::LoadAllAssets(const TSubclassOf<UBasePulseAsset> Type, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UAssetManager::GetIfInitialized();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	FPrimaryAssetType assetType;
	if (!TryGetPrimaryAssetType(Type, assetType))
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	auto allAssets = GetAllGameAssets(Type);
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

bool UPulseAssetManager::LoadMultipleAssets(const TMap<TSubclassOf<UBasePulseAsset>, TArray<int32>> TypeBatches, FOnMultipleAssetsLoaded& CallBack, const TArray<FName>& Bundles)
{
	auto mgr = UAssetManager::GetIfInitialized();
	if (!mgr)
	{
		if (CallBack.IsBound())
			CallBack.Broadcast(FAssetPack());
		return false;
	}
	if (TypeBatches.IsEmpty())
		return false;
	TArray<FPrimaryAssetId> allAssets;
	for (const auto& item : TypeBatches)
	{
		FPrimaryAssetType assetType;
		if (!TryGetPrimaryAssetType(item.Key, assetType))
			continue;
		for (int i = 0; i < item.Value.Num(); ++i)
		{
			FPrimaryAssetId assetId;
			if (!TryGetPrimaryAssetID(item.Key, item.Value[i], assetId))
				continue;
			allAssets.Add(assetId);
		}
	}
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
				{
					assets.Insert(assetData, 0);
				}
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


void UPulseAssetManager::TryUnloadGameAsset(const TSubclassOf<UBasePulseAsset> Type, const int32 Id)
{
	FPrimaryAssetId assetID;
	if (!TryGetPrimaryAssetID(Type, Id, assetID))
		return;
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		mgr->UnloadPrimaryAsset(assetID);
	}
}

void UPulseAssetManager::TryUnloadAllGameAssets(const TSubclassOf<UBasePulseAsset> Type)
{
	if (auto mgr = UAssetManager::GetIfInitialized())
	{
		FPrimaryAssetType type;
		if (!TryGetPrimaryAssetType(Type, type))
			return;
		mgr->UnloadPrimaryAssetsWithType(type);
	}
}

bool UPulseAssetManager::TryGetControlledCharacterID(const AController* Controller, FPrimaryAssetId& OutCharacterID)
{
	if (!Controller)
		return false;
	auto pawn = Controller->GetPawn();
	if (!pawn)
		return false;
	if (!pawn->Implements<UIIdentifiableActor>())
		return false;
	OutCharacterID = IIIdentifiableActor::Execute_GetID(pawn);
	return true;
}
