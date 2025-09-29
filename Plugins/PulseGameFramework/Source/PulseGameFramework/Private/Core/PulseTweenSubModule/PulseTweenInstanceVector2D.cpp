// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenInstanceVector2D.h"

void PulseTweenInstanceVector2D::Initialize(
	FVector2D InStart, FVector2D InEnd, TFunction<void(FVector2D)> InOnUpdate, float InDurationSecs, EPulseEase InEaseType)
{
	this->StartValue = InStart;
	this->EndValue = InEnd;
	this->OnUpdate = MoveTemp(InOnUpdate);
	this->InitializeSharedMembers(InDurationSecs, InEaseType);
}

void PulseTweenInstanceVector2D::ApplyEasing(float EasedPercent)
{
	OnUpdate(FMath::Lerp<FVector2D>(StartValue, EndValue, EasedPercent));
}
