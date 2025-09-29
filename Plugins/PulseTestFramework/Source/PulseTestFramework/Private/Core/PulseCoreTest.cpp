// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Misc/AutomationTest.h"
#include "Core/CoreTestTypes.h"
#include "Core/PulseSystemLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreTypeTest, "PulseTest.Core.TypeTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoreSerializationTest, "PulseTest.Core.SerializationTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

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


#endif
