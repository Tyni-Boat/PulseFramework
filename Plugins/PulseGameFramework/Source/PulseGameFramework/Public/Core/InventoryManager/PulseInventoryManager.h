// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/CoreTypes.h"
#include "Core/PulseSubModuleBase.h"
#include "Algo/Reverse.h"
#include "Core/PulseSystemLibrary.h"
#include "Core/PoolingSubModule/PulsePoolingManager.h"
#include "Core/PulseResourceManagement/Types/Item/BasePulseBaseItemAsset.h"
#include "PulseInventoryManager.generated.h"


#pragma region Additionnal Types

UENUM(BlueprintType)
enum class EInventoryContainerType: uint8
{
	Local, // The manager is not aware of this container
	Player, // Is the local player's inventory
	IDObject, // The container based on the primaryAsset ID
	Actor, // The container of the actor. like local but the manager is actually aware of it.
};

UENUM(BlueprintType)
enum class EInventoryItemRotation: uint8
{
	Zero,
	Ninety,
	Pi,
	MinusNinety,
};

USTRUCT(BlueprintType)
struct FInventoryItemCombineIngredient
{
	GENERATED_BODY()
public:
	FInventoryItemCombineIngredient(){}
	FInventoryItemCombineIngredient(FPrimaryAssetId id, uint8 qty): ItemID(id), Quantity(qty){}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FPrimaryAssetId ItemID;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	uint8 Quantity = 1;
};


USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

public:
	FInventoryItem(){}
	FInventoryItem(FPrimaryAssetId id, uint8 SizeX = 0, uint8 SizeY = 0): ItemID(id), ItemOccupation({SizeX, SizeY}){}
	
	// the asset Id of the Item. Must Be 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FPrimaryAssetId ItemID;

	// Occupation will be X = 1 + Occupation.X and Y = 1 + Occupation.Y
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FUInt82 ItemOccupation = FUInt82();

	// X-Column index, Y-Row index
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FUInt82 ItemLocation = FUInt82();

	// The current item rotation.
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	EInventoryItemRotation ItemRotation = EInventoryItemRotation::Zero;

	// The standard quantity value. 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	int32 Quantity = -1;

	// This will be use if the standard Quantity value is < 0. Qty-The item number, item health - array element (1[fully healthy] - 0[completely decayed])
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	TArray<float> QuantityAndDecay = {1.0f};

	int32 GetQuantity() const { return Quantity >= 0 ? Quantity : QuantityAndDecay.Num(); }
	bool IsValidItemEntry() const { return Quantity >= 0 ? Quantity > 0 : QuantityAndDecay.Num() > 0; }

	bool Add(int32 Amount)
	{
		if (Amount <= 0) return false;
		if (Quantity >= 0)
			Quantity += Amount;
		else
			QuantityAndDecay.Add(1);
		return true;
	}
	bool Remove(int32 Amount)
	{
		if (Amount <= 0) return false;
		if (Quantity >= 0)
			Quantity -= Amount;
		else if (QuantityAndDecay.Num() > 0)
			QuantityAndDecay.RemoveAt(QuantityAndDecay.Num() - 1);
		else
			return false;
		return true;
	}
	bool RemoveAt(int32 Index)
	{
		if (Quantity >= 0)
			Quantity -= 1;
		else if (QuantityAndDecay.IsValidIndex(Index))
			QuantityAndDecay.RemoveAt(Index);
		else
			return false;
		return true;
	}
	void MoveItem(FUInt82 NewLocation) { ItemLocation = NewLocation; }
	void RotateItem(EInventoryItemRotation NewRotation)
	{
		if (NewRotation == ItemRotation)
			return;
		bool mustSwap = (NewRotation == EInventoryItemRotation::Zero || NewRotation == EInventoryItemRotation::Pi)
			&& (ItemRotation == EInventoryItemRotation::Ninety || ItemRotation == EInventoryItemRotation::MinusNinety);
		bool mustSwap2 = (ItemRotation == EInventoryItemRotation::Zero || ItemRotation == EInventoryItemRotation::Pi)
			&& (NewRotation == EInventoryItemRotation::Ninety || NewRotation == EInventoryItemRotation::MinusNinety);
		ItemRotation = NewRotation;
		if (mustSwap || mustSwap2)
		{
			auto bkp = ItemOccupation.X;
			ItemOccupation.X = ItemOccupation.Y;
			ItemOccupation.Y = bkp;
		}
	}
	void RotateItemCCW()
	{
		EInventoryItemRotation newRot = static_cast<EInventoryItemRotation>(FMath::Modulo(static_cast<int>(ItemRotation) + 1, 4));
		RotateItem(newRot);
	}	
	void RotateItemCW()
	{
		EInventoryItemRotation newRot = static_cast<EInventoryItemRotation>(FMath::Modulo((static_cast<int>(ItemRotation) - 1 )< 0 ? 3 : static_cast<int>(ItemRotation) - 1, 4));
		RotateItem(newRot);
	}
	bool DecayItemAt(float DecayAmount, int32 Index)
	{
		if (QuantityAndDecay.IsValidIndex(Index))
		{
			QuantityAndDecay[Index] -= DecayAmount;
			QuantityAndDecay[Index] = FMath::Clamp(QuantityAndDecay[Index], 0, 1);
			if (QuantityAndDecay[Index] <= 0)
				QuantityAndDecay.RemoveAt(Index);
		}
		else
			return false;
		return true;
	}
	void DecayAllItems(float DecayAmount)
	{
		for (int Index = QuantityAndDecay.Num() - 1; Index >= 0; Index--)
		{
			QuantityAndDecay[Index] -= DecayAmount;
			QuantityAndDecay[Index] = FMath::Clamp(QuantityAndDecay[Index], 0, 1);
			if (QuantityAndDecay[Index] <= 0)
				QuantityAndDecay.RemoveAt(Index);
		}
	}
};

