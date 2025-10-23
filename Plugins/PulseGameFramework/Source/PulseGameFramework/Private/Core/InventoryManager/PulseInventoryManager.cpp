// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Core/InventoryManager/PulseInventoryManager.h"

#include "Core/PulseCoreModule.h"
#include "Core/PulseResourceManagement/Types/IIdentifiableActor.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

UInventoryComponent::UInventoryComponent()
{
}

int32 UInventoryComponent::GetControllerLocalID() const
{
	if (!GetOwner())
		return -1;
	if (const auto asPawn = Cast<APawn>(GetOwner()))
	{
		if (!asPawn->GetController())
			return -1;
		if (!asPawn->GetController()->IsLocalPlayerController())
			return -1;
		if (const auto plCtrl = Cast<APlayerController>(asPawn->GetController()))
		{
			return UGameplayStatics::GetPlayerControllerID(plCtrl);
		}
	}
	return -1;
}

FPrimaryAssetId UInventoryComponent::GetAssetID() const
{
	if (!GetOwner())
		return {};
	if (GetOwner()->Implements<UIIdentifiableActor>())
	{
		return IIIdentifiableActor::Execute_GetID(GetOwner());
	}
	return {};
}

FGuid UInventoryComponent::GetContainerUID() const
{
	if (auto mgr = UPulseInventoryManager::Get(this))
	{
		FGuid uid;
		if (mgr->GetContainerUID(InventoryType, uid, GetControllerLocalID(), GetAssetID(), GetOwner()))
			return uid;
	}
	return {};
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

bool UInventoryComponent::GetInventory(uint8 Type, FInventory& OutInventory)
{
	OutInventory = {};
	FInventoryContainer* containerPtr = UPulseInventoryManager::GetContainerPtr(this, GetContainerUID());
	if (!containerPtr)
		return false;
	if (auto inv = containerPtr->GetInventoryPtrByType(Type))
	{
		OutInventory = *inv;
		return true;
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
	OnComponentInventoryContainerChanged.Broadcast(this, InventoryType);
	return true;
}

bool UInventoryComponent::AddInventory(TSubclassOf<UBasePulseBaseItemAsset> ItemType, uint8 Type, uint8 Columns, uint8 Rows, bool bDecayableItemInventory)
{
	if (auto mgr = UPulseInventoryManager::Get(this))
		return mgr->AddInventory(GetContainerUID(), ItemType, Type, Columns, Rows, bDecayableItemInventory);
	return false;
}

bool UInventoryComponent::RemoveInventory(uint8 Type)
{
	if (auto mgr = UPulseInventoryManager::Get(this))
		return mgr->RemoveInventory(GetContainerUID(), Type);
	return false;
}

bool UInventoryComponent::SetInventorySize(uint8 Type, uint8 Columns, uint8 Rows, bool bPrioritizeInventorySize)
{
	if (auto mgr = UPulseInventoryManager::Get(this))
		return mgr->SetInventorySize(GetContainerUID(), Type, Columns, Rows, bPrioritizeInventorySize);
	return false;
}


EInventoryContainerType UPulseInventoryManager::GetContainerPtrType_Internal(const FInventoryContainer* ContainerPtr)
{
	for (auto pair : _PlayersInventories)
		for (auto s_pair : pair.Value.InventoryPerType)
			if (&_PlayersInventories[pair.Key] == ContainerPtr)
				return EInventoryContainerType::Player;
	for (auto pair : _AssetInventories)
		for (auto s_pair : pair.Value.InventoryPerType)
			if (&_AssetInventories[pair.Key] == ContainerPtr)
				return EInventoryContainerType::IDObject;
	for (auto pair : _ActorInventories)
		for (auto s_pair : pair.Value.InventoryPerType)
			if (&_ActorInventories[pair.Key] == ContainerPtr)
				return EInventoryContainerType::Actor;
	return EInventoryContainerType::None;
}

FInventoryContainer* UPulseInventoryManager::GetContainerPtr_Internal(const FGuid& ContainerUID)
{
	if (!ContainerUID.IsValid())
		return nullptr;
	for (auto pair : _PlayersInventories)
		if (_PlayersInventories[pair.Key].ContainerUID == ContainerUID)
			return &_PlayersInventories[pair.Key];
	for (auto pair : _AssetInventories)
		if (_AssetInventories[pair.Key].ContainerUID == ContainerUID)
			return &_AssetInventories[pair.Key];
	for (auto pair : _ActorInventories)
		if (_ActorInventories[pair.Key].ContainerUID == ContainerUID)
			return &_ActorInventories[pair.Key];
	return nullptr;
}

FInventory* UPulseInventoryManager::GetInventoryPtr_Internal(const FGuid& ContainerUID, const uint8 InventoryType)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return nullptr;
	return container->GetInventoryPtrByType(InventoryType);
}

void UPulseInventoryManager::AffectBoundInventories(const FGuid ContainerUID, uint8 InventoryType)
{
	TArray<FInventory*> BoundInventoryPtrs;
	FInventory ChangedInventory;
	if (GetInventory(ContainerUID, InventoryType, ChangedInventory) &&
		GetInventories([ContainerUID, InventoryType](const FInventory& Inventory)-> bool { return Inventory.BoundInventory == FInventoryBound(ContainerUID, InventoryType); },
		               BoundInventoryPtrs))
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

void UPulseInventoryManager::AffectItemInBoundInventories(FGuid ContainerUID, uint8 InventoryType, FPrimaryAssetId ItemID)
{
	AffectBoundInventories(ContainerUID, InventoryType);
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

FInventoryContainer* UPulseInventoryManager::GetContainerPtr(const UObject* WorldContextObject, const FGuid& ContainerUID)
{
	if (auto instance = Get(WorldContextObject))
		return instance->GetContainerPtr_Internal(ContainerUID);
	return nullptr;
}

bool UPulseInventoryManager::GetInventory(const FGuid& ContainerUID, uint8 InventoryType, FInventory& OutInventory)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	auto invPtr = container->GetInventoryPtrByType(InventoryType);
	if (!invPtr)
		return false;
	OutInventory = *invPtr;
	return true;
}

bool UPulseInventoryManager::GetInventories(std::function<bool(const FInventory&)> SelectFunc, TArray<FInventory*>& OutInventoryPtrs)
{
	if (!SelectFunc)
		return false;
	OutInventoryPtrs.Empty();
	for (auto pair : _PlayersInventories)
		for (auto s_pair : pair.Value.InventoryPerType)
			if (SelectFunc(_PlayersInventories[pair.Key].InventoryPerType[s_pair.Key]))
				OutInventoryPtrs.Add(&_PlayersInventories[pair.Key].InventoryPerType[s_pair.Key]);
	for (auto pair : _AssetInventories)
		for (auto s_pair : pair.Value.InventoryPerType)
			if (SelectFunc(_AssetInventories[pair.Key].InventoryPerType[s_pair.Key]))
				OutInventoryPtrs.Add(&_AssetInventories[pair.Key].InventoryPerType[s_pair.Key]);
	for (auto pair : _ActorInventories)
		for (auto s_pair : pair.Value.InventoryPerType)
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
	OnInventoryItemChanged.AddDynamic(this, &UPulseInventoryManager::AffectItemInBoundInventories);
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
	OnInventoryItemChanged.RemoveDynamic(this, &UPulseInventoryManager::AffectItemInBoundInventories);
}

void UPulseInventoryManager::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}


UClass* UPulseInventoryManager::GetSaveClassType_Implementation()
{
	return USaveInventory::StaticClass();
}

void UPulseInventoryManager::OnLoadedObject_Implementation(UObject* LoadedObject)
{
	if (!_bCanSave)
		return;
	if (!GetNetHasAuthority())
		return;
	if (USaveInventory* savedInventory = Cast<USaveInventory>(LoadedObject))
	{
		int32 playerId = -1;
		if (auto plc = UGameplayStatics::GetPlayerController(this, savedInventory->UserIndex))
			if (auto pls = plc->GetPlayerState<APlayerState>())
				playerId = pls->GetPlayerId();
		if (playerId >= 0)
		{
			RemovePlayerContainers(playerId);
			if (AddNewPlayerContainer(savedInventory->PlayerInventory.ContainerUID, playerId))
			{
				for (const auto pair : savedInventory->PlayerInventory.InventoryPerType)
				{
					if (AddInventory(savedInventory->PlayerInventory.ContainerUID, pair.Value.ItemTypeRestriction.Get(), pair.Key, pair.Value.InventorySlots.X,
					                 pair.Value.InventorySlots.Y,
					                 pair.Value.bIsDecayableItemInventory))
					{
						for (const auto itemEntry : savedInventory->PlayerInventory.InventoryPerType[pair.Key].Items)
						{
							AddItem(savedInventory->PlayerInventory.ContainerUID, pair.Key, itemEntry, itemEntry.GetQuantity(), true);
						}
					}
				}
			}
		}
		RemoveAllContainerOfType(EInventoryContainerType::IDObject);
		for (const auto as_pair : savedInventory->AssetInventories)
		{
			if (AddNewAssetContainer(as_pair.Value.ContainerUID, as_pair.Key))
			{
				for (const auto pair : savedInventory->AssetInventories[as_pair.Key].InventoryPerType)
				{
					if (AddInventory(as_pair.Value.ContainerUID, pair.Value.ItemTypeRestriction.Get(), pair.Key, pair.Value.InventorySlots.X,
					                 pair.Value.InventorySlots.Y,
					                 pair.Value.bIsDecayableItemInventory))
					{
						for (const auto itemEntry : savedInventory->AssetInventories[as_pair.Key].InventoryPerType[pair.Key].Items)
						{
							AddItem(as_pair.Value.ContainerUID, pair.Key, itemEntry, itemEntry.GetQuantity(), true);
						}
					}
				}
			}
		}
	}
}

UObject* UPulseInventoryManager::OnSaveObject_Implementation(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave)
{
	if (!_bCanSave)
		return nullptr;
	int32 playerId = 0;
	if (auto plc = UGameplayStatics::GetPlayerController(this, UserIndex))
		if (auto pls = plc->GetPlayerState<APlayerState>())
			playerId = pls->GetPlayerId();
	auto saveObj = NewObject<USaveInventory>();
	saveObj->UserIndex = UserIndex;
	if (_PlayersInventories.Contains(playerId))
		saveObj->PlayerInventory = _PlayersInventories[playerId];
	for (const auto pair : _AssetInventories)
		saveObj->AssetInventories.Add(pair.Key, pair.Value);
	return saveObj;
}

bool UPulseInventoryManager::FailedClientNetRepValueToRPCCall()
{
	return true;
}


void UPulseInventoryManager::OnNetInit_Implementation()
{
	int32 playerId = -1;
	if (auto gs = UGameplayStatics::GetGameState(this))
	{
		for (auto ps : gs->PlayerArray)
		{
			if (_PlayersInventories.Contains(playerId))
				continue;
			_PlayersInventories.Add(ps->GetPlayerId(), {});
		}
	}
	if (!_bCanReplicate)
		return;
	TArray<FReplicatedEntry> outNetDatas;
	if (IIPulseNetObject::Execute_TryGetNetRepValues(this, _repContainerTag, outNetDatas))
	{
		for (const auto netData : outNetDatas)
			IIPulseNetObject::Execute_OnNetValueReplicated(this, netData.Tag, netData, EReplicationEntryOperationType::AddNew);
	}
	if (IIPulseNetObject::Execute_TryGetNetRepValues(this, _repInventoryTag, outNetDatas))
	{
		for (const auto netData : outNetDatas)
			IIPulseNetObject::Execute_OnNetValueReplicated(this, netData.Tag, netData, EReplicationEntryOperationType::AddNew);
	}
	if (IIPulseNetObject::Execute_TryGetNetRepValues(this, _repItemTag, outNetDatas))
	{
		for (const auto netData : outNetDatas)
			IIPulseNetObject::Execute_OnNetValueReplicated(this, netData.Tag, netData, EReplicationEntryOperationType::AddNew);
	}
}

void UPulseInventoryManager::OnNetValueReplicated_Implementation(const FName Tag, FReplicatedEntry Value, EReplicationEntryOperationType OpType)
{
	if (!_bCanReplicate)
		return;
	auto strTag = Tag.ToString();
	if (!strTag.Contains(_repRootTag.ToString()))
		return;
	TArray<FName> tagParts;
	if (!UPulseSystemLibrary::ExtractNametagParts(Tag, tagParts))
		return;
	if (!tagParts.IsValidIndex(4))
		return;
	FGuid containerUID;
	if (!FGuid::Parse(tagParts[4].ToString(), containerUID))
		return;
	uint8 inventoryType;
	if (tagParts[3] == _repItemTag && tagParts.IsValidIndex(6))
	{
		// Item
		inventoryType = FCString::Atoi(*tagParts[5].ToString());
		FPrimaryAssetId itemID = FPrimaryAssetId::FromString(tagParts[6].ToString());
		OnItemReplication(containerUID, inventoryType, itemID, Value, OpType);
	}
	else if (tagParts[3] == _repInventoryTag && tagParts.IsValidIndex(5))
	{
		// Inventory
		inventoryType = FCString::Atoi(*tagParts[5].ToString());
		OnInventoryReplication(containerUID, inventoryType, Value, OpType);
	}
	else if (tagParts[3] == _repContainerTag)
	{
		// Container
		OnContainerReplication(containerUID, Value, OpType);
	}
}


bool UPulseInventoryManager::BindInventories(const FGuid& SourceContainerUID, const uint8& SourceInventoryType, const FGuid& TargetContainerUID, const uint8& TargetInventoryType)
{
	if (!SourceContainerUID.IsValid() || !TargetContainerUID.IsValid())
		return false;
	auto s_container = GetContainerPtr_Internal(SourceContainerUID);
	if (!s_container)
		return false;
	auto s_inv = s_container->GetInventoryPtrByType(SourceInventoryType);
	if (!s_inv)
		return false;
	if (auto t_container = GetContainerPtr_Internal(TargetContainerUID))
	{
		if (auto t_inv = t_container->GetInventoryPtrByType(TargetInventoryType))
		{
			t_inv->BoundInventory = FInventoryBound(SourceContainerUID, SourceInventoryType);
			return true;
		}
	}
	return false;
}

bool UPulseInventoryManager::UnbindInventories(const FGuid& ContainerUID, const uint8& InventoryType)
{
	if (!ContainerUID.IsValid())
		return false;
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	auto inv = container->GetInventoryPtrByType(InventoryType);
	if (!inv)
		return false;
	inv->BoundInventory = FInventoryBound();
	return true;
}


int32 UPulseInventoryManager::GetItemQuantity(const FInventoryItem& Item)
{
	return Item.GetQuantity();
}

bool UPulseInventoryManager::GetItemSlot(const FGuid& ContainerUID, const uint8& InventoryType, const FPrimaryAssetId& ItemID, FUInt84& OutLocationAndOccupation)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	return inv->GetItemSlot(ItemID, OutLocationAndOccupation);
}

