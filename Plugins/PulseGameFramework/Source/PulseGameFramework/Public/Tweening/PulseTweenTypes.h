// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "PulseTweenTypes.generated.h"


const float BACK_INOUT_OVERSHOOT_MODIFIER = 1.525f;
const float BOUNCE_R = 1.0f / 2.75f; // reciprocal
const float BOUNCE_K1 = BOUNCE_R; // 36.36%
const float BOUNCE_K2 = 2 * BOUNCE_R; // 72.72%
const float BOUNCE_K3 = 1.5f * BOUNCE_R; // 54.54%
const float BOUNCE_K4 = 2.5f * BOUNCE_R; // 90.90%
const float BOUNCE_K5 = 2.25f * BOUNCE_R; // 81.81%
const float BOUNCE_K6 = 2.625f * BOUNCE_R; // 95.45%
const float BOUNCE_K0 = 7.5625f;

// Type of tween eases
UENUM(BlueprintType)
enum class EPulseTweenEase : uint8
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
	InOutBack
};

// Status of a tween instance
UENUM(BlueprintType)
enum class EPulseTweenStatus: uint8
{
	WaitingToStart,
	JustStarted,
	Updating,
	JustPaused,
	Paused,
	JustResumed,
	JustReachedPingPongApex,
	JustLooped,
	JustCompleted,
	Completed,
};

USTRUCT(BlueprintType)
struct FTweenParams
{
	GENERATED_BODY()

public:
	// The time to tween from 0 to 1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Forward", meta=(ClampMin = "0.1", UIMin = "0.1"))
	float ForwardDuration = 0.1;

	// The time to tween in reverse from 1 to 0. Set to 0 to disable reverse tweening
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Reverse", meta=(ClampMin = "0", UIMin = "0"))
	float ReverseDuration = 0;

	// Delay before beginning to tween
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Forward", meta=(ClampMin = "0", UIMin = "0"))
	float StartDelay = 0;

	// Delay before reversing the tween
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Reverse", meta=(ClampMin = "0", UIMin = "0"))
	float ReverseDelay = 0;

	// Delay before looping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Loop", meta=(ClampMin = "0", UIMin = "0"))
	float LoopDelay = 0;

	// Number of loops to do. Set to < 0 for infinite looping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Loop")
	int32 Loops = 0;

	// Allow tween when the game is paused
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|System")
	bool TweenWhenPaused = false;

	// Tween speed affected by the global time dilation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|System")
	bool UseTimeDilation = false;

	// Forward tween easing type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Forward")
	EPulseTweenEase ForwardEasing = EPulseTweenEase::Linear;

	// Reverse tween easing type
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|Tweening|Reverse")
	EPulseTweenEase ReverseEasing = EPulseTweenEase::Linear;

	friend bool operator==(const FTweenParams& Lhs, const FTweenParams& RHS)
	{
		return Lhs.ForwardDuration == RHS.ForwardDuration
			&& Lhs.ReverseDuration == RHS.ReverseDuration
			&& Lhs.StartDelay == RHS.StartDelay
			&& Lhs.ReverseDelay == RHS.ReverseDelay
			&& Lhs.LoopDelay == RHS.LoopDelay
			&& Lhs.Loops == RHS.Loops
			&& Lhs.TweenWhenPaused == RHS.TweenWhenPaused
			&& Lhs.UseTimeDilation == RHS.UseTimeDilation
			&& Lhs.ForwardEasing == RHS.ForwardEasing
			&& Lhs.ReverseEasing == RHS.ReverseEasing;
	}

	friend bool operator!=(const FTweenParams& Lhs, const FTweenParams& RHS)
	{
		return !(Lhs == RHS);
	}
};

