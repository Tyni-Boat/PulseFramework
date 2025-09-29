// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "PulseTweenInstance.h"

class PULSEGAMEFRAMEWORK_API PulseTweenInstanceVector : public PulseTweenInstance
{
public:
	FVector StartValue;
	FVector EndValue;
	TFunction<void(FVector)> OnUpdate;

	void Initialize(FVector InStart, FVector InEnd, TFunction<void(FVector)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType);

protected:
	virtual void ApplyEasing(float EasedPercent) override;
};