bool UPulseInventoryManager::GetAllFreeSlots(const FGuid& ContainerUID, const uint8& InventoryType, TArray<FUInt82>& OutFreeSlots)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	return inv->GetAllFreeSlots(OutFreeSlots);
}

bool UPulseInventoryManager::CanFitItem(const FGuid& ContainerUID, const uint8& InventoryType, const FUInt82& ItemLocation, const FUInt82& ItemOccupation)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	return inv->CanFitItem(ItemLocation, ItemOccupation);
}

bool UPulseInventoryManager::IsValidInventory(const FGuid& ContainerUID, const uint8& InventoryType)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	return inv->IsValidInventory();
}

bool UPulseInventoryManager::GetItemInfos(const FGuid& ContainerUID, const uint8& InventoryType, const FPrimaryAssetId& ItemID, FInventoryItem& OutItem)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	return inv->GetItemInfos(ItemID, OutItem);
}

bool UPulseInventoryManager::GetPossibleItemMovements(const FGuid& ContainerUID, const uint8& InventoryType, const FPrimaryAssetId& ItemID, TArray<FUInt82>& OutLocations)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	return inv->GetPossibleMovements(ItemID, OutLocations);
}

bool UPulseInventoryManager::AddItem(const FGuid& ContainerUID, const uint8& InventoryType, FInventoryItem Item, int Quantity, bool bAttemptReorganisation)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	if (inv->AddItem(Item, Quantity, bAttemptReorganisation))
	{
		const auto tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *Item.ItemID.ToString()))
		});
		const auto repEntry = MakeItemRep(ContainerUID, InventoryType, Item.ItemID);
		IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
		OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
		return true;
	}
	return false;
}