// Responsible for the linear transform math
USTRUCT()
struct FPulseEaseEvaluator
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float Amplitude = 1.0f;
	UPROPERTY()
	float Period = 0.2f;
	UPROPERTY()
	float Overshoot = 1.70158f;
	UPROPERTY()
	float SmoothStepX0 = 0;
	UPROPERTY()
	float SmoothStepX1 = 1;
	UPROPERTY()
	int Steps = 10;

	inline float EaseLinear(float t) const { return t; }

	inline float EaseSmoothstep(float t) const
	{
		float x = FMath::Clamp<float>((t - SmoothStepX0) / (SmoothStepX1 - SmoothStepX0), 0.0f, 1.0f);
		return x * x * (3.0f - 2.0f * x);
	}

	inline float EaseStepped(float t) const
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

	inline float EaseInSine(float t) const { return 1 - FMath::Cos(t * PI * .5f); }
	inline float EaseOutSine(float t) const { return FMath::Sin(t * PI * .5f); }
	inline float EaseInOutSine(float t) const { return 0.5f * (1 - FMath::Cos(t * PI)); }
	inline float EaseInQuad(float t) const { return t * t; }
	inline float EaseOutQuad(float t) const { return t * (2 - t); }

	inline float EaseInOutQuad(float t) const
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

	inline float EaseInCubic(float t) const { return t * t * t; }

	inline float EaseOutCubic(float t) const
	{
		float m = t - 1;
		return 1 + m * m * m;
	}

	inline float EaseInOutCubic(float t) const
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

	inline float EaseInQuart(float t) const { return t * t * t * t; }

	inline float EaseOutQuart(float t) const
	{
		float m = t - 1;
		return 1 - m * m * m * m;
	}

	inline float EaseInOutQuart(float t) const
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

	inline float EaseInQuint(float t) const { return t * t * t * t * t; }

	inline float EaseOutQuint(float t) const
	{
		float m = t - 1;
		return 1 + m * m * m * m * m;
	}

	inline float EaseInOutQuint(float t) const
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

	inline float EaseInExpo(float t) const
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

	inline float EaseOutExpo(float t) const
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

	inline float EaseInOutExpo(float t) const
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

	inline float EaseInCirc(float t) const { return 1 - FMath::Sqrt(1 - t * t); }

	inline float EaseOutCirc(float t) const
	{
		float m = t - 1;
		return FMath::Sqrt(1 - m * m);
	}

	inline float EaseInOutCirc(float t) const
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

	inline float EaseInElastic(float t) const
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

	inline float EaseOutElastic(float t) const
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

	inline float EaseInOutElastic(float t) const
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

	inline float EaseInBounce(float t) const { return 1 - EaseOutBounce(1 - t); }

	inline float EaseOutBounce(float t) const
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

	inline float EaseInOutBounce(float t) const
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

	inline float EaseInBack(float t) const { return t * t * ((Overshoot + 1) * t - Overshoot); }

	inline float EaseOutBack(float t) const
	{
		float m = t - 1;
		return 1 + m * m * (m * (Overshoot + 1) + Overshoot);
	}

	inline float EaseInOutBack(float t) const
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
};

// Tweening object. Use GetValue() to get the actual eased value [0-1]
USTRUCT()
struct FPulseTweenInstance
{
	GENERATED_BODY()

private:
	FPulseEaseEvaluator easeEvaluator;

	TWeakObjectPtr<const UObject> _Owner = nullptr;

	FString _ownerName;

	inline EPulseTweenEase GetReverseEase(EPulseTweenEase InEase) const
	{
		switch (InEase)
		{
		case EPulseTweenEase::InSine:
			return EPulseTweenEase::OutSine;
		case EPulseTweenEase::OutSine:
			return EPulseTweenEase::InSine;
		case EPulseTweenEase::InQuad:
			return EPulseTweenEase::OutQuad;
		case EPulseTweenEase::OutQuad:
			return EPulseTweenEase::InQuad;
		case EPulseTweenEase::InCubic:
			return EPulseTweenEase::OutCubic;
		case EPulseTweenEase::OutCubic:
			return EPulseTweenEase::InCubic;
		case EPulseTweenEase::InQuart:
			return EPulseTweenEase::OutQuart;
		case EPulseTweenEase::OutQuart:
			return EPulseTweenEase::InQuart;
		case EPulseTweenEase::InQuint:
			return EPulseTweenEase::OutQuint;
		case EPulseTweenEase::OutQuint:
			return EPulseTweenEase::InQuint;
		case EPulseTweenEase::InExpo:
			return EPulseTweenEase::OutExpo;
		case EPulseTweenEase::OutExpo:
			return EPulseTweenEase::InExpo;
		case EPulseTweenEase::InCirc:
			return EPulseTweenEase::OutCirc;
		case EPulseTweenEase::OutCirc:
			return EPulseTweenEase::InCirc;
		case EPulseTweenEase::InElastic:
			return EPulseTweenEase::OutElastic;
		case EPulseTweenEase::OutElastic:
			return EPulseTweenEase::InElastic;
		case EPulseTweenEase::InBounce:
			return EPulseTweenEase::OutBounce;
		case EPulseTweenEase::OutBounce:
			return EPulseTweenEase::InBounce;
		case EPulseTweenEase::InBack:
			return EPulseTweenEase::OutBack;
		case EPulseTweenEase::OutBack:
			return EPulseTweenEase::InBack;
		default:
			return InEase;
		}
	}

public:
	inline FPulseTweenInstance()
	{
	}
	
