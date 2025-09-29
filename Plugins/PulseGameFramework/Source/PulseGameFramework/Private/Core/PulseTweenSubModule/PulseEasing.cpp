// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseEasing.h"

const float BACK_INOUT_OVERSHOOT_MODIFIER = 1.525f;
const float BOUNCE_R = 1.0f / 2.75f;		  // reciprocal
const float BOUNCE_K1 = BOUNCE_R;			  // 36.36%
const float BOUNCE_K2 = 2 * BOUNCE_R;		  // 72.72%
const float BOUNCE_K3 = 1.5f * BOUNCE_R;	  // 54.54%
const float BOUNCE_K4 = 2.5f * BOUNCE_R;	  // 90.90%
const float BOUNCE_K5 = 2.25f * BOUNCE_R;	  // 81.81%
const float BOUNCE_K6 = 2.625f * BOUNCE_R;	  // 95.45%
const float BOUNCE_K0 = 7.5625f;

float PulseEasing::Ease(float t, EPulseEase EaseType)
{
	switch (EaseType)
	{
		default:
		case EPulseEase::Linear:
			return EaseLinear(t);
		case EPulseEase::Smoothstep:
			return EaseSmoothstep(t);
		case EPulseEase::Stepped:
			return EaseStepped(t);
		case EPulseEase::InSine:
			return EaseInSine(t);
		case EPulseEase::OutSine:
			return EaseOutSine(t);
		case EPulseEase::InOutSine:
			return EaseInOutSine(t);
		case EPulseEase::InQuad:
			return EaseInQuad(t);
		case EPulseEase::OutQuad:
			return EaseOutQuad(t);
		case EPulseEase::InOutQuad:
			return EaseInOutQuad(t);
		case EPulseEase::InCubic:
			return EaseInCubic(t);
		case EPulseEase::OutCubic:
			return EaseOutCubic(t);
		case EPulseEase::InOutCubic:
			return EaseInOutCubic(t);
		case EPulseEase::InQuart:
			return EaseInQuart(t);
		case EPulseEase::OutQuart:
			return EaseOutQuart(t);
		case EPulseEase::InOutQuart:
			return EaseInOutQuart(t);
		case EPulseEase::InQuint:
			return EaseInQuint(t);
		case EPulseEase::OutQuint:
			return EaseOutQuint(t);
		case EPulseEase::InOutQuint:
			return EaseInOutQuint(t);
		case EPulseEase::InExpo:
			return EaseInExpo(t);
		case EPulseEase::OutExpo:
			return EaseOutExpo(t);
		case EPulseEase::InOutExpo:
			return EaseInOutExpo(t);
		case EPulseEase::InCirc:
			return EaseInCirc(t);
		case EPulseEase::OutCirc:
			return EaseOutCirc(t);
		case EPulseEase::InOutCirc:
			return EaseInOutCirc(t);
		case EPulseEase::InElastic:
			return EaseInElastic(t);
		case EPulseEase::OutElastic:
			return EaseOutElastic(t);
		case EPulseEase::InOutElastic:
			return EaseInOutElastic(t);
		case EPulseEase::InBounce:
			return EaseInBounce(t);
		case EPulseEase::OutBounce:
			return EaseOutBounce(t);
		case EPulseEase::InOutBounce:
			return EaseInOutBounce(t);
		case EPulseEase::InBack:
			return EaseInBack(t);
		case EPulseEase::OutBack:
			return EaseOutBack(t);
		case EPulseEase::InOutBack:
			return EaseInOutBack(t);
	}
}

