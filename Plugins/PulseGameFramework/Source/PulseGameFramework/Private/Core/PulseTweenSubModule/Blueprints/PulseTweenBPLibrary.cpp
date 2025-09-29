// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/Blueprints/PulseTweenBPLibrary.h"
#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"


float UPulseTweenBPLibrary::Ease(float t, EPulseEase EaseType)

{
	return PulseEasing::Ease(t, EaseType);
}

float UPulseTweenBPLibrary::EaseWithParams(float t, EPulseEase EaseType, float Param1, float Param2)
{
	return PulseEasing::EaseWithParams(t, EaseType, Param1, Param2);
}

void UPulseTweenBPLibrary::EnsureTweenCapacity(
	int NumFloatTweens, int NumVectorTweens, int NumVector2DTweens, int NumQuatTweens)
{
	PulseTweenSystemCore::EnsureCapacity(NumFloatTweens, NumVectorTweens, NumVector2DTweens, NumQuatTweens);
}