USTRUCT(BlueprintType)
struct FInventory
{
	GENERATED_BODY()

public:
	FInventory()
	{
	}

	FInventory(FGuid uid) { if (uid.IsValid()) UID = uid.ToString(); }

	FInventory OfSize(uint8 Columns, uint8 Rows)
	{
		InventorySlots = FUInt82(Columns, Rows);
		return *this;
	}

	FInventory WithDecayableItems(bool decayableItemInventory)
	{
		bIsDecayableItemInventory = decayableItemInventory;
		return *this;
	}

	FInventory WithTypeRestriction(TSubclassOf<UBasePulseBaseItemAsset> restrictionClass)
	{
		ItemTypeRestriction = restrictionClass;
		return *this;
	}


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FString UID = "";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FString BoundInventoryUID = "";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	bool bIsDecayableItemInventory = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	FUInt82 InventorySlots = FUInt82(1, 1);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	TArray<FInventoryItem> Items;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	TSoftClassPtr<UBasePulseBaseItemAsset> ItemTypeRestriction = nullptr;

private:
	
	TArray<FUInt82> GetSlotArray() const
	{
		TArray<FUInt82> AllSlots;
		for (uint8 i = 0; i < InventorySlots.X; i++)
			for (uint8 j = 0; j < InventorySlots.Y; j++)
				AllSlots.Add(FUInt82(i, j));
		return AllSlots;
	}
	static void BreakOccupationIntoSlots(FUInt84 LocationAndOccupation, TArray<FUInt82>& OutOccupiedSlots)
	{
		LocationAndOccupation.Z++;
		LocationAndOccupation.W++;
		for (int i = 0; i < LocationAndOccupation.Z; i++)
			for (int j = 0; j < LocationAndOccupation.W; j++)
				OutOccupiedSlots.Add(FUInt82(LocationAndOccupation.X + i, LocationAndOccupation.Y + j));
	}
	static bool GetClusteredSlots(const FUInt82& Occupation, const TArray<FUInt82>& Slots, TArray<FUInt82>& OutLocations)
	{
		OutLocations.Empty();
		for (const auto slot: Slots)
		{
			int32 expectedCount = (Occupation.X + 1) * (Occupation.Y + 1);
			for (uint8 i = 0; i <= Occupation.X; i++)
			{
				bool mustBreak = false;
				for (uint8 j = 0; j <= Occupation.Y; j++)
				{
					if (!Slots.Contains(FUInt82(slot.X + i, slot.Y + j)))
					{
						mustBreak = true;
						break;
					}
					expectedCount--;
				}
				if (mustBreak)
					break;
			}
			if (expectedCount <= 0)
				OutLocations.Add(slot);
		}
		return !OutLocations.IsEmpty();
	}
	static bool CheckClusteredSlotsAt(const FUInt84 LocationAndOccupation, const TArray<FUInt82>& Slots)
	{
		const FUInt82 Location = FUInt82(LocationAndOccupation.X, LocationAndOccupation.Y);
		const FUInt82 Occupation = FUInt82(LocationAndOccupation.Z, LocationAndOccupation.W);
		TArray<FUInt82> PossibleLocations;
		if (GetClusteredSlots(Occupation, Slots, PossibleLocations))
			return PossibleLocations.Contains(Location);
		return false;
	}
	bool GetOverlappingItems(const FInventoryItem& Item, TArray<FPrimaryAssetId>& OutItemIds) const
	{
		OutItemIds.Empty();
		TArray<FUInt82> slotsA;
		TArray<FUInt82> slotsB;
		slotsA.Empty();
		BreakOccupationIntoSlots(FUInt84(Item.ItemLocation, Item.ItemOccupation), slotsA);
		if (slotsA.IsEmpty())
			return false;
		for (int i = 0; i < Items.Num(); i++)
		{
			if (Items[i].ItemID == Item.ItemID)
				continue;
			slotsB.Empty();
			BreakOccupationIntoSlots(FUInt84(Items[i].ItemLocation, Items[i].ItemOccupation), slotsB);
			if (slotsB.IsEmpty())
				continue;
			for (const auto sb : slotsB)
			{
				if (slotsA.Contains(FUInt82(sb.X, sb.Y)))
				{
					OutItemIds.Add(Items[i].ItemID);
					break;
				}
			}
		}
		return !OutItemIds.IsEmpty();
	}
	bool CheckOverlappingItems() const
	{
		TArray<FPrimaryAssetId> overlappingItemsIds;
		for (int i = 0; i < Items.Num(); i++)
		{
			if (GetOverlappingItems(Items[i], overlappingItemsIds))
				return true;
		}
		return false;
	}
	void ReorganizeItems()
	{
		TArray<FInventoryItem> tempItems;
		for (int i = 0; i < Items.Num(); i++)
		{
			tempItems.Add(Items[i]);
			tempItems.Last().ItemLocation = FUInt82();			
		}
		Items.Empty();
		tempItems.Sort([](const FInventoryItem& a, const FInventoryItem& b)->bool{ return a.ItemOccupation.Lenght() > b.ItemOccupation.Lenght(); });
		if (tempItems[0].ItemOccupation.Lenght() < tempItems.Last().ItemOccupation.Lenght())
			Algo::Reverse(tempItems);
		for (int i = 0; i < tempItems.Num(); i++)
		{
			AddItem(tempItems[i], tempItems[i].Quantity);
		}
	}
	bool VerifyAssetType(const FPrimaryAssetId& ItemID) const
	{
		if (!ItemID.IsValid())
			return false;
		if (!ItemTypeRestriction.Get())
			return true;
		return UPulseSystemLibrary::IsAssetTypeDerivedFrom(ItemID.PrimaryAssetType, ItemTypeRestriction.Get());
	}

public:

	bool GetItemSlot(FPrimaryAssetId ItemID, FUInt84& OutLocationAndOccupation)
	{
		const int32 Index = Items.IndexOfByPredicate([ItemID](const FInventoryItem& item)-> bool { return item.ItemID == ItemID; });
		if (Index == INDEX_NONE)
			return false;
		OutLocationAndOccupation.X = Items[Index].ItemLocation.X;
		OutLocationAndOccupation.Y = Items[Index].ItemLocation.Y;
		OutLocationAndOccupation.Z = Items[Index].ItemOccupation.X;
		OutLocationAndOccupation.W = Items[Index].ItemOccupation.Y;
		return true;
	}
	bool GetAllFreeSlots(TArray<FUInt82>& OutFreeSlots)
	{
		OutFreeSlots.Empty();
		auto allSlots = GetSlotArray();
		for (int i = 0; i < Items.Num(); i++)
		{
			if (allSlots.IsEmpty())
				break;
			FUInt84 itemLocationAndOccupation;
			if (GetItemSlot(Items[i].ItemID, itemLocationAndOccupation))
			{
				TArray<FUInt82> itemOccupiedSlots;
				BreakOccupationIntoSlots(itemLocationAndOccupation, itemOccupiedSlots);
				for (const auto occ : itemOccupiedSlots)
					if (allSlots.Contains(occ))
						allSlots.Remove(occ);
			}
		}
		OutFreeSlots = allSlots;
		return OutFreeSlots.Num() > 0;
	}
	bool CanFitItem(const FUInt82& ItemLocation, const FUInt82& ItemOccupation)
	{
		TArray<FUInt82> freeSlots;
		if (!GetAllFreeSlots(freeSlots))
			return false;
		return CheckClusteredSlotsAt(FUInt84(ItemLocation, ItemOccupation), freeSlots);
	}
	bool IsValidInventory() const
	{
		FGuid uid;
		if (!FGuid::Parse(UID, uid))
			return false;
		return uid.IsValid() && InventorySlots.X > 0 && InventorySlots.Y > 0;
	}
	bool AddItem(FInventoryItem Item, int Quantity = 1, bool bAttemptReorganisation = false)
	{
		if (!Item.ItemID.IsValid())
			return false;
		if (!VerifyAssetType(Item.ItemID))
			return false;
		const int32 index = Items.IndexOfByPredicate([Item](const FInventoryItem& item)->bool{ return item.ItemID == Item.ItemID; });
		if (index != INDEX_NONE)
		{
			Items[index].Add(Quantity);
			return true;
		}
		TArray<FUInt82> availableSlots;
		if (!GetAllFreeSlots(availableSlots))
			return false;
		TArray<FUInt82> availableLocations;
		if (!GetClusteredSlots(Item.ItemOccupation, availableSlots, availableLocations))
		{
			Item.RotateItemCW();
			if (!GetClusteredSlots(Item.ItemOccupation, availableSlots, availableLocations))
			{
				if (!bAttemptReorganisation)
					return false;
				TMap<FPrimaryAssetId, FUInt82> _locationBKP;
				for (const FInventoryItem& item : Items)
					_locationBKP.Add(item.ItemID, item.ItemLocation);
				Item.QuantityAndDecay.Empty();
				Item.Quantity = -1;
				if (bIsDecayableItemInventory)
					for (int i = 0; i < Quantity; i++)
						Item.QuantityAndDecay.Add(1);
				else
					Item.Quantity = Quantity;
				Items.Add(Item);
				ReorganizeItems();
				if (!CheckOverlappingItems())
						return true;
				const int32 idx = Items.IndexOfByPredicate([Item](const FInventoryItem& item)->bool{ return item.ItemID == Item.ItemID; });
				if (index != INDEX_NONE)
				{
					Items[idx].RotateItemCW();
					ReorganizeItems();
					if (!CheckOverlappingItems())
						return true;
				}
				for (int i = Items.Num() - 1; i >= 0; i--)
				{
					if (Items[i].ItemID == Item.ItemID)
					{
						Items.RemoveAt(i);
						continue;
					}
					if (_locationBKP.Contains(Items[i].ItemID))
					{
						Items[i].ItemLocation = _locationBKP[Items[i].ItemID];
					}
				}
				return false;
			}
		}
		Item.ItemLocation = availableLocations[0];
		Item.QuantityAndDecay.Empty();
		Item.Quantity = -1;
		if (bIsDecayableItemInventory)
			for (int i = 0; i < Quantity; i++)
				Item.QuantityAndDecay.Add(1);
		else
			Item.Quantity = Quantity;
		Items.Add(Item);
		return true;
	}
	bool ConsumeItem(FPrimaryAssetId ItemID, int Quantity, int32& OutConsumedQuantity)
	{
		if (!ItemID.IsValid())
			return false;
		const int32 index = Items.IndexOfByPredicate([ItemID](const FInventoryItem& item)->bool{ return item.ItemID == ItemID; });
		if (index == INDEX_NONE)
			return false;
		OutConsumedQuantity = Quantity >= Items[index].Quantity? Items[index].Quantity : Items[index].Quantity - Quantity;
		if (!Items[index].Remove(OutConsumedQuantity))
			return false;
		if (Items[index].GetQuantity() <= 0)
			Items.RemoveAt(index);
		return true;
	}
	bool RemoveItem(FPrimaryAssetId ItemID)
	{
		if (!ItemID.IsValid())
			return false;
		const int32 index = Items.IndexOfByPredicate([ItemID](const FInventoryItem& item)->bool{ return item.ItemID == ItemID; });
		if (index == INDEX_NONE)
			return false;
		Items.RemoveAt(index);
		return true;
	}
	bool DecayItem(FPrimaryAssetId ItemID, float decayAmount, int32 decayItemIndex = -1)
	{
		if (!bIsDecayableItemInventory)
			return false;
		if (!ItemID.IsValid())
			return false;
		const int32 index = Items.IndexOfByPredicate([ItemID](const FInventoryItem& item)->bool{ return item.ItemID == ItemID; });
		if (index == INDEX_NONE)
			return false;
		if (decayItemIndex >= 0)
			return Items[index].DecayItemAt(decayAmount, decayItemIndex);
		else
			Items[index].DecayAllItems(decayAmount);
		if (Items[index].GetQuantity() <= 0)
			Items.RemoveAt(index);
		return true;
	}
	void DecayAllItems(float decayAmount, int32 decayItemIndex = -1)
	{
		if (!bIsDecayableItemInventory)
			return;
		for (int i = Items.Num() - 1; i >= 0; i--)
		{
			if (decayItemIndex >= 0)
				Items[i].DecayItemAt(decayAmount, decayItemIndex);
			else
				Items[i].DecayAllItems(decayAmount);
			if (Items[i].GetQuantity() <= 0)
				Items.RemoveAt(i);
		}
	}
	bool GetItemInfos(FPrimaryAssetId ItemID, FInventoryItem& OutItem)
	{
		if (!ItemID.IsValid())
			return false;
		const int32 index = Items.IndexOfByPredicate([ItemID](const FInventoryItem& item)->bool{ return item.ItemID == ItemID; });
		if (index == INDEX_NONE)
			return false;
		auto item = Items[index];
		OutItem = item;
		return true;
	}
	bool SetInventorySize(uint8 Columns, uint8 Rows, bool bPrioritizeInventorySize = false)
	{
		auto newSize = FUInt82(Columns, Rows);
		if (newSize == InventorySlots)
			return false;
		if (!bPrioritizeInventorySize)
		{
			TArray<FUInt82> currentItemOccupation;
			for (const auto item : Items)
				BreakOccupationIntoSlots(FUInt84(item.ItemLocation, item.ItemOccupation), currentItemOccupation);
			if (currentItemOccupation.Num() > 0)
			{
				uint8 maxX = UPulseSystemLibrary::MaxInCollectionBy<FUInt82, uint8>(currentItemOccupation, [](const FUInt82& entry) { return entry.X; }) + 1;
				uint8 maxY = UPulseSystemLibrary::MaxInCollectionBy<FUInt82, uint8>(currentItemOccupation, [](const FUInt82& entry) { return entry.Y; }) + 1;
				if (Columns < maxX || Rows < maxY)
					return false;
			}
		}
		if (newSize.X < InventorySlots.X || newSize.Y < InventorySlots.Y)
		{
			InventorySlots = newSize;
			ReorganizeItems();
		}
		InventorySlots = newSize;
		return true;
	}
	bool GetPossibleMovements(const FInventoryItem& Item, TArray<FUInt82>& OutLocations) const
	{
		TArray<FUInt82> availableSlots = GetSlotArray();
		return GetClusteredSlots(Item.ItemOccupation, availableSlots, OutLocations);
	}
	bool MoveItem(const FPrimaryAssetId& ItemID, const FUInt82 NewLocation, bool bMoveOverlappingItems = false)
	{
		if (!ItemID.IsValid())
			return false;
		const int32 index = Items.IndexOfByPredicate([ItemID](const FInventoryItem& item)->bool{ return item.ItemID == ItemID; });
		if (index == INDEX_NONE)
			return false;
		TArray<FUInt82> possibleOutLocations;
		if (!GetPossibleMovements(Items[index], possibleOutLocations))
			return false;
		if (!possibleOutLocations.Contains(NewLocation))
			return false;
		TArray<FPrimaryAssetId> overlappingItems;
		auto tempItem = Items[index];
		tempItem.ItemLocation = NewLocation;
		if (!GetOverlappingItems(tempItem, overlappingItems))
		{
			Items[index].ItemLocation = NewLocation;
			return true;
		}
		if (!bMoveOverlappingItems)
			return false;
		Items[index].ItemLocation = NewLocation;
		TArray<FInventoryItem> tempItems;
		for (const auto id: overlappingItems)
		{
			const int32 idx = Items.IndexOfByPredicate([id](const FInventoryItem& item)->bool{ return item.ItemID == id; });
			if (idx == INDEX_NONE)
				continue;
			tempItems.Add(Items[idx]);
			Items.RemoveAt(idx);
		}
		for (auto item: tempItems)
		{
			AddItem(item, item.Quantity);
		}
		return true;
	}
	bool CombineItems(const TArray<FInventoryItemCombineIngredient>& Ingredients, const FInventoryItem& ResultItem, uint8 ResultQty = 1)
	{
		if (Ingredients.Num() < 2)
			return false;
		if (!VerifyAssetType(ResultItem.ItemID))
			return false;
		auto ItemsBKP = Items;
		TArray<FUInt82> indexes;
		for (const auto ing: Ingredients)
		{
			if (ing.Quantity <= 0)
				return false;
			if (!ing.ItemID.IsValid())
				return false;
			if (!VerifyAssetType(ing.ItemID))
				return false;
			const int32 index = Items.IndexOfByPredicate([ing](const FInventoryItem& item)->bool{ return item.ItemID == ing.ItemID; });
			if (index == INDEX_NONE)
				return false;
			if (Items[index].Quantity < ing.Quantity)
				return false;
			indexes.Add({index, ing.Quantity} );
		}
		for (const FUInt82& idx : indexes)
		{
			int32 consumed = 0;
			if (!ConsumeItem(Items[idx.X].ItemID, idx.Y, consumed))
			{
				Items = ItemsBKP;
				return false;
			}
		}
		if (!AddItem(ResultItem, ResultQty, true))
		{
			Items = ItemsBKP;
		}
		return true;
	}
};

