// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenSystemCore.h"
#include "Core/PulseTweenSubModule/PulseTweenManager.h"


DEFINE_LOG_CATEGORY(LogPulseTween)

const int DEFAULT_FLOAT_TWEEN_CAPACITY = 50;
const int DEFAULT_VECTOR_TWEEN_CAPACITY = 50;
const int DEFAULT_VECTOR2D_TWEEN_CAPACITY = 50;
const int DEFAULT_QUAT_TWEEN_CAPACITY = 10;

PulseTweenManager<PulseTweenInstanceFloat>* PulseTweenSystemCore::FloatTweenManager = nullptr;
PulseTweenManager<PulseTweenInstanceVector>* PulseTweenSystemCore::VectorTweenManager = nullptr;
PulseTweenManager<PulseTweenInstanceVector2D>* PulseTweenSystemCore::Vector2DTweenManager = nullptr;
PulseTweenManager<PulseTweenInstanceQuat>* PulseTweenSystemCore::QuatTweenManager = nullptr;

int PulseTweenSystemCore::NumReservedFloat = DEFAULT_FLOAT_TWEEN_CAPACITY;
int PulseTweenSystemCore::NumReservedVector = DEFAULT_VECTOR_TWEEN_CAPACITY;
int PulseTweenSystemCore::NumReservedVector2D = DEFAULT_VECTOR2D_TWEEN_CAPACITY;
int PulseTweenSystemCore::NumReservedQuat = DEFAULT_QUAT_TWEEN_CAPACITY;

void PulseTweenSystemCore::Initialize()
{
	FloatTweenManager = new PulseTweenManager<PulseTweenInstanceFloat>(DEFAULT_FLOAT_TWEEN_CAPACITY);
	VectorTweenManager = new PulseTweenManager<PulseTweenInstanceVector>(DEFAULT_VECTOR_TWEEN_CAPACITY);
	Vector2DTweenManager = new PulseTweenManager<PulseTweenInstanceVector2D>(DEFAULT_VECTOR2D_TWEEN_CAPACITY);
	QuatTweenManager = new PulseTweenManager<PulseTweenInstanceQuat>(DEFAULT_QUAT_TWEEN_CAPACITY);
	
	NumReservedFloat = DEFAULT_FLOAT_TWEEN_CAPACITY;
	NumReservedVector = DEFAULT_VECTOR_TWEEN_CAPACITY;
	NumReservedVector2D = DEFAULT_VECTOR2D_TWEEN_CAPACITY;
	NumReservedQuat = DEFAULT_QUAT_TWEEN_CAPACITY;
}

void PulseTweenSystemCore::Deinitialize()
{
	delete FloatTweenManager;
	delete VectorTweenManager;
	delete Vector2DTweenManager;
	delete QuatTweenManager;
}

void PulseTweenSystemCore::EnsureCapacity(int NumFloatTweens, int NumVectorTweens, int NumVector2DTweens, int NumQuatTweens)
{
	FloatTweenManager->EnsureCapacity(NumFloatTweens);
	VectorTweenManager->EnsureCapacity(NumVectorTweens);
	Vector2DTweenManager->EnsureCapacity(NumVector2DTweens);
	QuatTweenManager->EnsureCapacity(NumQuatTweens);
	
	NumReservedFloat = FloatTweenManager->GetCurrentCapacity();
	NumReservedVector = VectorTweenManager->GetCurrentCapacity();
	NumReservedVector2D = Vector2DTweenManager->GetCurrentCapacity();
	NumReservedQuat = QuatTweenManager->GetCurrentCapacity();
}

void PulseTweenSystemCore::EnsureCapacity(int NumTweens)
{
	EnsureCapacity(NumTweens, NumTweens, NumTweens, NumTweens);
}

void PulseTweenSystemCore::Update(float UnscaledDeltaSeconds, float DilatedDeltaSeconds, bool bIsGamePaused)
{
	FloatTweenManager->Update(UnscaledDeltaSeconds, DilatedDeltaSeconds, bIsGamePaused);
	VectorTweenManager->Update(UnscaledDeltaSeconds, DilatedDeltaSeconds, bIsGamePaused);
	Vector2DTweenManager->Update(UnscaledDeltaSeconds, DilatedDeltaSeconds, bIsGamePaused);
	QuatTweenManager->Update(UnscaledDeltaSeconds, DilatedDeltaSeconds, bIsGamePaused);
}