bool UPulseInventoryManager::ConsumeItem(const FGuid& ContainerUID, const uint8& InventoryType, FPrimaryAssetId ItemID, int Quantity, int32& OutConsumedQuantity)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	if (inv->ConsumeItem(ItemID, Quantity, OutConsumedQuantity))
	{
		const auto tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *ItemID.ToString()))
		});
		if (inv->Items.IndexOfByPredicate([ItemID](const FInventoryItem& items)-> bool { return items.ItemID == ItemID; }) != INDEX_NONE)
		{
			const auto repEntry = MakeItemRep(ContainerUID, InventoryType, ItemID);
			IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
			OnInventoryItemChanged.Broadcast(ContainerUID, InventoryType, ItemID);
		}
		else
		{
			IIPulseNetObject::Execute_RemoveReplicationTag(this, tag, GetContainerActor(ContainerUID));
			OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
		}
		return true;
	}
	return false;
}

bool UPulseInventoryManager::RemoveItem(const FGuid& ContainerUID, const uint8& InventoryType, FPrimaryAssetId ItemID)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	if (inv->RemoveItem(ItemID))
	{
		const auto tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *ItemID.ToString()))
		});
		IIPulseNetObject::Execute_RemoveReplicationTag(this, tag, GetContainerActor(ContainerUID));
		OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
		return true;
	}
	return false;
}

