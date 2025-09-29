// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "PulseTweenInstance.h"

class PULSEGAMEFRAMEWORK_API PulseTweenInstanceFloat : public PulseTweenInstance
{
public:
	float StartValue;
	float EndValue;
	TFunction<void(float)> OnUpdate;

	void Initialize(float InStart, float InEnd, TFunction<void(float)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType);

protected:
	virtual void ApplyEasing(float EasedPercent) override;
};
