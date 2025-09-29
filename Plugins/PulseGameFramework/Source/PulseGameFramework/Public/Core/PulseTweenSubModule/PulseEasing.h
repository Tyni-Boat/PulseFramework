// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

UENUM(BlueprintType)
enum class EPulseEase : uint8
{
	Linear,
	Smoothstep,
	Stepped,
	InSine,
	OutSine,
	InOutSine,
	InQuad,
	OutQuad,
	InOutQuad,
	InCubic,
	OutCubic,
	InOutCubic,
	InQuart,
	OutQuart,
	InOutQuart,
	InQuint,
	OutQuint,
	InOutQuint,
	InExpo,
	OutExpo,
	InOutExpo,
	InCirc,
	OutCirc,
	InOutCirc,
	InElastic,
	OutElastic,
	InOutElastic,
	InBounce,
	OutBounce,
	InOutBounce,
	InBack,
	OutBack,
	InOutBack,
};

class PULSEGAMEFRAMEWORK_API PulseEasing
{
public:
	static float Ease(float t, EPulseEase EaseType);
	/**
	 * Ease with overriding parameters
	 * @param Param1 Elastic: Amplitude (1.0) / Back: Overshoot (1.70158) / Stepped: Steps (10) / Smoothstep: x0 (0)
	 * @param Param2 Elastic: Period (0.2) / Smoothstep: x1 (1)
	 */
	static float EaseWithParams(float t, EPulseEase EaseType, float Param1 = 0, float Param2 = 0);
	static float EaseLinear(float t);
	static float EaseSmoothstep(float t, float x0 = 0, float x1 = 1);
	static float EaseStepped(float t, int Steps = 10);
	static float EaseInSine(float t);
	static float EaseOutSine(float t);
	static float EaseInOutSine(float t);
	static float EaseInQuad(float t);
	static float EaseOutQuad(float t);
	static float EaseInOutQuad(float t);
	static float EaseInCubic(float t);
	static float EaseOutCubic(float t);
	static float EaseInOutCubic(float t);
	static float EaseInQuart(float t);
	static float EaseOutQuart(float t);
	static float EaseInOutQuart(float t);
	static float EaseInQuint(float t);
	static float EaseOutQuint(float t);
	static float EaseInOutQuint(float t);
	static float EaseInExpo(float t);
	static float EaseOutExpo(float t);
	static float EaseInOutExpo(float t);
	static float EaseInCirc(float t);
	static float EaseOutCirc(float t);
	static float EaseInOutCirc(float t);
	static float EaseInElastic(float t, float Amplitude = 1.0f, float Period = .2f);
	static float EaseOutElastic(float t, float Amplitude = 1.0f, float Period = .2f);
	static float EaseInOutElastic(float t, float Amplitude = 1.0f, float Period = .2f);
	static float EaseInBounce(float t);
	static float EaseOutBounce(float t);
	static float EaseInOutBounce(float t);
	static float EaseInBack(float t, float Overshoot = 1.70158f);
	static float EaseOutBack(float t, float Overshoot = 1.70158f);
	static float EaseInOutBack(float t, float Overshoot = 1.70158f);
};
