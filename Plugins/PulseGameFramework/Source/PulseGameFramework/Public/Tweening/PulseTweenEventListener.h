// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseTweenTypes.h"
#include "Components/ActorComponent.h"
#include "PulseTweenEventListener.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PULSEGAMEFRAMEWORK_API UPulseTweenEventListener : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPulseTweenEventListener();
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

protected:
	UFUNCTION()
	void OnTweenUpdateEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenStartedEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenPausedEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenResumedEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenCompletedEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenCancelledEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenPingPongApexEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenLoopEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenSequenceMoveNextEvent_Func(TSet<FGuid> TweenUIDSet);
	UFUNCTION()
	void OnTweenSequenceCompletedEvent_Func(TSet<FGuid> TweenUIDSet);

public:
	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenUpdateEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenStartedEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenPausedEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenResumedEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenCompletedEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenCancelledEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenPingPongApexEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenLoopEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenSequenceMoveNextEvent;

	UPROPERTY(BlueprintAssignable, Category="PulseCore|Tweening")
	FTweenIDTriggerEvent OnTweenSequenceCompletedEvent;
};