bool UPulseInventoryManager::DecayItem(const FGuid& ContainerUID, const uint8& InventoryType, FPrimaryAssetId ItemID, float DecayAmount, int32 DecayItemIndex)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	if (!inv->bIsDecayableItemInventory)
		return false;
	if (inv->DecayItem(ItemID, DecayAmount, DecayItemIndex))
	{
		if (inv->Items.IndexOfByPredicate([ItemID](const FInventoryItem& items)-> bool { return items.ItemID == ItemID; }) != INDEX_NONE)
		{
			OnInventoryItemChanged.Broadcast(ContainerUID, InventoryType, ItemID);
			return true;
		}
		const auto tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *ItemID.ToString()))
		});
		IIPulseNetObject::Execute_RemoveReplicationTag(this, tag, GetContainerActor(ContainerUID));
		OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
		return true;
	}
	return false;
}

void UPulseInventoryManager::DecayAllItems(const FGuid& ContainerUID, const uint8& InventoryType, float DecayAmount, int32 DecayItemIndex)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return;
	if (!inv->bIsDecayableItemInventory)
		return;
	TArray<FPrimaryAssetId> itemIDList;
	for (const auto item : inv->Items)
		itemIDList.Add(item.ItemID);
	inv->DecayAllItems(DecayAmount, DecayItemIndex);
	for (const auto id : itemIDList)
	{
		if (inv->Items.IndexOfByPredicate([id](const FInventoryItem& item)-> bool { return item.ItemID == id; }) != INDEX_NONE)
		{
			OnInventoryItemChanged.Broadcast(ContainerUID, InventoryType, id);
			continue;
		}
		const auto tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *id.ToString()))
		});
		IIPulseNetObject::Execute_RemoveReplicationTag(this, tag, GetContainerActor(ContainerUID));
	}
	if (inv->Items.Num() != itemIDList.Num())
		OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
}

bool UPulseInventoryManager::MoveItem(const FGuid& ContainerUID, const uint8& InventoryType, const FPrimaryAssetId& ItemID, const FUInt82 NewLocation, bool bMoveOverlappingItems)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	if (inv->MoveItem(ItemID, NewLocation, bMoveOverlappingItems))
	{
		const auto tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *ItemID.ToString()))
		});
		const auto repEntry = MakeItemRep(ContainerUID, InventoryType, ItemID);
		IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
		OnInventoryItemChanged.Broadcast(ContainerUID, InventoryType, ItemID);
		return true;
	}
	return false;
}

