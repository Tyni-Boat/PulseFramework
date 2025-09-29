// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenInstanceFloat.h"

void PulseTweenInstanceFloat::Initialize(
	float InStart, float InEnd, TFunction<void(float)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType)
{
	this->StartValue = InStart;
	this->EndValue = InEnd;
	this->OnUpdate = MoveTemp(InOnUpdate);
	this->InitializeSharedMembers(InDurationSecs, InEaseType);
}

void PulseTweenInstanceFloat::ApplyEasing(float EasedPercent)
{
	OnUpdate(FMath::Lerp<float>(StartValue, EndValue, EasedPercent));
}