float PulseEasing::EaseWithParams(float t, EPulseEase EaseType, float Param1, float Param2)
{
	if (Param1 == 0 && Param2 == 0)
	{
		return Ease(t, EaseType);
	}

	switch (EaseType)
	{
		default:
		case EPulseEase::Linear:
			return EaseLinear(t);
		case EPulseEase::Smoothstep:
			return EaseSmoothstep(t, Param1, Param2);
		case EPulseEase::Stepped:
			return EaseStepped(t, Param1);
		case EPulseEase::InSine:
			return EaseInSine(t);
		case EPulseEase::OutSine:
			return EaseOutSine(t);
		case EPulseEase::InOutSine:
			return EaseInOutSine(t);
		case EPulseEase::InQuad:
			return EaseInQuad(t);
		case EPulseEase::OutQuad:
			return EaseOutQuad(t);
		case EPulseEase::InOutQuad:
			return EaseInOutQuad(t);
		case EPulseEase::InCubic:
			return EaseInCubic(t);
		case EPulseEase::OutCubic:
			return EaseOutCubic(t);
		case EPulseEase::InOutCubic:
			return EaseInOutCubic(t);
		case EPulseEase::InQuart:
			return EaseInQuart(t);
		case EPulseEase::OutQuart:
			return EaseOutQuart(t);
		case EPulseEase::InOutQuart:
			return EaseInOutQuart(t);
		case EPulseEase::InQuint:
			return EaseInQuint(t);
		case EPulseEase::OutQuint:
			return EaseOutQuint(t);
		case EPulseEase::InOutQuint:
			return EaseInOutQuint(t);
		case EPulseEase::InExpo:
			return EaseInExpo(t);
		case EPulseEase::OutExpo:
			return EaseOutExpo(t);
		case EPulseEase::InOutExpo:
			return EaseInOutExpo(t);
		case EPulseEase::InCirc:
			return EaseInCirc(t);
		case EPulseEase::OutCirc:
			return EaseOutCirc(t);
		case EPulseEase::InOutCirc:
			return EaseInOutCirc(t);
		case EPulseEase::InElastic:
			return EaseInElastic(t, Param1, Param2);
		case EPulseEase::OutElastic:
			return EaseOutElastic(t, Param1, Param2);
		case EPulseEase::InOutElastic:
			return EaseInOutElastic(t, Param1, Param2);
		case EPulseEase::InBounce:
			return EaseInBounce(t);
		case EPulseEase::OutBounce:
			return EaseOutBounce(t);
		case EPulseEase::InOutBounce:
			return EaseInOutBounce(t);
		case EPulseEase::InBack:
			return EaseInBack(t, Param1);
		case EPulseEase::OutBack:
			return EaseOutBack(t, Param1);
		case EPulseEase::InOutBack:
			return EaseInOutBack(t, Param1);
	}
}

float PulseEasing::EaseLinear(float t)
{
	return t;
}

