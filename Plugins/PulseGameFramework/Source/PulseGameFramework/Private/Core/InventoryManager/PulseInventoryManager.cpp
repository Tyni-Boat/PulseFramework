// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Core/InventoryManager/PulseInventoryManager.h"

#include "Core/PulseCoreModule.h"
#include "Core/PulseResourceManagement/Types/IIdentifiableActor.h"
#include "GameFramework/PlayerState.h"

UInventoryComponent::UInventoryComponent()
{
}

FInventoryContainer* UInventoryComponent::GetInventoryPtr()
{
	int32 playerId = -1;
	if (auto plc = UGameplayStatics::GetPlayerController(this, LocalPlayerID))
		if (auto pls = plc->GetPlayerState<APlayerState>())
			playerId = pls->GetPlayerId();
	FPrimaryAssetId assetID = bUseCustomID ? CustomAssetID : (GetOwner()->Implements<UIIdentifiableActor>() ? IIIdentifiableActor::Execute_GetID(GetOwner()) : FPrimaryAssetId());
	AActor* actor = GetOwner();

	switch (InventoryType)
	{
	case EInventoryContainerType::Local: return GetLocalInventoryPtr();
	case EInventoryContainerType::Player: return UPulseInventoryManager::GetPlayerContainerPtr(this, playerId);
	case EInventoryContainerType::IDObject: return UPulseInventoryManager::GetAssetContainerPtr(this, assetID);
	case EInventoryContainerType::Actor: return UPulseInventoryManager::GetActorContainerPtr(this, actor);
	}
	return nullptr;
}

FInventoryContainer* UInventoryComponent::GetLocalInventoryPtr()
{
	return &_localContainer;
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

bool UInventoryComponent::GetInventory(uint8 Type, FGuid& OutInventoryID)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtrByType(Type))
		{
			return FGuid::Parse(inv->UID, OutInventoryID);
		}
	}
	return false;
}

bool UInventoryComponent::GetItemSlot(FGuid InventoryID, FPrimaryAssetId ItemID, FUInt84& OutLocationAndOccupation)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			return inv->GetItemSlot(ItemID, OutLocationAndOccupation);
		}
	}
	return false;
}

bool UInventoryComponent::GetAllFreeSlots(FGuid InventoryID, TArray<FUInt82>& OutFreeSlots)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			return inv->GetAllFreeSlots(OutFreeSlots);
		}
	}
	return false;
}

bool UInventoryComponent::CanFitItem(FGuid InventoryID, const FUInt82& ItemLocation, const FUInt82& ItemOccupation)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			return inv->CanFitItem(ItemLocation, ItemOccupation);
		}
	}
	return false;
}

bool UInventoryComponent::IsValidInventory(FGuid InventoryID)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			return inv->IsValidInventory();
		}
	}
	return false;
}

bool UInventoryComponent::GetItemInfos(FGuid InventoryID, FPrimaryAssetId ItemID, FInventoryItem& OutItem)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			return inv->GetItemInfos(ItemID, OutItem);
		}
	}
	return false;
}

bool UInventoryComponent::GetPossibleItemMovements(FGuid InventoryID, const FInventoryItem& Item, TArray<FUInt82>& OutLocations)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			return inv->GetPossibleMovements(Item, OutLocations);
		}
	}
	return false;
}


bool UInventoryComponent::ChangeContainerType(EInventoryContainerType newType, bool bCustomId, FPrimaryAssetId CustomId)
{
	if (newType == InventoryType)
	{
		if (newType != EInventoryContainerType::IDObject)
			return false;
		if (bCustomId == bUseCustomID)
			return false;
	}
	InventoryType = newType;
	bCustomId = bUseCustomID;
	CustomAssetID = CustomId;
	if (auto invMgr = UPulseInventoryManager::Get(this))
	{
		auto invPtr = GetInventoryPtr();
		invMgr->OnInventoryContainerChanged.Broadcast(this, invPtr ? *invPtr : FInventoryContainer());
	}
	return true;
}

bool UInventoryComponent::AddInventory(TSubclassOf<UBasePulseBaseItemAsset> ItemType, uint8 Type, uint8 Columns, uint8 Rows, bool bDecayableItemInventory)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (invPtr->InventoryPerType.Contains(Type))
			return false;
		invPtr->InventoryPerType.Add(Type, FInventory(FGuid::NewGuid()).OfSize(Columns, Rows).WithDecayableItems(bDecayableItemInventory).WithTypeRestriction(ItemType));
		if (auto invMgr = UPulseInventoryManager::Get(this))
		{
			invMgr->OnInventoryContainerChanged.Broadcast(this, *invPtr);
		}
	}
	return false;
}