void PulseTweenSystemCore::ClearActiveTweens()
{
	FloatTweenManager->ClearActiveTweens();
	VectorTweenManager->ClearActiveTweens();
	Vector2DTweenManager->ClearActiveTweens();
	QuatTweenManager->ClearActiveTweens();
}

int PulseTweenSystemCore::CheckTweenCapacity()
{
	if(FloatTweenManager->GetCurrentCapacity() > NumReservedFloat)
	{
		UE_LOG(LogPulseTween, Warning, TEXT("Consider increasing initial capacity for Float tweens with PulseTweenSystemCore::EnsureCapacity(). %d were initially reserved, but now there are %d in memory."),
			NumReservedFloat, FloatTweenManager->GetCurrentCapacity());
	}
	if(VectorTweenManager->GetCurrentCapacity() > NumReservedVector)
	{
		UE_LOG(LogPulseTween, Warning, TEXT("Consider increasing initial capacity for Vector (3d vector) tweens with PulseTweenSystemCore::EnsureCapacity(). %d were initially reserved, but now there are %d in memory."),
			NumReservedVector, VectorTweenManager->GetCurrentCapacity());
	}
	if(Vector2DTweenManager->GetCurrentCapacity() > NumReservedVector2D)
	{
		UE_LOG(LogPulseTween, Warning, TEXT("Consider increasing initial capacity for Vector2D tweens with PulseTweenSystemCore::EnsureCapacity(). %d were initially reserved, but now there are %d in memory."),
			NumReservedVector2D, Vector2DTweenManager->GetCurrentCapacity());
	}
	if(QuatTweenManager->GetCurrentCapacity() > NumReservedQuat)
	{
		UE_LOG(LogPulseTween, Warning, TEXT("Consider increasing initial capacity for Quaternion tweens with PulseTweenSystemCore::EnsureCapacity(). %d were initially reserved, but now there are %d in memory."),
			NumReservedQuat, QuatTweenManager->GetCurrentCapacity());
	}

	return FloatTweenManager->GetCurrentCapacity() + VectorTweenManager->GetCurrentCapacity() +  Vector2DTweenManager->GetCurrentCapacity() + QuatTweenManager->GetCurrentCapacity();
}

float PulseTweenSystemCore::Ease(float t, EPulseEase EaseType)
{
	return PulseEasing::Ease(t, EaseType);
}

PulseTweenInstanceFloat* PulseTweenSystemCore::Play(float Start, float End, TFunction<void(float)> OnUpdate, float DurationSecs, EPulseEase EaseType)
{
	PulseTweenInstanceFloat* NewTween = FloatTweenManager->CreateTween();
	NewTween->Initialize(Start, End, MoveTemp(OnUpdate), DurationSecs, EaseType);
	return NewTween;
}

PulseTweenInstanceVector* PulseTweenSystemCore::Play(
	FVector Start, FVector End, TFunction<void(FVector)> OnUpdate, float DurationSecs, EPulseEase EaseType)
{
	PulseTweenInstanceVector* NewTween = VectorTweenManager->CreateTween();
	NewTween->Initialize(Start, End, MoveTemp(OnUpdate), DurationSecs, EaseType);
	return NewTween;
}

PulseTweenInstanceVector2D* PulseTweenSystemCore::Play(
	FVector2D Start, FVector2D End, TFunction<void(FVector2D)> OnUpdate, float DurationSecs, EPulseEase EaseType)
{
	PulseTweenInstanceVector2D* NewTween = Vector2DTweenManager->CreateTween();
	NewTween->Initialize(Start, End, MoveTemp(OnUpdate), DurationSecs, EaseType);
	return NewTween;
}

PulseTweenInstanceQuat* PulseTweenSystemCore::Play(FQuat Start, FQuat End, TFunction<void(FQuat)> OnUpdate, float DurationSecs, EPulseEase EaseType)
{
	PulseTweenInstanceQuat* NewTween = QuatTweenManager->CreateTween();
	NewTween->Initialize(Start, End, MoveTemp(OnUpdate), DurationSecs, EaseType);
	return NewTween;
}