USTRUCT(BlueprintType)
struct FInventoryContainer
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
	TMap<uint8, FInventory> InventoryPerType;

	FInventory* GetInventoryPtr(FGuid Guid)
	{
		for (const auto pair: InventoryPerType)
			if (pair.Value.UID == Guid.ToString())
				return &InventoryPerType[pair.Key];
		return nullptr;
	}
	FInventory* GetInventoryPtrByType(uint8 type)
	{
		if (InventoryPerType.Contains(type))
			return &InventoryPerType[type];
		return nullptr;
	}
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryContainerChanged, UInventoryComponent*, ComponentChanging, FInventoryContainer, Container);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, FInventory, ChangedInventory);

UCLASS(Blueprintable, BlueprintType)
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UInventoryComponent();

private:
	bool IsIDInventory() const { return InventoryType == EInventoryContainerType::IDObject; }
	FInventoryContainer* GetInventoryPtr();
	FInventoryContainer* GetLocalInventoryPtr();
	FInventoryContainer _localContainer;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	EInventoryContainerType InventoryType;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta=(EditCondition = IsIDInventory))
	bool bUseCustomID = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta=(EditCondition = IsIDInventory))
	FPrimaryAssetId CustomAssetID;

	void BeginPlay() override;
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 LocalPlayerID = 0;