	inline FPulseTweenInstance(const UObject* Owner, const FTweenParams& Params)
	{
		_Owner = MakeWeakObjectPtr(Owner);
		_ownerName = Owner != nullptr? Owner->GetName() : "";
		ForwardDuration = Params.ForwardDuration;
		ReverseDuration = Params.ReverseDuration;
		PingPong = ReverseDuration > 0;
		StartDelay = Params.StartDelay;
		StartDelayRemaining = Params.StartDelay;
		ReverseDelayDuration = Params.ReverseDelay;
		LoopDelayDuration = Params.LoopDelay;
		InfiniteLoop = Params.Loops < 0;
		TotalLoops = FMath::Max(Params.Loops, 0);
		LoopRemaining = TotalLoops;
		UpdateWhenPaused = Params.TweenWhenPaused;
		UseTimeDilation = Params.UseTimeDilation;
		Easing = Params.ForwardEasing;
		ReverseUpdateEasing = Params.ReverseEasing;
		LoopedOnce = false;
	}

	inline FPulseTweenInstance(const FTweenParams& Params)
	{
		ForwardDuration = Params.ForwardDuration;
		ReverseDuration = Params.ReverseDuration;
		PingPong = ReverseDuration > 0;
		StartDelay = Params.StartDelay;
		StartDelayRemaining = Params.StartDelay;
		ReverseDelayDuration = Params.ReverseDelay;
		LoopDelayDuration = Params.LoopDelay;
		InfiniteLoop = Params.Loops < 0;
		TotalLoops = FMath::Max(Params.Loops, 0);
		LoopRemaining = TotalLoops;
		UpdateWhenPaused = Params.TweenWhenPaused;
		UseTimeDilation = Params.UseTimeDilation;
		Easing = Params.ForwardEasing;
		ReverseUpdateEasing = Params.ReverseEasing;
		LoopedOnce = false;
	}

	inline FString ToString() const
	{
		const FGuid guid = Identifier;
		FString status = FString::Printf(TEXT("%s"), *UEnum::GetValueAsString(Status));
		FString enumName;
		status.Split("::", &enumName, &status);
		return FString::Printf(TEXT("ID %s: Status: %s, Time: %lf/%lf ,Loops: %d%s"), *guid.ToString(), *status,
										  Time, GetDuration(), LoopRemaining,
										  *FString(_Owner.IsExplicitlyNull()? "" : (_Owner.IsStale(true, true)? " ,Owner is no more" : FString(" , Owner: ").Append(_ownerName))));
	}

	UPROPERTY()
	float ForwardDuration = 0;
	UPROPERTY()
	float ReverseDuration = 0;
	UPROPERTY()
	float StartDelay = 0;
	UPROPERTY()
	float StartDelayRemaining = 0;
	UPROPERTY()
	float ReverseDelayDuration = 0;
	UPROPERTY()
	float LoopDelayDuration = 0;
	UPROPERTY()
	int32 TotalLoops = 0;
	UPROPERTY()
	bool InfiniteLoop = false;
	UPROPERTY()
	bool PingPong = false;
	UPROPERTY()
	bool UpdateWhenPaused = false;
	UPROPERTY()
	bool UseTimeDilation = false;
	UPROPERTY()
	EPulseTweenEase Easing = EPulseTweenEase::Linear;
	UPROPERTY()
	EPulseTweenEase ReverseUpdateEasing = EPulseTweenEase::Linear;

