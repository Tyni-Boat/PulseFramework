// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/Blueprints/PulseTweenBPActionVector2D.h"
#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"


UPulseTweenBPActionVector2D* UPulseTweenBPActionVector2D::TweenVector2D(FVector2D Start, FVector2D End, float DurationSecs,
                                                                  EPulseEase EaseType, float EaseParam1, float EaseParam2, float Delay, int Loops, float LoopDelay, bool bYoyo, float YoyoDelay,
                                                                  bool bCanTickDuringPause, bool bUseGlobalTimeDilation)
{
	UPulseTweenBPActionVector2D* BlueprintNode = NewObject<UPulseTweenBPActionVector2D>();
	BlueprintNode->SetSharedTweenProperties(
		DurationSecs, Delay, Loops, LoopDelay, bYoyo, YoyoDelay, bCanTickDuringPause, bUseGlobalTimeDilation);
	BlueprintNode->EaseType = EaseType;
	BlueprintNode->Start = Start;
	BlueprintNode->End = End;
	BlueprintNode->EaseParam1 = EaseParam1;
	BlueprintNode->EaseParam2 = EaseParam2;
	return BlueprintNode;
}

UPulseTweenBPActionVector2D* UPulseTweenBPActionVector2D::TweenVector2DCustomCurve(FVector2D Start, FVector2D End, float DurationSecs,
	UCurveFloat* Curve, float Delay, int Loops, float LoopDelay, bool bYoyo, float YoyoDelay, bool bCanTickDuringPause,
	bool bUseGlobalTimeDilation)
{
	UPulseTweenBPActionVector2D* BlueprintNode = NewObject<UPulseTweenBPActionVector2D>();
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

PulseTweenInstance* UPulseTweenBPActionVector2D::CreateTween()
{
	return PulseTweenSystemCore::Play(
		Start, End, [&](FVector2D t) { ApplyEasing.Broadcast(t); }, DurationSecs, EaseType);
}

PulseTweenInstance* UPulseTweenBPActionVector2D::CreateTweenCustomCurve()
{
	return PulseTweenSystemCore::Play(
		0, 1,
		[&](float t)
		{
			float EasedTime = CustomCurve->GetFloatValue(t);
			FVector2D EasedValue = FMath::Lerp(Start, End, EasedTime);
			ApplyEasing.Broadcast(EasedValue);
		},
		DurationSecs, EaseType);
}