bool UPulseInventoryManager::CombineItems(const FGuid& ContainerUID, const uint8& InventoryType, const TArray<FInventoryItemCombineIngredient>& Ingredients,
                                          const FInventoryItem& ResultItem, uint8 ResultQty)
{
	auto inv = GetInventoryPtr_Internal(ContainerUID, InventoryType);
	if (!inv)
		return false;
	TArray<FPrimaryAssetId> ingredientIDList;
	for (const auto item : Ingredients)
		ingredientIDList.Add(item.ItemID);
	FInventoryItem dummyItem;
	bool containsResult = inv->GetItemInfos(ResultItem.ItemID, dummyItem);
	if (inv->CombineItems(Ingredients, ResultItem, ResultQty))
	{
		FReplicatedEntry repEntry;
		FName tag;
		for (const auto id : ingredientIDList)
		{
			tag = UPulseSystemLibrary::ConstructNametag({
				_repRootTag,
				_repItemTag,
				FName(ContainerUID.ToString()),
				FName(FString::Printf(TEXT("%d"), InventoryType)),
				FName(FString::Printf(TEXT("%s"), *id.ToString()))
			});
			if (inv->Items.IndexOfByPredicate([id](const FInventoryItem& item)-> bool { return item.ItemID == id; }) != INDEX_NONE)
			{
				repEntry = MakeItemRep(ContainerUID, InventoryType, id);
				IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
				OnInventoryItemChanged.Broadcast(ContainerUID, InventoryType, id);
				continue;
			}
			IIPulseNetObject::Execute_RemoveReplicationTag(this, tag, GetContainerActor(ContainerUID));
		}
		tag = UPulseSystemLibrary::ConstructNametag({
			_repRootTag,
			_repItemTag,
			FName(ContainerUID.ToString()),
			FName(FString::Printf(TEXT("%d"), InventoryType)),
			FName(FString::Printf(TEXT("%s"), *ResultItem.ItemID.ToString()))
		});
		repEntry = MakeItemRep(ContainerUID, InventoryType, ResultItem.ItemID);
		IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
		if (containsResult)
			OnInventoryItemChanged.Broadcast(ContainerUID, InventoryType, ResultItem.ItemID);
		else
			OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
		return true;
	}
	return false;
}

void UPulseInventoryManager::OnItemReplication(const FGuid& ContainerUID, const uint8& InventoryType, const FPrimaryAssetId& ItemID, FReplicatedEntry Value,
                                               EReplicationEntryOperationType OpType)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return;
	auto invPtr = container->GetInventoryPtrByType(InventoryType);
	if (!invPtr)
		return;
	FInventoryItem item;
	if (OpType == EReplicationEntryOperationType::Update && !invPtr->GetItemInfos(ItemID, item))
		OpType = EReplicationEntryOperationType::AddNew;
	switch (OpType)
	{
	case EReplicationEntryOperationType::Update:
		{
			int32 consumedQty = 0;
			if (!invPtr->GetItemInfos(ItemID, item))
				return;
			if (item.GetQuantity() < Value.Float32Value.Z)
				AddItem(ContainerUID, InventoryType, item, Value.Float32Value.Z - item.GetQuantity());
			else if (item.GetQuantity() > Value.Float32Value.Z)
				ConsumeItem(ContainerUID, InventoryType, ItemID, item.GetQuantity() - Value.Float32Value.Z, consumedQty);
			if (item.ItemLocation != FUInt82(Value.Float32Value.X, Value.Float32Value.Y))
				MoveItem(ContainerUID, InventoryType, ItemID, FUInt82(Value.Float32Value.X, Value.Float32Value.Y), true);
		}
		break;
	case EReplicationEntryOperationType::AddNew:
		{
			item.ItemID = ItemID;
			item.ItemOccupation = FUInt82(Value.Float31Value.X, Value.Float31Value.Y);
			item.ItemRotation = static_cast<EInventoryItemRotation>(Value.Float31Value.Z);
			AddItem(ContainerUID, InventoryType, item, Value.Float32Value.Z);
		}
		break;
	case EReplicationEntryOperationType::Remove:
		RemoveItem(ContainerUID, InventoryType, ItemID);
		break;
	}
}

FReplicatedEntry UPulseInventoryManager::MakeItemRep(const FGuid& ContainerUID, const uint8& InventoryType, const FPrimaryAssetId& ItemID)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return FReplicatedEntry();
	auto invPtr = container->GetInventoryPtrByType(InventoryType);
	if (!invPtr)
		return FReplicatedEntry();
	FInventoryItem item;
	if (!invPtr->GetItemInfos(ItemID, item))
		return FReplicatedEntry();
	return FReplicatedEntry()
	       .WithVector(FVector(item.ItemOccupation.X, item.ItemOccupation.Y, static_cast<int>(item.ItemRotation)), 0)
	       .WithVector(FVector(item.ItemLocation.X, item.ItemLocation.Y, item.GetQuantity()), 1);
}


bool UPulseInventoryManager::AddInventory(const FGuid& ContainerUID, TSubclassOf<UBasePulseBaseItemAsset> ItemType, uint8 Type, uint8 Columns, uint8 Rows,
                                          bool bDecayableItemInventory)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	FInventory NewInventory = FInventory().OfSize(Columns, Rows).WithTypeRestriction(ItemType).WithDecayableItems(bDecayableItemInventory);
	if (!NewInventory.IsValidInventory())
		return false;
	if (container->GetInventoryPtrByType(Type) != nullptr)
		return false;
	container->InventoryPerType.Add(Type, NewInventory);
	FReplicatedEntry repEntry = MakeInventoryRep(ContainerUID, Type);
	const auto tag = UPulseSystemLibrary::ConstructNametag({_repRootTag, _repInventoryTag, FName(ContainerUID.ToString()), FName(FString::Printf(TEXT("%d"), Type))});
	IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
	OnInventoryContainerChanged.Broadcast(ContainerUID);
	return true;
}

