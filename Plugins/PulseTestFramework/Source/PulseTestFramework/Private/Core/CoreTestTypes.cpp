// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Core/CoreTestTypes.h"

#include "Kismet/KismetSystemLibrary.h"

FName UCoreTestSubModule::GetSubModuleName() const
{
	return "TestSubmodule";
}

bool UCoreTestSubModule::WantToTick() const
{
	return true;
}

bool UCoreTestSubModule::TickWhenPaused() const
{
	return false;
}

void UCoreTestSubModule::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
}

void UCoreTestSubModule::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
}

void UCoreTestSubModule::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}

FName UCoreTestSubModule2::GetSubModuleName() const
{
	return "TestSubmodule2";
}

bool UCoreTestSubModule2::WantToTick() const
{
	return true;
}

bool UCoreTestSubModule2::TickWhenPaused() const
{
	return false;
}

void UCoreTestSubModule2::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
}

void UCoreTestSubModule2::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
}

void UCoreTestSubModule2::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
}

FName UCoreTestModule::GetModuleName() const
{
	return "TestCoreModule";
}

TStatId UCoreTestModule::GetStatId() const
{
	return Super::GetStatId();
}

UWorld* UCoreTestModule::GetTickableGameObjectWorld() const
{
	return Super::GetTickableGameObjectWorld();
}

ETickableTickType UCoreTestModule::GetTickableTickType() const
{
	return ETickableTickType::Always;
}

bool UCoreTestModule::IsTickable() const
{
	return true;
}

bool UCoreTestModule::IsTickableInEditor() const
{
	return false;
}

bool UCoreTestModule::IsTickableWhenPaused() const
{
	return false;
}

void UCoreTestModule::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UCoreTestModule::Deinitialize()
{
	Super::Deinitialize();
}

void UCoreTestModule::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TArray<TSubclassOf<UPulseSubModuleBase>> UCoreTestModule::GetSubmodulesTypes() const
{
	return {
		UCoreTestSubModule::StaticClass(),
		UCoreTestSubModule2::StaticClass()
	};
}