public:
	
	// Get an Inventory's Id by its Type
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetInventory(uint8 Type, FGuid& OutInventoryID);
	
	// Get the location (X,Y) the amount of additional space (Z,W) the item take in an inventory. 
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemSlot(FGuid InventoryID, FPrimaryAssetId ItemID, FUInt84& OutLocationAndOccupation);
	
	// Get all the still free locations in an inventory
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetAllFreeSlots(FGuid InventoryID, TArray<FUInt82>& OutFreeSlots);
	
	// Check if an item with a set size can fit in a certain location in an inventory
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanFitItem(FGuid InventoryID, const FUInt82& ItemLocation, const FUInt82& ItemOccupation);
	
	// Check if an inventory is valid
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool IsValidInventory(FGuid InventoryID);
	
	// Get the detail about an item in an inventory
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemInfos(FGuid InventoryID, FPrimaryAssetId ItemID, FInventoryItem& OutItem);
	
	// Check if moving an item within an inventory is possible and Get the possible moves locations
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetPossibleItemMovements(FGuid InventoryID, const FInventoryItem& Item, TArray<FUInt82>& OutLocations);

	
	// Change the Inventory container this component is pointing at 
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ChangeContainerType(EInventoryContainerType newType, bool bCustomId = false, FPrimaryAssetId CustomId = FPrimaryAssetId());
	
	// Add a new inventory by type.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddInventory(TSubclassOf<UBasePulseBaseItemAsset> ItemType, uint8 Type, uint8 Columns, uint8 Rows = 1, bool bDecayableItemInventory = false);
	
	// Add a new inventory by type.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveInventory(uint8 Type);
	
	// Add a new item or add up to its quantity, in an inventory. Attempt reorganisation  will try to reorganize items in this inventory to fit the new item. 
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FGuid InventoryID, FInventoryItem Item, int Quantity = 1, bool bAttemptReorganisation = false);
	
	// Consume a quantity and remove the depleted item. OutConsumed is the actual quantity consumed.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeItem(FGuid InventoryID, FPrimaryAssetId ItemID, int Quantity, int32& OutConsumedQuantity);
	
	// Remove an item from an inventory
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FGuid InventoryID, FPrimaryAssetId ItemID);
	
	// Try to decay an item by a certain percentage (0-1) if it allows it (the inventory must be for decayable items)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool DecayItem(FGuid InventoryID, FPrimaryAssetId ItemID, float DecayAmount, int32 DecayItemIndex = -1);
	
	// Try to decay all items by a certain percentage (0-1) if they allow it (the inventory must be for decayable items)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DecayAllItems(FGuid InventoryID, float DecayAmount, int32 DecayItemIndex = -1);
	
	// Change the size of an inventory. Prioritize Inventory Size can remove existing item whenever the inventory shrinks and can't fit all the items anymore
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool SetInventorySize(FGuid InventoryID, uint8 Columns, uint8 Rows, bool bPrioritizeInventorySize = false);
	
	// Move and Item within its inventory. Move Overlapping Items will try to relocate the items already at that place.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(FGuid InventoryID, const FPrimaryAssetId& ItemID, const FUInt82 NewLocation, bool bMoveOverlappingItems = false);
	
	// Try to combine 2 or more ingredients into a resulting item, consuming them in the process. 
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CombineItems(FGuid InventoryID, const TArray<FInventoryItemCombineIngredient>& Ingredients, const FInventoryItem& ResultItem, uint8 ResultQty = 1);
};