	UPROPERTY()
	FGuid Identifier = {};
	UPROPERTY()
	float Time = 0;
	UPROPERTY()
	float TotalTime = 0;
	UPROPERTY()
	int32 LoopRemaining = 0;
	UPROPERTY()
	float SwingTimer = 0;
	UPROPERTY()
	bool LoopedOnce = false;
	UPROPERTY()
	EPulseTweenStatus Status = EPulseTweenStatus::WaitingToStart;
	UPROPERTY()
	bool UpdateInReverse = false;
	UPROPERTY()
	bool TweenIsPaused = false;
	UPROPERTY()
	float OnCompletedDeltaDiff = 0;

	inline bool IsComplete() const { return LoopRemaining < 0; }

	inline void Reset()
	{
		Time = 0;
		SwingTimer = 0;
		LoopRemaining = TotalLoops;
		Status = EPulseTweenStatus::WaitingToStart;
		UpdateInReverse = false;
		OnCompletedDeltaDiff = 0;
		LoopedOnce = false;
	}

	inline float GetDuration() const { return UpdateInReverse ? ReverseDuration : ForwardDuration; }

	inline float GetTotalDuration() const
	{
		return InfiniteLoop ? 0 :
			((ReverseDuration + ForwardDuration) * FMath::Max(TotalLoops + 1, 0)
				+ (PingPong? ReverseDelayDuration * (TotalLoops + 1) : 0)
				+ LoopDelayDuration * (TotalLoops));
	}

	// Consume the difference Abs(Time - Duration) calculated when the tween completed. Useful for sequences to "inject" to next tween instance's delta time.
	inline bool ConsumeCompletedDeltaDiff(float& OutDeltaDiff)
	{
		if (!IsComplete())
			return false;
		OutDeltaDiff = OnCompletedDeltaDiff;
		OnCompletedDeltaDiff = 0;
		return true;
	}

	inline void Update(float DeltaTime, float TimeDilation = 1, bool bIsPaused = false)
	{
		if (!_Owner.IsExplicitlyNull() && _Owner.IsStale(true, true))
		{
			LoopRemaining = -1;
			if (Status != EPulseTweenStatus::Completed)
			{
				OnCompletedDeltaDiff = 0;
				Status = EPulseTweenStatus::Completed;
			}
			return;
		}
		if (TweenIsPaused)
		{
			if (Status != EPulseTweenStatus::Paused)
			{
				if (Status == EPulseTweenStatus::JustPaused)
					Status = EPulseTweenStatus::Paused;
				else
					Status = EPulseTweenStatus::JustPaused;
			}
			return;
		}
		if (!UpdateWhenPaused && bIsPaused)
		{
			if (Status != EPulseTweenStatus::Paused)
			{
				if (Status == EPulseTweenStatus::JustPaused)
					Status = EPulseTweenStatus::Paused;
				else
					Status = EPulseTweenStatus::JustPaused;
			}
			return;
		}
		float d = DeltaTime * (UseTimeDilation ? TimeDilation : 1);
		float duration = GetDuration();
		float activeDelay = UpdateInReverse ? ReverseDelayDuration : (LoopedOnce? LoopDelayDuration : 0);
		if (StartDelayRemaining > 0)
		{
			if (Status != EPulseTweenStatus::WaitingToStart)
				Status = EPulseTweenStatus::WaitingToStart;
			StartDelayRemaining -= d;
			return;
		}
		TotalTime += d;
		if (Time >= 0 && Time <= duration && LoopRemaining >= 0)
		{
			if (Status != EPulseTweenStatus::Updating)
			{
				if (Status == EPulseTweenStatus::WaitingToStart)
					Status = EPulseTweenStatus::JustStarted;
				else if (Status == EPulseTweenStatus::JustPaused || Status == EPulseTweenStatus::Paused)
					Status = EPulseTweenStatus::JustResumed;
				else
					Status = EPulseTweenStatus::Updating;
			}
			if (activeDelay > 0 && SwingTimer < activeDelay)
			{
				SwingTimer += d;
				if (SwingTimer >= activeDelay)
				{
					const float remainingD = FMath::Abs(SwingTimer - activeDelay);
					if (remainingD <= 0)
						return;
					d = remainingD;
				}
				else
				{
					return;
				}
			}
			Time += d * (UpdateInReverse ? -1 : 1);
		}
		else if (LoopRemaining >= 0)
		{
			SwingTimer = 0;
			if (PingPong)
			{
				if (!UpdateInReverse)
				{
					UpdateInReverse = true;
					Time = ReverseDuration - (ReverseDelayDuration > 0 ? 0 : FMath::Abs(Time - ForwardDuration) + d);
					Status = EPulseTweenStatus::JustReachedPingPongApex;
					return;
				}
				UpdateInReverse = false;
			}
			LoopedOnce = true;
			if (!InfiniteLoop)
			{
				LoopRemaining--;
			}
			Time = (ReverseDelayDuration || LoopDelayDuration > 0 ? 0 : (Time < 0 ? FMath::Abs(Time) : 0));
			if (LoopRemaining >= 0)
				Status = EPulseTweenStatus::JustLooped;
			else
				Status = EPulseTweenStatus::JustCompleted;
		}
		else
		{
			if (Status != EPulseTweenStatus::Completed)
			{
				OnCompletedDeltaDiff = FMath::Abs(Time - (PingPong ? 0 : duration));
				Status = EPulseTweenStatus::Completed;
			}
		}
	}