bool UPulseInventoryManager::RemoveInventory(const FGuid& ContainerUID, const uint8& InventoryType)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	if (container->GetInventoryPtrByType(InventoryType) == nullptr)
		return false;
	if (container->InventoryPerType.Remove(InventoryType) >= 0)
	{
		const auto tag = UPulseSystemLibrary::ConstructNametag({_repRootTag, _repInventoryTag, FName(ContainerUID.ToString()), FName(FString::Printf(TEXT("%d"), InventoryType))});
		IIPulseNetObject::Execute_RemoveReplicationTag(this, tag, GetContainerActor(ContainerUID));
		OnInventoryContainerChanged.Broadcast(ContainerUID);
	}
	return true;
}

bool UPulseInventoryManager::GetInventoryInfos(const FGuid& ContainerUID, const uint8& InventoryType, FInventory& OutInventory)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	const auto inv = container->GetInventoryPtrByType(InventoryType);
	if (!inv)
		return false;
	OutInventory = FInventory(*inv);
	return true;
}

bool UPulseInventoryManager::SetInventorySize(const FGuid& ContainerUID, const uint8& InventoryType, uint8 Columns, uint8 Rows, bool bPrioritizeInventorySize)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	auto inv = container->GetInventoryPtrByType(InventoryType);
	if (inv == nullptr)
		return false;
	if (inv->SetInventorySize(Columns, Rows, bPrioritizeInventorySize))
	{
		FReplicatedEntry repEntry = MakeInventoryRep(ContainerUID, InventoryType);
		const auto tag = UPulseSystemLibrary::ConstructNametag({_repRootTag, _repInventoryTag, FName(ContainerUID.ToString()), FName(FString::Printf(TEXT("%d"), InventoryType))});
		IIPulseNetObject::Execute_ReplicateValue(this, tag, repEntry);
		OnInventoryChanged.Broadcast(ContainerUID, InventoryType);
	}
	return true;
}

void UPulseInventoryManager::OnInventoryReplication(const FGuid& ContainerUID, const uint8& InventoryType, FReplicatedEntry Value, EReplicationEntryOperationType OpType)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return;
	if (OpType == EReplicationEntryOperationType::Update && container->GetInventoryPtrByType(InventoryType) == nullptr)
		OpType = EReplicationEntryOperationType::AddNew;
	FSoftObjectPath SoftPath(Value.NameValue.ToString());
	auto InventoryRestrictedClass = TSoftClassPtr<UBasePulseBaseItemAsset>(SoftPath).Get();
	switch (OpType)
	{
	case EReplicationEntryOperationType::Update:
		SetInventorySize(ContainerUID, InventoryType, Value.Float31Value.X, Value.Float31Value.Y, true);
		break;
	case EReplicationEntryOperationType::AddNew:
		AddInventory(ContainerUID, InventoryRestrictedClass, InventoryType, Value.Float31Value.X, Value.Float31Value.Y, Value.FlagValue > 0 ? true : false);
		break;
	case EReplicationEntryOperationType::Remove:
		RemoveInventory(ContainerUID, InventoryType);
		break;
	}
}

FReplicatedEntry UPulseInventoryManager::MakeInventoryRep(const FGuid& ContainerUID, const uint8& InventoryType)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return FReplicatedEntry();
	auto invPtr = container->GetInventoryPtrByType(InventoryType);
	if (!invPtr)
		return FReplicatedEntry();
	return FReplicatedEntry()
	       .WithName(invPtr->ItemTypeRestriction.IsValid() ? FName(invPtr->ItemTypeRestriction.ToString()) : "")
	       .WithVector(FVector(invPtr->InventorySlots.X, invPtr->InventorySlots.Y, 0))
	       .WithFlags(invPtr->bIsDecayableItemInventory ? 1 : 0);
}


bool UPulseInventoryManager::GetContainerUID(EInventoryContainerType ContainerType, FGuid& OutContainerUID, int32 LocalPlayerIndex, FPrimaryAssetId ObjectID, AActor* ObjectPtr)
{
	if (LocalPlayerIndex >= 0)
	{
		if (ContainerType != EInventoryContainerType::Player)
			return false;
		auto playerCtrl = UGameplayStatics::GetPlayerControllerFromID(this, LocalPlayerIndex);
		if (!playerCtrl)
			return false;
		auto playerState = playerCtrl->GetPlayerState<APlayerState>();
		if (!playerState)
			return false;
		const int32 PlayerID = playerState->GetPlayerId();
		for (const auto& containerPair : _PlayersInventories)
		{
			if (containerPair.Key == PlayerID)
			{
				OutContainerUID = containerPair.Value.ContainerUID;
				return true;
			}
		}
	}
	else if (ObjectID.IsValid())
	{
		if (ContainerType != EInventoryContainerType::IDObject)
			return false;
		for (const auto& containerPair : _AssetInventories)
		{
			if (containerPair.Key == ObjectID)
			{
				OutContainerUID = containerPair.Value.ContainerUID;
				return true;
			}
		}
	}
	else if (ObjectPtr)
	{
		if (ContainerType != EInventoryContainerType::Actor)
			return false;
		for (const auto& containerPair : _ActorInventories)
		{
			if (containerPair.Key == ObjectPtr)
			{
				OutContainerUID = containerPair.Value.ContainerUID;
				return true;
			}
		}
	}
	return false;
}

