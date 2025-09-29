// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenSubModule.h"

#include "Core/PulseModuleBase.h"
#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"


FName UPulseTweenSubModule::GetSubModuleName() const
{
	return Super::GetSubModuleName();
}

bool UPulseTweenSubModule::WantToTick() const
{
	return true;
}

bool UPulseTweenSubModule::TickWhenPaused() const
{
	return true;
}

void UPulseTweenSubModule::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	LastTickedFrame = GFrameCounter;

#if ENGINE_MAJOR_VERSION < 5
	if (GetWorld() != nullptr)
	{
		LastRealTimeSeconds = GetWorld()->RealTimeSeconds;
	}
#endif

#if WITH_EDITOR
	PulseTweenSystemCore::ClearActiveTweens();
#endif
}

void UPulseTweenSubModule::DeinitializeSubModule()
{
	Super::DeinitializeSubModule();
#if WITH_EDITOR
	PulseTweenSystemCore::CheckTweenCapacity();
	PulseTweenSystemCore::ClearActiveTweens();
#endif
}

void UPulseTweenSubModule::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
	if (LastTickedFrame < GFrameCounter)
	{
		LastTickedFrame = GFrameCounter;
#if ENGINE_MAJOR_VERSION < 5
		float DeltaRealTimeSeconds = GetWorld()->RealTimeSeconds - LastRealTimeSeconds;
		PulseTweenSystemCore::Update(DeltaRealTimeSeconds, GetWorld()->DeltaTimeSeconds, GetWorld()->IsPaused());
		LastRealTimeSeconds = GetWorld()->RealTimeSeconds;
#else
		PulseTweenSystemCore::Update(DeltaTime, DeltaTime * CurrentTimeDilation, bIsGamePaused);
#endif
	}
}
