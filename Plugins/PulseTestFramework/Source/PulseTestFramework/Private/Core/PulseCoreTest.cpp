// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Misc/AutomationTest.h"
#include "Core/CoreTestTypes.h"
#include "Core/PulseSystemLibrary.h"
#include "Core/InventoryManager/PulseInventoryManager.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreTypeTest, "PulseTest.Core.TypeTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreSerializationTest, "PulseTest.Core.SerializationTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreInventoryTest, "PulseTest.Core.InventoryTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCoreTypeTest::RunTest(const FString& Parameters)
{
	auto gameInstance = NewObject<UGameInstance>();
	gameInstance->Init();
	auto module = UPulseSystemLibrary::GetPulseModule<UCoreTestModule>(gameInstance);
	auto modName = module != nullptr ? module->GetModuleName() : "";
	auto submodule = UPulseSystemLibrary::GetPulseSubModule<UCoreTestModule, UCoreTestSubModule>(gameInstance);
	// module != nullptr ? module->GetSubModule<UCoreTestSubModule>() : nullptr;
	auto subModName = submodule != nullptr ? submodule->GetSubModuleName() : "";
	float moduleCfgVal = module && module->GetProjectConfig() != nullptr ? module->GetProjectConfig()->ModuleTestValue : 0.0f;
	float subModCfgVal = submodule != nullptr && submodule->GetOwningModule() && Cast<UCoreTestModule>(submodule->GetOwningModule()) && Cast<
		                     UCoreTestModule>(submodule->GetOwningModule())
	                     ->GetProjectConfig()
		                     ? Cast<UCoreTestModule>(submodule->GetOwningModule())
		                       ->GetProjectConfig()->SubModuleTestValue
		                     : 0.0f;
	int result = 0;
	result += TestNotNull(TEXT("Test Module Non Null:"), module);
	result += TestEqualSensitive(TEXT("Test Module Name:"), modName.ToString(), TEXT("TestCoreModule")); //TestCoreModule
	result += TestNotNull(TEXT("Test Sub-Module Non Null"), submodule);
	result += TestEqualSensitive(TEXT("Test Sub-Module Name:"), subModName.ToString(), TEXT("TestSubmodule")); //TestSubmodule
	result += TestEqual(TEXT("Module Config value:"), moduleCfgVal, 154.0f); //154
	result += TestEqual(TEXT("Sub-Module Config value:"), subModCfgVal, 256.0f); //256
	gameInstance->Shutdown();
	return result >= 6;
}


bool FCoreSerializationTest::RunTest(const FString& Parameters)
{
	auto testObject_1 = NewObject<UCoreTestModuleConfig>();
	auto testObject_2 = NewObject<UCoreTestModuleConfigInherited>();
	testObject_1->ModuleTestValue = 2500;
	testObject_1->Struct.TestValue = 3345;
	testObject_2->ModuleTestValue = 800;
	testObject_2->Struct.TestValue = 12;
	TArray<uint8> byteArray;
	int result = 0;
	UPulseSystemLibrary::SerializeObjectToBytes(testObject_1, byteArray);
	UPulseSystemLibrary::DeserializeObjectFromBytes(byteArray, testObject_2);
	result += TestNotNull(TEXT("Serialized Object"), testObject_1);
	result += TestNotNull(TEXT("Deserialized Object"), testObject_2);
	result += TestEqual(TEXT("Compare value"), testObject_2->ModuleTestValue, 2500);
	result += TestEqual(TEXT("Compare Struct"), testObject_2->Struct.TestValue, 3345.0f);
	return result >= 3;
}