	inline float GetValue() const
	{
		const float duration = GetDuration();
		const float Value = FMath::Clamp(duration > 0 ? Time / duration : 0, 0, 1);
		const EPulseTweenEase InEaseType = (PingPong && UpdateInReverse) ? GetReverseEase(ReverseUpdateEasing) : Easing;
		float inEase = 0;
		switch (InEaseType)
		{
		default:
			inEase = Value;
			break;
		case EPulseTweenEase::Linear:
			inEase = easeEvaluator.EaseLinear(Value);
			break;
		case EPulseTweenEase::Smoothstep:
			inEase = easeEvaluator.EaseSmoothstep(Value);
			break;
		case EPulseTweenEase::Stepped:
			inEase = easeEvaluator.EaseStepped(Value);
			break;
		case EPulseTweenEase::InSine:
			inEase = easeEvaluator.EaseInSine(Value);
			break;
		case EPulseTweenEase::OutSine:
			inEase = easeEvaluator.EaseOutSine(Value);
			break;
		case EPulseTweenEase::InOutSine:
			inEase = easeEvaluator.EaseInOutSine(Value);
			break;
		case EPulseTweenEase::InQuad:
			inEase = easeEvaluator.EaseInQuad(Value);
			break;
		case EPulseTweenEase::OutQuad:
			inEase = easeEvaluator.EaseOutQuad(Value);
			break;
		case EPulseTweenEase::InOutQuad:
			inEase = easeEvaluator.EaseInOutQuad(Value);
			break;
		case EPulseTweenEase::InCubic:
			inEase = easeEvaluator.EaseInCubic(Value);
			break;
		case EPulseTweenEase::OutCubic:
			inEase = easeEvaluator.EaseOutCubic(Value);
			break;
		case EPulseTweenEase::InOutCubic:
			inEase = easeEvaluator.EaseInOutCubic(Value);
			break;
		case EPulseTweenEase::InQuart:
			inEase = easeEvaluator.EaseInQuart(Value);
			break;
		case EPulseTweenEase::OutQuart:
			inEase = easeEvaluator.EaseOutQuart(Value);
			break;
		case EPulseTweenEase::InOutQuart:
			inEase = easeEvaluator.EaseInOutQuart(Value);
			break;
		case EPulseTweenEase::InQuint:
			inEase = easeEvaluator.EaseInQuint(Value);
			break;
		case EPulseTweenEase::OutQuint:
			inEase = easeEvaluator.EaseOutQuint(Value);
			break;
		case EPulseTweenEase::InOutQuint:
			inEase = easeEvaluator.EaseInOutQuint(Value);
			break;
		case EPulseTweenEase::InExpo:
			inEase = easeEvaluator.EaseInExpo(Value);
			break;
		case EPulseTweenEase::OutExpo:
			inEase = easeEvaluator.EaseOutExpo(Value);
			break;
		case EPulseTweenEase::InOutExpo:
			inEase = easeEvaluator.EaseInOutExpo(Value);
			break;
		case EPulseTweenEase::InCirc:
			inEase = easeEvaluator.EaseInCirc(Value);
			break;
		case EPulseTweenEase::OutCirc:
			inEase = easeEvaluator.EaseOutCirc(Value);
			break;
		case EPulseTweenEase::InOutCirc:
			inEase = easeEvaluator.EaseInOutCirc(Value);
			break;
		case EPulseTweenEase::InElastic:
			inEase = easeEvaluator.EaseInElastic(Value);
			break;
		case EPulseTweenEase::OutElastic:
			inEase = easeEvaluator.EaseOutElastic(Value);
			break;
		case EPulseTweenEase::InOutElastic:
			inEase = easeEvaluator.EaseInOutElastic(Value);
			break;
		case EPulseTweenEase::InBounce:
			inEase = easeEvaluator.EaseInBounce(Value);
			break;
		case EPulseTweenEase::OutBounce:
			inEase = easeEvaluator.EaseOutBounce(Value);
			break;
		case EPulseTweenEase::InOutBounce:
			inEase = easeEvaluator.EaseInOutBounce(Value);
			break;
		case EPulseTweenEase::InBack:
			inEase = easeEvaluator.EaseInBack(Value);
			break;
		case EPulseTweenEase::OutBack:
			inEase = easeEvaluator.EaseOutBack(Value);
			break;
		case EPulseTweenEase::InOutBack:
			inEase = easeEvaluator.EaseInOutBack(Value);
			break;
		}
		return inEase;
	}
};


