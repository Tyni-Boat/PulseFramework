// Copyright © by Tyni Boat. All Rights Reserved.

#include "Core/PulseTweenSubModule/PulseTweenUObject.h"

UPulseTweenUObject::UPulseTweenUObject()
{
	Tween = nullptr;
}

void UPulseTweenUObject::BeginDestroy()
{
	if (Tween != nullptr)
	{
		Tween->Destroy();
		Tween = nullptr;
	}
	UObject::BeginDestroy();
}

void UPulseTweenUObject::SetTweenInstance(PulseTweenInstance* InTween)
{
	this->Tween = InTween;
	// destroy when we are destroyed
	this->Tween->SetAutoDestroy(false);
}

void UPulseTweenUObject::Destroy()
{
	this->Tween->Destroy();
	this->Tween = nullptr;
	ConditionalBeginDestroy();
}
