// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "Core/PulseSubModuleBase.h"
#include "PulseTweenSubModule.generated.h"

UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseTweenSubModule : public UPulseSubModuleBase
{
	GENERATED_BODY()

private:
	UPROPERTY()
	uint64 LastTickedFrame = 0;
	UPROPERTY()
	float LastRealTimeSeconds;

public:	
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
};