AActor* UPulseInventoryManager::GetContainerActor(const FGuid& ContainerUID)
{
	for (const auto& containerPair : _ActorInventories)
	{
		if (containerPair.Value.ContainerUID == ContainerUID)
			return containerPair.Key;
	}
	return nullptr;
}

bool UPulseInventoryManager::GetAllPlayerIDInventories(TArray<int32>& OutPlayerIDs, TArray<FGuid>& OutContainerUIDs)
{
	OutPlayerIDs.Empty();
	OutContainerUIDs.Empty();
	for (const auto& containerPair : _PlayersInventories)
	{
		OutPlayerIDs.Add(containerPair.Key);
		OutContainerUIDs.Add(containerPair.Value.ContainerUID);
	}
	return OutPlayerIDs.Num() > 0;
}

bool UPulseInventoryManager::GetAllAssetIDInventories(TArray<FPrimaryAssetId>& OutAssetIDs, TArray<FGuid>& OutContainerUIDs)
{
	OutAssetIDs.Empty();
	OutContainerUIDs.Empty();
	for (const auto& containerPair : _AssetInventories)
	{
		OutAssetIDs.Add(containerPair.Key);
		OutContainerUIDs.Add(containerPair.Value.ContainerUID);
	}
	return OutAssetIDs.Num() > 0;
}

bool UPulseInventoryManager::GetAllActorInventories(TArray<AActor*>& OutActors, TArray<FGuid>& OutContainerUIDs)
{
	OutActors.Empty();
	OutContainerUIDs.Empty();
	for (const auto& containerPair : _ActorInventories)
	{
		OutActors.Add(containerPair.Key);
		OutContainerUIDs.Add(containerPair.Value.ContainerUID);
	}
	return OutActors.Num() > 0;
}

bool UPulseInventoryManager::CreateNewContainer(const EInventoryContainerType& OfType, APlayerController* PlayerController, FPrimaryAssetId AssetID, AActor* ObjectPtr)
{
	const auto newGuid = FGuid::NewGuid();
	switch (OfType)
	{
	default:
		break;
	case EInventoryContainerType::Player:
		{
			if (!PlayerController)
				return false;
			const auto gs = PlayerController->GetPlayerState<APlayerState>();
			if (!gs)
				return false;
			const auto playerId = gs->GetPlayerId();
			return AddNewPlayerContainer(newGuid, playerId);
		}
	case EInventoryContainerType::IDObject:
		return AddNewAssetContainer(newGuid, AssetID);
	case EInventoryContainerType::Actor:
		return AddNewActorContainer(newGuid, ObjectPtr);
	}
	return false;
}

bool UPulseInventoryManager::AddNewPlayerContainer(const FGuid& ContainerUID, const int32 PlayerID)
{
	if (!ContainerUID.IsValid())
		return false;
	if (_PlayersInventories.Contains(PlayerID))
		return false;
	_PlayersInventories.Add(PlayerID, FInventoryContainer(ContainerUID));
	FReplicatedEntry repEntry = MakeContainerRep(PlayerID);
	IIPulseNetObject::Execute_ReplicateValue(this, UPulseSystemLibrary::ConstructNametag({_repRootTag, _repContainerTag, FName(ContainerUID.ToString())}), repEntry);
	OnInventoryContainerAdded.Broadcast(ContainerUID, EInventoryContainerType::Player, PlayerID, {}, nullptr);
	return true;
}

bool UPulseInventoryManager::AddNewAssetContainer(const FGuid& ContainerUID, const FPrimaryAssetId& AssetID)
{
	if (!ContainerUID.IsValid())
		return false;
	if (!AssetID.IsValid())
		return false;
	if (_AssetInventories.Contains(AssetID))
		return false;
	_AssetInventories.Add(AssetID, FInventoryContainer(ContainerUID));
	FReplicatedEntry repEntry = MakeContainerRep(-1, AssetID);
	IIPulseNetObject::Execute_ReplicateValue(this, UPulseSystemLibrary::ConstructNametag({_repRootTag, _repContainerTag, FName(ContainerUID.ToString())}), repEntry);
	OnInventoryContainerAdded.Broadcast(ContainerUID, EInventoryContainerType::IDObject, -1, AssetID, nullptr);
	return true;
}

bool UPulseInventoryManager::AddNewActorContainer(const FGuid& ContainerUID, AActor* Actor)
{
	if (!ContainerUID.IsValid())
		return false;
	if (!Actor)
		return false;
	if (_ActorInventories.Contains(Actor))
		return false;
	FReplicatedEntry repEntry = MakeContainerRep(-1, {}, Actor);
	IIPulseNetObject::Execute_ReplicateValue(this, UPulseSystemLibrary::ConstructNametag({_repRootTag, _repContainerTag, FName(ContainerUID.ToString())}), repEntry);
	OnInventoryContainerAdded.Broadcast(ContainerUID, EInventoryContainerType::Actor, -1, {}, Actor);
	return true;
}