float PulseEasing::EaseSmoothstep(float t, float x0, float x1)
{
	float x = FMath::Clamp<float>((t - x0) / (x1 - x0), 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
}

float PulseEasing::EaseStepped(float t, int Steps)
{
	if (t <= 0)
	{
		return 0;
	}
	else if (t >= 1)
	{
		return 1;
	}
	else
	{
		return FMath::FloorToFloat(Steps * t) / Steps;
	}
}

float PulseEasing::EaseInSine(float t)
{
	return 1 - FMath::Cos(t * PI * .5f);
}

float PulseEasing::EaseOutSine(float t)
{
	return FMath::Sin(t * PI * .5f);
}

float PulseEasing::EaseInOutSine(float t)
{
	return 0.5f * (1 - FMath::Cos(t * PI));
}

float PulseEasing::EaseInQuad(float t)
{
	return t * t;
}

float PulseEasing::EaseOutQuad(float t)
{
	return t * (2 - t);
}

float PulseEasing::EaseInOutQuad(float t)
{
	float t2 = t * 2;
	if (t2 < 1)
	{
		return t * t2;
	}
	else
	{
		float m = t - 1;
		return 1 - m * m * 2;
	}
}

float PulseEasing::EaseInCubic(float t)
{
	return t * t * t;
}

float PulseEasing::EaseOutCubic(float t)
{
	float m = t - 1;
	return 1 + m * m * m;
}

float PulseEasing::EaseInOutCubic(float t)
{
	float t2 = t * 2;
	if (t2 < 1)
	{
		return t * t2 * t2;
	}
	else
	{
		float m = t - 1;
		return 1 + m * m * m * 4;
	}
}

float PulseEasing::EaseInQuart(float t)
{
	return t * t * t * t;
}

float PulseEasing::EaseOutQuart(float t)
{
	float m = t - 1;
	return 1 - m * m * m * m;
}

float PulseEasing::EaseInOutQuart(float t)
{
	float t2 = t * 2;
	if (t2 < 1)
	{
		return t * t2 * t2 * t2;
	}
	else
	{
		float m = t - 1;
		return 1 - m * m * m * m * 8;
	}
}

float PulseEasing::EaseInQuint(float t)
{
	return t * t * t * t * t;
}

float PulseEasing::EaseOutQuint(float t)
{
	float m = t - 1;
	return 1 + m * m * m * m * m;
}

float PulseEasing::EaseInOutQuint(float t)
{
	float t2 = t * 2;
	if (t2 < 1)
	{
		return t * t2 * t2 * t2 * t2;
	}
	else
	{
		float m = t - 1;
		return 1 + m * m * m * m * m * 16;
	}
}

float PulseEasing::EaseInExpo(float t)
{
	if (t <= 0)
	{
		return 0;
	}
	if (t >= 1)
	{
		return 1;
	}
	return FMath::Pow(2, 10 * (t - 1));
}

float PulseEasing::EaseOutExpo(float t)
{
	if (t <= 0)
	{
		return 0;
	}
	if (t >= 1)
	{
		return 1;
	}
	return 1 - FMath::Pow(2, -10 * t);
}

float PulseEasing::EaseInOutExpo(float t)
{
	if (t <= 0)
	{
		return 0;
	}
	if (t >= 1)
	{
		return 1;
	}
	if (t < 0.5f)
	{
		return FMath::Pow(2, 10 * (2 * t - 1) - 1);
	}
	else
	{
		return 1 - FMath::Pow(2, -10 * (2 * t - 1) - 1);
	}
}

float PulseEasing::EaseInCirc(float t)
{
	return 1 - FMath::Sqrt(1 - t * t);
}

float PulseEasing::EaseOutCirc(float t)
{
	float m = t - 1;
	return FMath::Sqrt(1 - m * m);
}

float PulseEasing::EaseInOutCirc(float t)
{
	float t2 = t * 2;
	if (t2 < 1)
	{
		return (1 - FMath::Sqrt(1 - t2 * t2)) * .5f;
	}
	else
	{
		float m = t - 1;
		return (FMath::Sqrt(1 - 4 * m * m) + 1) * .5f;
	}
}

float PulseEasing::EaseInElastic(float t, float Amplitude, float Period)
{
	if (t == 0)
	{
		return 0;
	}
	else if (t == 1)
	{
		return 1;
	}
	else
	{
		float m = t - 1;
		float s = Period / 4.0f;
		if (Amplitude > 1)
		{
			s = Period * FMath::Asin(1.0f / Amplitude) / (2.0f * PI);
		}

		return -(Amplitude * FMath::Pow(2, 10 * m) * FMath::Sin((m - s) * (2.0f * PI) / Period));
	}
}

float PulseEasing::EaseOutElastic(float t, float Amplitude, float Period)
{
	if (t == 0)
	{
		return 0;
	}
	else if (t == 1)
	{
		return 1;
	}
	else
	{
		float s = Period / 4.0f;
		if (Amplitude > 1)
		{
			s = Period * FMath::Asin(1.0f / Amplitude) / (2.0f * PI);
		}
		return 1.0f + Amplitude * FMath::Pow(2, -10 * t) * FMath::Sin((t - s) * (2.0f * PI) / Period);
	}
}

float PulseEasing::EaseInOutElastic(float t, float Amplitude, float Period)
{
	if (t == 0)
	{
		return 0;
	}
	else if (t == 1)
	{
		return 1;
	}
	else
	{
		float m = 2.0f * t - 1;
		float s = Period / 4.0f;
		if (Amplitude > 1)
		{
			s = Period * FMath::Asin(1.0f / Amplitude) / (2.0f * PI);
		}

		if (m < 0)
		{
			return .5f * -(Amplitude * FMath::Pow(2, 10 * m) * FMath::Sin((m - s) * (2.0f * PI) / Period));
		}
		else
		{
			return 1.0f + .5f * (Amplitude * FMath::Pow(2, -10 * t) * FMath::Sin((t - s) * (2.0f * PI) / Period));
		}
	}
}

float PulseEasing::EaseInBounce(float t)
{
	return 1 - EaseOutBounce(1 - t);
}

float PulseEasing::EaseOutBounce(float t)
{
	float t2;

	if (t < BOUNCE_K1)
	{
		return BOUNCE_K0 * t * t;
	}
	else if (t < BOUNCE_K2)
	{
		t2 = t - BOUNCE_K3;
		return BOUNCE_K0 * t2 * t2 + 0.75f;
	}
	else if (t < BOUNCE_K4)
	{
		t2 = t - BOUNCE_K5;
		return BOUNCE_K0 * t2 * t2 + 0.9375f;
	}
	else
	{
		t2 = t - BOUNCE_K6;
		return BOUNCE_K0 * t2 * t2 + 0.984375f;
	}
}

float PulseEasing::EaseInOutBounce(float t)
{
	float t2 = t * 2;
	if (t2 < 1)
	{
		return .5f - .5f * EaseOutBounce(1 - t2);
	}
	else
	{
		return .5f + .5f * EaseOutBounce(t2 - 1);
	}
}

float PulseEasing::EaseInBack(float t, float Overshoot)
{
	return t * t * ((Overshoot + 1) * t - Overshoot);
}

float PulseEasing::EaseOutBack(float t, float Overshoot)
{
	float m = t - 1;
	return 1 + m * m * (m * (Overshoot + 1) + Overshoot);
}

float PulseEasing::EaseInOutBack(float t, float Overshoot)
{
	float t2 = t * 2;
	float s = Overshoot * BACK_INOUT_OVERSHOOT_MODIFIER;
	if (t < .5f)
	{
		return t * t2 * (t2 * (s + 1) - s);
	}
	else
	{
		float m = t - 1;
		return 1 + 2 * m * m * (2 * m * (s + 1) + s);
	}
}