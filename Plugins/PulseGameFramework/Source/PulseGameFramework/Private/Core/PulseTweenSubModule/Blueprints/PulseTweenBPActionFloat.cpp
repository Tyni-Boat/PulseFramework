// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/Blueprints/PulseTweenBPActionFloat.h"
#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"


UPulseTweenBPActionFloat* UPulseTweenBPActionFloat::TweenFloat(float Start, float End, float DurationSecs, EPulseEase EaseType,
                                                         float EaseParam1, float EaseParam2, float Delay, int Loops, float LoopDelay, bool bYoyo, float YoyoDelay,
                                                         bool bCanTickDuringPause, bool bUseGlobalTimeDilation)
{
	UPulseTweenBPActionFloat* BlueprintNode = NewObject<UPulseTweenBPActionFloat>();
	BlueprintNode->SetSharedTweenProperties(
		DurationSecs, Delay, Loops, LoopDelay, bYoyo, YoyoDelay, bCanTickDuringPause, bUseGlobalTimeDilation);
	BlueprintNode->EaseType = EaseType;
	BlueprintNode->Start = Start;
	BlueprintNode->End = End;
	BlueprintNode->EaseParam1 = EaseParam1;
	BlueprintNode->EaseParam2 = EaseParam2;
	return BlueprintNode;
}

UPulseTweenBPActionFloat* UPulseTweenBPActionFloat::TweenFloatCustomCurve(float Start, float End, float DurationSecs, UCurveFloat* Curve,
	float Delay, int Loops, float LoopDelay, bool bYoyo, float YoyoDelay, bool bCanTickDuringPause, bool bUseGlobalTimeDilation)
{
	UPulseTweenBPActionFloat* BlueprintNode = NewObject<UPulseTweenBPActionFloat>();
	BlueprintNode->SetSharedTweenProperties(
		DurationSecs, Delay, Loops, LoopDelay, bYoyo, YoyoDelay, bCanTickDuringPause, bUseGlobalTimeDilation);
	BlueprintNode->CustomCurve = Curve;
	BlueprintNode->bUseCustomCurve = true;
	BlueprintNode->Start = Start;
	BlueprintNode->End = End;
	BlueprintNode->EaseParam1 = 0;
	BlueprintNode->EaseParam2 = 0;
	return BlueprintNode;
}

PulseTweenInstance* UPulseTweenBPActionFloat::CreateTween()
{
	return PulseTweenSystemCore::Play(
		Start, End, [&](float t) { ApplyEasing.Broadcast(t); }, DurationSecs, EaseType);
}

PulseTweenInstance* UPulseTweenBPActionFloat::CreateTweenCustomCurve()
{
	return PulseTweenSystemCore::Play(
		0, 1,
		[&](float t)
		{
			float EasedTime = CustomCurve->GetFloatValue(t);
			float EasedValue = FMath::Lerp(Start, End, EasedTime);
			ApplyEasing.Broadcast(EasedValue);
		},
		DurationSecs, EaseType);
}