bool UInventoryComponent::RemoveInventory(uint8 Type)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (!invPtr->InventoryPerType.Contains(Type))
			return false;
		invPtr->InventoryPerType.Remove(Type);
		if (auto invMgr = UPulseInventoryManager::Get(this))
		{
			invMgr->OnInventoryContainerChanged.Broadcast(this, *invPtr);
		}
	}
	return false;
}

bool UInventoryComponent::AddItem(FGuid InventoryID, FInventoryItem Item, int Quantity, bool bAttemptReorganisation)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->AddItem(Item, Quantity, bAttemptReorganisation))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}

bool UInventoryComponent::ConsumeItem(FGuid InventoryID, FPrimaryAssetId ItemID, int Quantity, int32& OutConsumedQuantity)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->ConsumeItem(ItemID, Quantity, OutConsumedQuantity))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}

bool UInventoryComponent::RemoveItem(FGuid InventoryID, FPrimaryAssetId ItemID)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->RemoveItem(ItemID))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}

bool UInventoryComponent::DecayItem(FGuid InventoryID, FPrimaryAssetId ItemID, float DecayAmount, int32 DecayItemIndex)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->DecayItem(ItemID, DecayAmount, DecayItemIndex))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}

void UInventoryComponent::DecayAllItems(FGuid InventoryID, float DecayAmount, int32 DecayItemIndex)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			inv->DecayAllItems(DecayAmount, DecayItemIndex);
			if (auto invMgr = UPulseInventoryManager::Get(this))
			{
				invMgr->OnInventoryChanged.Broadcast(*inv);
			}
		}
	}
}

bool UInventoryComponent::SetInventorySize(FGuid InventoryID, uint8 Columns, uint8 Rows, bool bPrioritizeInventorySize)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->SetInventorySize(Columns, Rows, bPrioritizeInventorySize))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}

bool UInventoryComponent::MoveItem(FGuid InventoryID, const FPrimaryAssetId& ItemID, const FUInt82 NewLocation, bool bMoveOverlappingItems)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->MoveItem(ItemID, NewLocation, bMoveOverlappingItems))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}

bool UInventoryComponent::CombineItems(FGuid InventoryID, const TArray<FInventoryItemCombineIngredient>& Ingredients, const FInventoryItem& ResultItem, uint8 ResultQty)
{
	if (auto invPtr = GetInventoryPtr())
	{
		if (auto inv = invPtr->GetInventoryPtr(InventoryID))
		{
			if (inv->CombineItems(Ingredients, ResultItem, ResultQty))
			{
				if (auto invMgr = UPulseInventoryManager::Get(this))
				{
					invMgr->OnInventoryChanged.Broadcast(*inv);
				}
			}
		}
	}
	return false;
}



void UPulseInventoryManager::AffectBoundInventories(FInventory ChangedInventory)
{
	TArray<FInventory*> BoundInventoryPtrs;
	if (GetInventories([ChangedInventory](const FInventory& Inventory)->bool{ return Inventory.BoundInventoryUID == ChangedInventory.UID; }, BoundInventoryPtrs))
	{
		for (int i = 0; i < BoundInventoryPtrs.Num(); i++)
		{
			if (BoundInventoryPtrs[i])
			{
				BoundInventoryPtrs[i]->ItemTypeRestriction = ChangedInventory.ItemTypeRestriction;
				BoundInventoryPtrs[i]->bIsDecayableItemInventory = ChangedInventory.bIsDecayableItemInventory;
				BoundInventoryPtrs[i]->InventorySlots = ChangedInventory.InventorySlots;
				BoundInventoryPtrs[i]->Items = ChangedInventory.Items;
			}
		}
	}
}

UPulseInventoryManager* UPulseInventoryManager::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;
	const auto gi = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (!gi)
		return nullptr;
	const auto core = gi->GetSubsystem<UPulseCoreModule>();
	if (!core)
		return nullptr;
	return core->GetSubModule<UPulseInventoryManager>();
}

FInventoryContainer* UPulseInventoryManager::GetPlayerContainerPtr(const UObject* WorldContextObject, const int32 playerID)
{
	if (auto instance = Get(WorldContextObject))
		return instance->_PlayersInventories.Contains(playerID)? &instance->_PlayersInventories[playerID] : nullptr;
	return nullptr;
}

