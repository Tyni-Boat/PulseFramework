// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/Blueprints/PulseTweenBPActionRotator.h"
#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"


UPulseTweenBPActionRotator* UPulseTweenBPActionRotator::TweenRotator(FRotator Start, FRotator End, float DurationSecs, EPulseEase EaseType,
                                                               float EaseParam1, float EaseParam2, float Delay, int Loops, float LoopDelay, bool bYoyo, float YoyoDelay,
                                                               bool bCanTickDuringPause, bool bUseGlobalTimeDilation)
{
	UPulseTweenBPActionRotator* BlueprintNode = NewObject<UPulseTweenBPActionRotator>();
	BlueprintNode->SetSharedTweenProperties(
		DurationSecs, Delay, Loops, LoopDelay, bYoyo, YoyoDelay, bCanTickDuringPause, bUseGlobalTimeDilation);
	BlueprintNode->EaseType = EaseType;
	BlueprintNode->Start = Start.Quaternion();
	BlueprintNode->End = End.Quaternion();
	BlueprintNode->EaseParam1 = EaseParam1;
	BlueprintNode->EaseParam2 = EaseParam2;
	return BlueprintNode;
}

UPulseTweenBPActionRotator* UPulseTweenBPActionRotator::TweenRotatorCustomCurve(FRotator Start, FRotator End, float DurationSecs,
	UCurveFloat* Curve, float Delay, int Loops, float LoopDelay, bool bYoyo, float YoyoDelay, bool bCanTickDuringPause,
	bool bUseGlobalTimeDilation)
{
	UPulseTweenBPActionRotator* BlueprintNode = NewObject<UPulseTweenBPActionRotator>();
	BlueprintNode->SetSharedTweenProperties(
		DurationSecs, Delay, Loops, LoopDelay, bYoyo, YoyoDelay, bCanTickDuringPause, bUseGlobalTimeDilation);
	BlueprintNode->CustomCurve = Curve;
	BlueprintNode->bUseCustomCurve = true;
	BlueprintNode->Start = Start.Quaternion();
	BlueprintNode->End = End.Quaternion();
	BlueprintNode->EaseParam1 = 0;
	BlueprintNode->EaseParam2 = 0;
	return BlueprintNode;
}

PulseTweenInstance* UPulseTweenBPActionRotator::CreateTween()
{
	return PulseTweenSystemCore::Play(
		Start, End, [&](FQuat t) { ApplyEasing.Broadcast(t.Rotator()); }, DurationSecs, EaseType);
}

PulseTweenInstance* UPulseTweenBPActionRotator::CreateTweenCustomCurve()
{
	return PulseTweenSystemCore::Play(
		0, 1,
		[&](float t)
		{
			float EasedTime = CustomCurve->GetFloatValue(t);
			FQuat EasedValue = FMath::Lerp(Start, End, EasedTime);
			ApplyEasing.Broadcast(EasedValue.Rotator());
		},
		DurationSecs, EaseType);
}