bool UPulseInventoryManager::GetContainerInfos(const FGuid& ContainerUID, FInventoryContainer& OutContainer)
{
	auto container = GetContainerPtr_Internal(ContainerUID);
	if (!container)
		return false;
	OutContainer = FInventoryContainer(*container);
	return true;
}

bool UPulseInventoryManager::RemoveContainer(const FGuid& ContainerUID)
{
	int32 PlayerID = -1;
	FPrimaryAssetId ObjectAssetID = {};
	AActor* Actor = nullptr;
	EInventoryContainerType Type = EInventoryContainerType::None;
	bool foundContainer = false;
	if (!foundContainer)
		for (auto pair : _PlayersInventories)
			if (_PlayersInventories[pair.Key].ContainerUID == ContainerUID)
			{
				PlayerID = pair.Key;
				Type = EInventoryContainerType::Player;
				foundContainer = true;
				_PlayersInventories.Remove(pair.Key);
				break;
			}
	if (!foundContainer)
		for (auto pair : _AssetInventories)
			if (_AssetInventories[pair.Key].ContainerUID == ContainerUID)
			{
				ObjectAssetID = pair.Key;
				Type = EInventoryContainerType::IDObject;
				foundContainer = true;
				_AssetInventories.Remove(pair.Key);
				break;
			}
	if (!foundContainer)
		for (auto pair : _ActorInventories)
			if (_ActorInventories[pair.Key].ContainerUID == ContainerUID)
			{
				Actor = pair.Key;
				Type = EInventoryContainerType::Actor;
				foundContainer = true;
				_ActorInventories.Remove(pair.Key);
				break;
			}
	if (!foundContainer)
		return false;
	IIPulseNetObject::Execute_RemoveReplicationTag(this, UPulseSystemLibrary::ConstructNametag({_repRootTag, _repContainerTag, FName(ContainerUID.ToString())}), Actor);
	OnInventoryContainerRemoved.Broadcast(ContainerUID, Type, PlayerID, ObjectAssetID, Actor);
	return true;
}

bool UPulseInventoryManager::RemovePlayerContainers(const int32 PlayerID)
{
	if (!_PlayersInventories.Contains(PlayerID))
		return false;
	return RemoveContainer(_PlayersInventories[PlayerID].ContainerUID);
}

bool UPulseInventoryManager::RemoveAssetContainers(const FPrimaryAssetId& AssetID)
{
	if (!_AssetInventories.Contains(AssetID))
		return false;
	return RemoveContainer(_AssetInventories[AssetID].ContainerUID);
}

bool UPulseInventoryManager::RemoveActorContainers(AActor* Actor)
{
	if (!_ActorInventories.Contains(Actor))
		return false;
	return RemoveContainer(_ActorInventories[Actor].ContainerUID);
}

void UPulseInventoryManager::RemoveAllContainerOfType(const EInventoryContainerType& OfType)
{
	TArray<FGuid> ContainerUIDs;
	switch (OfType)
	{
	case EInventoryContainerType::None:
		break;
	case EInventoryContainerType::Player:
		for (const auto& pair : _PlayersInventories)
			ContainerUIDs.Add(pair.Value.ContainerUID);
		break;
	case EInventoryContainerType::IDObject:
		for (const auto& pair : _AssetInventories)
			ContainerUIDs.Add(pair.Value.ContainerUID);
		break;
	case EInventoryContainerType::Actor:
		for (const auto& pair : _ActorInventories)
			ContainerUIDs.Add(pair.Value.ContainerUID);
		break;
	}
	if (ContainerUIDs.Num() <= 0)
		return;
	for (auto uid : ContainerUIDs)
		RemoveContainer(uid);
}

FReplicatedEntry UPulseInventoryManager::MakeContainerRep(int32 PlayerId, const FPrimaryAssetId& ObjectID, AActor* ObjectPtr)
{
	return FReplicatedEntry()
	       .WithInteger(PlayerId)
	       .WithName(FName(ObjectID.ToString()))
	       .WithObject(ObjectPtr);
}

void UPulseInventoryManager::OnContainerReplication(const FGuid& ContainerUID, FReplicatedEntry Value, EReplicationEntryOperationType OpType)
{
	int32 PlayerId = Value.IntegerValue;
	const FPrimaryAssetId ObjectID = FPrimaryAssetId::FromString(Value.NameValue.ToString());
	AActor* ObjectPtr = Cast<AActor>(Value.WeakObjectPtr);
	switch (OpType)
	{
	case EReplicationEntryOperationType::Update:
		break;
	case EReplicationEntryOperationType::AddNew:
		{
			if (PlayerId >= 0)
				AddNewPlayerContainer(ContainerUID, PlayerId);
			else if (ObjectID.IsValid())
				AddNewAssetContainer(ContainerUID, ObjectID);
			else if (ObjectPtr)
				AddNewActorContainer(ContainerUID, ObjectPtr);
		}
		break;
	case EReplicationEntryOperationType::Remove:
		RemoveContainer(ContainerUID);
		break;
	}
}