FInventoryContainer* UPulseInventoryManager::GetAssetContainerPtr(const UObject* WorldContextObject, const FPrimaryAssetId& AssetID)
{
	if (auto instance = Get(WorldContextObject))
		return instance->_AssetInventories.Contains(AssetID)? &instance->_AssetInventories[AssetID] : nullptr;
	return nullptr;
}

FInventoryContainer* UPulseInventoryManager::GetActorContainerPtr(const UObject* WorldContextObject, const AActor* Actor)
{
	if (auto instance = Get(WorldContextObject))
		return instance->_ActorInventories.Contains(Actor)? &instance->_ActorInventories[Actor] : nullptr;
	return nullptr;
}

bool UPulseInventoryManager::GetInventories(std::function<bool(const FInventory&)> SelectFunc, TArray<FInventory*>& OutInventoryPtrs)
{
	if (!SelectFunc)
		return false;
	OutInventoryPtrs.Empty();
	for (auto pair: _PlayersInventories)
		for (auto s_pair: pair.Value.InventoryPerType)
			if (SelectFunc(_PlayersInventories[pair.Key].InventoryPerType[s_pair.Key]))
				OutInventoryPtrs.Add(&_PlayersInventories[pair.Key].InventoryPerType[s_pair.Key]);
	for (auto pair: _AssetInventories)
		for (auto s_pair: pair.Value.InventoryPerType)
			if (SelectFunc(_AssetInventories[pair.Key].InventoryPerType[s_pair.Key]))
				OutInventoryPtrs.Add(&_AssetInventories[pair.Key].InventoryPerType[s_pair.Key]);
	for (auto pair: _ActorInventories)
		for (auto s_pair: pair.Value.InventoryPerType)
			if (SelectFunc(_ActorInventories[pair.Key].InventoryPerType[s_pair.Key]))
				OutInventoryPtrs.Add(&_ActorInventories[pair.Key].InventoryPerType[s_pair.Key]);
	return OutInventoryPtrs.Num() > 0;
}

FName UPulseInventoryManager::GetSubModuleName() const
{
	return Super::GetSubModuleName();
}

bool UPulseInventoryManager::WantToTick() const
{
	return false;
}

bool UPulseInventoryManager::TickWhenPaused() const
{
	return false;
}

void UPulseInventoryManager::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	OnInventoryChanged.AddDynamic(this, &UPulseInventoryManager::AffectBoundInventories);
	if (auto coreModule = Cast<UPulseCoreModule>(OwningModule))
	{
		if (auto config = coreModule->GetProjectConfig())
		{
			_bCanReplicate = config->bReplicateInventory;
			_bCanSave = config->bSaveInventory;
		}
	}
}

void UPulseInventoryManager::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
	OnInventoryChanged.RemoveDynamic(this, &UPulseInventoryManager::AffectBoundInventories);
}

void UPulseInventoryManager::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}

bool UPulseInventoryManager::BindInventories(FGuid SourceInventoryID, FGuid TargetInventoryID)
{
	if (!SourceInventoryID.IsValid() || !TargetInventoryID.IsValid())
		return false;
	TArray<FInventory*> MatchingTargetInventoryPtrs;
	if (GetInventories([TargetInventoryID](const FInventory& Inventory)->bool{ return Inventory.UID == TargetInventoryID.ToString(); }, MatchingTargetInventoryPtrs))
	{
		bool didMatch = false;
		for (int i = 0; i < MatchingTargetInventoryPtrs.Num(); i++)
		{
			if (MatchingTargetInventoryPtrs[i])
			{
				MatchingTargetInventoryPtrs[i]->BoundInventoryUID = SourceInventoryID.ToString();
				didMatch = true;
			}
		}
		return didMatch;
	}
	return false;
}

bool UPulseInventoryManager::UnbindInventories(FGuid SourceInventoryID)
{
	if (!SourceInventoryID.IsValid())
		return false;
	TArray<FInventory*> MatchingTargetInventoryPtrs;
	if (GetInventories([SourceInventoryID](const FInventory& Inventory)->bool{ return Inventory.BoundInventoryUID == SourceInventoryID.ToString(); }, MatchingTargetInventoryPtrs))
	{
		bool didUnmatch = false;
		for (int i = 0; i < MatchingTargetInventoryPtrs.Num(); i++)
		{
			if (MatchingTargetInventoryPtrs[i])
			{
				MatchingTargetInventoryPtrs[i]->BoundInventoryUID = "";
				didUnmatch = true;
			}
		}
		return didUnmatch;
	}
	return false;
}
