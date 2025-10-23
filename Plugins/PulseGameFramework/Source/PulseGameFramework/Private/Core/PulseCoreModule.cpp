// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/PulseCoreModule.h"

#include "Core/InventoryManager/PulseInventoryManager.h"
#include "Core/OperationModifierSubModule/OperationModifierSubModule.h"
#include "Core/PoolingSubModule/PulsePoolingManager.h"
#include "Core/PulseNetworkingModule/PulseNetManager.h"
#include "Core/PulseResourceManagement/PulseAssetManager.h"
#include "Core/PulseTweenSubModule/PulseTweenSubModule.h"
#include "Core/SaveGameSubModule/PulseSaveManager.h"
#include "Core/TimeOfDaySubModule/PulseTimeOfDayManager.h"


UCoreProjectSetting* UPulseCoreModule::GetProjectConfig() const
{
	return GetMutableDefault<UCoreProjectSetting>();
}

FName UPulseCoreModule::GetModuleName() const
{
	return Super::GetModuleName();
}

TStatId UPulseCoreModule::GetStatId() const
{
	return Super::GetStatId();
}

ETickableTickType UPulseCoreModule::GetTickableTickType() const
{
	return ETickableTickType::Always;
}

bool UPulseCoreModule::IsTickable() const
{
	return true;
}

bool UPulseCoreModule::IsTickableWhenPaused() const
{
	return true;
}

void UPulseCoreModule::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPulseCoreModule::Deinitialize()
{
	Super::Deinitialize();
}

void UPulseCoreModule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TArray<TSubclassOf<UPulseSubModuleBase>> UPulseCoreModule::GetSubmodulesTypes() const
{
	return  {
		UPulseTweenSubModule::StaticClass(),
		UPulsePoolingManager::StaticClass(),
		UPulseNetManager::StaticClass(),
		UPulseAssetManagerSubModule::StaticClass(),
		UPulseSaveManager::StaticClass(),
		UOperationModifierSubModule::StaticClass(),
		UPulseTimeOfDayManager::StaticClass(),
		UPulseInventoryManager::StaticClass(),
	};
}

UPulseSaveManager* UPulseCoreModule::GetSaveManager()
{
	return GetSubModule<UPulseSaveManager>();
}

UPulseTimeOfDayManager* UPulseCoreModule::GetTimeOfDayManager()
{
	return GetSubModule<UPulseTimeOfDayManager>();
}

UOperationModifierSubModule* UPulseCoreModule::GetBuffManager()
{
	return GetSubModule<UOperationModifierSubModule>();
}

UPulseNetManager* UPulseCoreModule::GetNetManager()
{
	return GetSubModule<UPulseNetManager>();
}

UPulseInventoryManager* UPulseCoreModule::GetInventoryManager()
{
	return GetSubModule<UPulseInventoryManager>();
}