bool FCoreInventoryTest::RunTest(const FString& Parameters)
{
	FInventory Inventory = FInventory().OfSize(6, 8);
	FInventoryItem Item1, Item2, Item3, Item4, Item5;
	Item1.ItemID = FPrimaryAssetId(FPrimaryAssetType("Weapon"), "Bazooka");
	Item1.ItemOccupation = FUInt82(3, 4);
	Item2.ItemID = FPrimaryAssetId(FPrimaryAssetType("Weapon"), "Sniper");
	Item2.ItemOccupation = FUInt82(5, 2);
	Item3.ItemID = FPrimaryAssetId(FPrimaryAssetType("Weapon"), "RocketLauncher");
	Item3.ItemOccupation = FUInt82(3, 1);
	Item4.ItemID = FPrimaryAssetId(FPrimaryAssetType("Ammo"), "NuclearHead");
	Item4.ItemOccupation = FUInt82(0, 0);
	Item5.ItemID = FPrimaryAssetId(FPrimaryAssetType("BFG"), "Nuke RPG");
	Item5.ItemOccupation = FUInt82(6, 1);
	int result = 0;
	result += TestTrue(TEXT("Is Valid Inventory"), Inventory.IsValidInventory());
	TArray<FUInt82> freeSlots;
	Inventory.GetAllFreeSlots(freeSlots);
	result += TestEqual(TEXT("Free Item Slots"), freeSlots.Num(), 48);
	Inventory.AddItem(Item1);
	Inventory.AddItem(Item2, 1);
	Inventory.AddItem(Item3);
	Inventory.AddItem(Item4, 5);
	Inventory.AddItem(Item5, 3);
	result += TestEqual(TEXT("Item Count"), Inventory.Items.Num(), 4);
	FInventoryItem outItem;
	Inventory.GetItemInfos(Item1.ItemID, outItem);
	result += TestEqual(TEXT("Item 1 Location"), outItem.ItemLocation, FUInt82(0,0));
	Inventory.GetItemInfos(Item2.ItemID, outItem);
	result += TestEqual(TEXT("Item 2 Location"), outItem.ItemLocation, FUInt82(0,5));
	Inventory.GetItemInfos(Item3.ItemID, outItem);
	result += TestEqual(TEXT("Item 3 Location"), outItem.ItemLocation, FUInt82(4,0));
	Inventory.GetItemInfos(Item4.ItemID, outItem);
	result += TestEqual(TEXT("Item 4 Location"), outItem.ItemLocation, FUInt82(4,4));
	Inventory.GetAllFreeSlots(freeSlots);
	result += TestEqual(TEXT("Remaining free Slots"), freeSlots.Num(), 1);
	result += TestEqual(TEXT("Last free Slot location"), freeSlots[0], FUInt82(5,4));
	Inventory.MoveItem(Item1.ItemID, {2,3}, true);
	Inventory.GetItemInfos(Item1.ItemID, outItem);
	result += TestEqual(TEXT("Item 1 New Location"), outItem.ItemLocation, FUInt82(2,3));
	Inventory.GetItemInfos(Item2.ItemID, outItem);
	result += TestEqual(TEXT("Item 2 New Location"), outItem.ItemLocation, FUInt82(0,0));
	Inventory.GetItemInfos(Item3.ItemID, outItem);
	result += TestEqual(TEXT("Item 3 New Location"), outItem.ItemLocation, FUInt82(0,3));
	Inventory.GetItemInfos(Item4.ItemID, outItem);
	result += TestEqual(TEXT("Item 4 New Location"), outItem.ItemLocation, FUInt82(0,7));
	Inventory.GetAllFreeSlots(freeSlots);
	result += TestEqual(TEXT("New Remaining free Slots 1"), freeSlots.Num(), 1);
	result += TestEqual(TEXT("New Last free Slot location"), freeSlots[0], FUInt82(1,7));
	bool setSize = Inventory.SetInventorySize(6,7);
	result += TestFalse(TEXT("Should Fail to Resize"), setSize);
	setSize = Inventory.SetInventorySize(6,7, true);
	result += TestTrue(TEXT("Should Successfully Resize"), setSize);
	Inventory.GetAllFreeSlots(freeSlots);
	result += TestEqual(TEXT("New Item Count"), Inventory.Items.Num(), 3);
	result += TestEqual(TEXT("New Remaining free Slots 2"), freeSlots.Num(), 3);
	FInventoryItemCombineIngredient Ingredient1 = {Item4.ItemID, 2};
	FInventoryItemCombineIngredient Ingredient2 = {Item2.ItemID, 1};
	result += TestTrue(TEXT("Should Successfully Combine"),  Inventory.CombineItems({Ingredient1, Ingredient2}, Item5, 1));
	result += TestFalse(TEXT("Should Fail to Combine"), Inventory.CombineItems({Ingredient1, Ingredient2}, Item5, 1));
	return result >= 21;
}


#endif
