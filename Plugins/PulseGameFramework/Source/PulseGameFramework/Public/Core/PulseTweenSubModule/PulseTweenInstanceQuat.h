// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "PulseTweenInstance.h"

class PULSEGAMEFRAMEWORK_API PulseTweenInstanceQuat : public PulseTweenInstance
{
public:
	FQuat StartValue;
	FQuat EndValue;
	TFunction<void(FQuat)> OnUpdate;

	void Initialize(FQuat InStart, FQuat InEnd, TFunction<void(FQuat)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType);

protected:
	virtual void ApplyEasing(float EasedPercent) override;
};