#pragma endregion


/**
 * Manage the inventories
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseInventoryManager : public UPulseSubModuleBase
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<int32, FInventoryContainer> _PlayersInventories;
	UPROPERTY()
	TMap<FPrimaryAssetId, FInventoryContainer> _AssetInventories;
	UPROPERTY()
	TMap<AActor*, FInventoryContainer> _ActorInventories;

	bool _bCanSave = false;
	bool _bCanReplicate = false;
	
	UFUNCTION()
	void AffectBoundInventories(FInventory ChangedInventory);

public:

	static UPulseInventoryManager* Get(const UObject* WorldContextObject);
	static FInventoryContainer* GetPlayerContainerPtr(const UObject* WorldContextObject, const int32 playerID);
	static FInventoryContainer* GetAssetContainerPtr(const UObject* WorldContextObject, const FPrimaryAssetId& AssetID);
	static FInventoryContainer* GetActorContainerPtr(const UObject* WorldContextObject, const AActor* Actor);
	bool GetInventories(std::function<bool(const FInventory&)> SelectFunc, TArray<FInventory*>& OutInventoryPtrs);

	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
	
public:

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnInventoryContainerChanged OnInventoryContainerChanged;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnInventoryChanged OnInventoryChanged;
	
	
	// Bind a source inventory to a target.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool BindInventories(FGuid SourceInventoryID, FGuid TargetInventoryID);
	
	
	// Bind a source inventory to a target.
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UnbindInventories(FGuid SourceInventoryID);
	
};
