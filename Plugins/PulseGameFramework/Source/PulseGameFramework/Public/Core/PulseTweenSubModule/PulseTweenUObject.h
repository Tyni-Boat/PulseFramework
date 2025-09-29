// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once
#include "PulseTweenInstance.h"
#include "PulseTweenUObject.generated.h"

/**
 * @brief Use this to wrap an PulseTweenInstance inside a UObject, so that it's destroyed when its outer object is destroyed
 */
UCLASS()
class UPulseTweenUObject : public UObject
{
	GENERATED_BODY()

public:
	PulseTweenInstance* Tween;

	UPulseTweenUObject();
	virtual void BeginDestroy() override;

	void SetTweenInstance(PulseTweenInstance* InTween);
	/**
	 * @brief Stop the tween immediately and mark this object for destruction
	 */
	void Destroy();
};
