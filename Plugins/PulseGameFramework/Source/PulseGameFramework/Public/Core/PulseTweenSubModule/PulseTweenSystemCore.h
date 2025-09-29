// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "PulseEasing.h"
#include "PulseTweenInstance.h"
#include "PulseTweenInstanceFloat.h"
#include "PulseTweenInstanceQuat.h"
#include "PulseTweenInstanceVector.h"
#include "PulseTweenInstanceVector2D.h"
#include "PulseTweenManager.h"

PULSEGAMEFRAMEWORK_API DECLARE_LOG_CATEGORY_EXTERN(LogPulseTween, Log, All)

class PULSEGAMEFRAMEWORK_API PulseTweenSystemCore
{
private:
	static PulseTweenManager<PulseTweenInstanceFloat>* FloatTweenManager;
	static PulseTweenManager<PulseTweenInstanceVector>* VectorTweenManager;
	static PulseTweenManager<PulseTweenInstanceVector2D>* Vector2DTweenManager;
	static PulseTweenManager<PulseTweenInstanceQuat>* QuatTweenManager;

	static int NumReservedFloat;
	static int NumReservedVector;
	static int NumReservedVector2D;
	static int NumReservedQuat;

public:
	static void Initialize();
	static void Deinitialize();

	/**
	 * @brief Ensure there are at least this many tweens in the recycle pool. Call this at game startup to increase your initial
	 * capacity for each type of tween, if you know you will be needing more and don't want to allocate memory during the game.
	 */
	static void EnsureCapacity(int NumFloatTweens, int NumVectorTweens, int NumVector2DTweens, int NumQuatTweens);
	/**
	 * @brief Add more tweens to the recycle pool. Call this at game startup to increase your initial capacity if you know you will
	 * be needing more and don't want to allocate memory during the game.
	 */
	static void EnsureCapacity(int NumTweens);
	static void Update(float UnscaledDeltaSeconds, float DilatedDeltaSeconds, bool bIsGamePaused);
	static void ClearActiveTweens();

	/**
	 * @brief compare the current reserved memory for tweens against the initial capacity, to tell the developer if initial capacity needs to be increased
	 */
	static int CheckTweenCapacity();

	/**
	 * @brief Convenience function for UFCEasing::Ease()
	 */
	static float Ease(float t, EPulseEase EaseType);

	static PulseTweenInstanceFloat* Play(
		float Start, float End, TFunction<void(float)> OnUpdate, float DurationSecs, EPulseEase EaseType = EPulseEase::OutQuad);

	static PulseTweenInstanceVector* Play(
		FVector Start, FVector End, TFunction<void(FVector)> OnUpdate, float DurationSecs, EPulseEase EaseType = EPulseEase::OutQuad);

	static PulseTweenInstanceVector2D* Play(FVector2D Start, FVector2D End, TFunction<void(FVector2D)> OnUpdate, float DurationSecs,
		EPulseEase EaseType = EPulseEase::OutQuad);

	static PulseTweenInstanceQuat* Play(
		FQuat Start, FQuat End, TFunction<void(FQuat)> OnUpdate, float DurationSecs, EPulseEase EaseType = EPulseEase::OutQuad);
};