// Tweening in sequence
USTRUCT()
struct FPulseTweenSequence
{
	GENERATED_BODY()

protected:
	bool InfiniteLoop = false;

public:
	int32 CurrentIndex = 0;
	TArray<FPulseTweenInstance> TweenSequenceInstances;
	FGuid TweenSequenceID;
	int32 LoopCount = 0;
	bool wasRestarted = false;

	void Reset()
	{
		CurrentIndex = 0;
		TweenSequenceInstances.Empty();
		InfiniteLoop = LoopCount < 0;
	}

	bool IsCompleted() const { return !TweenSequenceInstances.IsEmpty() && !TweenSequenceInstances.IsValidIndex(CurrentIndex) && TweenSequenceID.IsValid(); }

	int32 ContainsTweenAtIndex(const FGuid& TweenID) const
	{
		const int32 tIndex = TweenSequenceInstances.IndexOfByPredicate([TweenID](const FPulseTweenInstance& tween)-> bool { return tween.Identifier == TweenID; });
		return tIndex;
	}

	// try to go to the next index, try looping and return true if the sequence index is still valid.
	bool SequenceNext(const TSet<FGuid>& CompletedInstanceSet)
	{
		FPulseTweenInstance instance;
		if (!GetCurrentInstance(instance))
			return false;
		if (CompletedInstanceSet.IsEmpty())
			return false;
		if (!CompletedInstanceSet.Contains(instance.Identifier))
			return false;
		TweenSequenceInstances[CurrentIndex].Reset();
		if (!wasRestarted)
			CurrentIndex++;
		else
			wasRestarted = false;
		if (CurrentIndex >= TweenSequenceInstances.Num())
		{
			if (!InfiniteLoop)
			{
				LoopCount--;
				if (LoopCount < 0)
					return false;
			}
		}
		CurrentIndex = FMath::Modulo(CurrentIndex, TweenSequenceInstances.Num());
		return TweenSequenceInstances.IsValidIndex(CurrentIndex);
	}

	bool GetCurrentInstance(FPulseTweenInstance& OutInstance)
	{
		if (!TweenSequenceInstances.IsValidIndex(CurrentIndex))
			return false;
		OutInstance = TweenSequenceInstances[CurrentIndex];
		return true;
	}

	int32 InstanceIndex(const FGuid& TweenID) const
	{
		const int32 idx = TweenSequenceInstances.IndexOfByPredicate([TweenID](const FPulseTweenInstance& tween)-> bool { return tween.Identifier == TweenID; });
		return idx;
	}
};



DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTweenUpdateEvent, float, Value, float, Percentage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTweenIDTriggerEvent, TSet<FGuid>, TweenUIDSet);
