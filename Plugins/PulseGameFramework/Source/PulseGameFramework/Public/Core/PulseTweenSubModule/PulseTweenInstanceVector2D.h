// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "PulseTweenInstance.h"

class PULSEGAMEFRAMEWORK_API PulseTweenInstanceVector2D : public PulseTweenInstance
{
public:
	FVector2D StartValue;
	FVector2D EndValue;
	TFunction<void(FVector2D)> OnUpdate;

	void Initialize(
		FVector2D InStart, FVector2D InEnd, TFunction<void(FVector2D)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType);

protected:
	virtual void ApplyEasing(float EasedPercent) override;
};
