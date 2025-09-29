// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenInstanceQuat.h"

void PulseTweenInstanceQuat::Initialize(
	FQuat InStart, FQuat InEnd, TFunction<void(FQuat)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType)
{
	this->StartValue = InStart;
	this->EndValue = InEnd;
	this->OnUpdate = MoveTemp(InOnUpdate);
	this->InitializeSharedMembers(InDurationSecs, InEaseType);
}

void PulseTweenInstanceQuat::ApplyEasing(float EasedPercent)
{
	OnUpdate(FQuat::Slerp(StartValue, EndValue, EasedPercent));
}
