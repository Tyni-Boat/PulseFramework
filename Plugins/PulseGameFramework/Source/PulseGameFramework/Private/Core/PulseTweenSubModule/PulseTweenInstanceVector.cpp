// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenInstanceVector.h"

void PulseTweenInstanceVector::Initialize(
	FVector InStart, FVector InEnd, TFunction<void(FVector)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType)
{
	this->StartValue = InStart;
	this->EndValue = InEnd;
	this->OnUpdate = MoveTemp(InOnUpdate);
	this->InitializeSharedMembers(InDurationSecs, InEaseType);
}

void PulseTweenInstanceVector::ApplyEasing(float EasedPercent)
{
	OnUpdate(FMath::Lerp<FVector>(StartValue, EndValue, EasedPercent));
}
