// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseTweenTypes.h"
#include "Containers/SpscQueue.h"
#include "Core/PulseCoreTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "PulseTween.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTweenUpdateEvent, float, Value, float, Percentage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTweenIDTriggerEvent, TSet<FGuid>, TweenUIDSet);


/**
 * Pulse Tween tickable world sub-System
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseTween : public UTickableWorldSubsystem, public IIPulseCore
{
	GENERATED_BODY()

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


	virtual void BeginDestroy() override;
	virtual void Deinitialize() override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual void Tick(float DeltaTime) override;

	bool GetTweenInstance(const FGuid& Guid, FPulseTweenInstance& OutInstance);

	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	bool PauseTweenInstance(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	bool ResumeTweenInstance(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	bool ResetTweenInstance(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	bool ResetSequence(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	bool CancelTweenInstance(const FGuid& Guid);

	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	bool CancelSequence(const FGuid& Guid);

private:
	// Index-Code; code-> 1= pause, 2-resume, 3-cancel, 4-Reset
	TMap<FGuid, int32> _tweenStatusChangeRequest;
	TArray<FPulseTweenInstance> _tweenInstances;
	TMap<FGuid, int32> _tweenInstanceIndexesMap;
	TSpscQueue<FPulseTweenInstance> _newTweensQueue;
	TArray<FPulseTweenSequence> _ActiveSequences;

	TArray<int32> _reusedIndexList;
	TSpscQueue<int32> _reusedIndexQueue;
	TArray<FGuid> _unUsedGUIDs;
	TSet<FGuid> _updatingSet;
	TSet<FGuid> _startedSet;
	TSet<FGuid> _pausedSet;
	TSet<FGuid> _resumedSet;
	TSet<FGuid> _completedSet;
	TSet<FGuid> _cancelledSet;
	TSet<FGuid> _pingPongApexSet;
	TSet<FGuid> _loopSet;
	TSet<FGuid> _sequenceCompletedSet;
	TSet<FGuid> _sequenceMoveNextSet;
	TSet<FGuid> _sequenceChkSet;
	UPROPERTY()
	TObjectPtr<UWorld> _world;
	int32 _tweenCountMT = -1;

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
	bool AddToStatusSet(const FPulseTweenInstance& tweenInstance, FRWLock& Lock);
	bool MustTriggerEvents() const;

	FGuid GetTweenNewGuid();

public:
	// Compare two tween params
	UFUNCTION(BlueprintPure, DisplayName= "==", Category="PulseCore|Tweening", meta=(Keywords="=, ==, equal"))
	static bool TweenParamsEquals(const FTweenParams& Params1, const FTweenParams& Params2);

	// Compare two tween params
	UFUNCTION(BlueprintPure, DisplayName= "!=", Category="PulseCore|Tweening", meta=(Keywords="!, !=, not equal"))
	static bool TweenParamsNotEquals(const FTweenParams& Params1, const FTweenParams& Params2);

	// Print real time tween debug data on screen.
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening")
	void DebugTween(FLinearColor Color);

	// Check if the tween ID is a valid and point to an active tween instance
	UFUNCTION(BlueprintPure, Category="PulseCore|Tweening")
	bool IsActiveTween(const FGuid& TweenGUID) const;

	// Get the current eased value and the completion percentage of an active tween from it's UID
	UFUNCTION(BlueprintPure, Category="PulseCore|Tweening")
	bool GetTweenValues(const FGuid& TweenGUID, float& OutTweenValue, float& OutTweenPercentage);

	// Check if a tween is from a tween sequence, and return the sequence's Id
	UFUNCTION(BlueprintPure, Category="PulseCore|Tweening")
	bool GetTweenSequenceID(const FGuid& TweenGUID, FGuid& OutSequenceID);

	// Get the Id and the index of the current tween of a sequence
	UFUNCTION(BlueprintPure, Category="PulseCore|Tweening")
	bool GetSequenceCurrentTweenID(const FGuid& SequenceID, FGuid& OutTweenID, int32& OutTweenIndex);

	// Get the number of tween instances of a sequence
	UFUNCTION(BlueprintPure, Category="PulseCore|Tweening")
	bool GetSequenceTweenCount(const FGuid& SequenceID, int32& OutTweenCount);

	// Get the current eased value, the completion percentage and the sequence overall Completion value it's sequence UID
	UFUNCTION(BlueprintPure, Category="PulseCore|Tweening")
	bool GetTweenSequenceValues(const FGuid& SequenceGUID, float& OutTweenValue, float& OutTweenPercentage, float& OutOverallPercentage);

	/**
	 * @brief Create a tween instance from parameters and start tweening.
	 * @param Owner The object from which to get the actual world we're tweening in
	 * @param OutTweenID The output tween UID
	 * @param TweenParams The parameters of the tween.
	 * @param bAttachToOwner Bound the lifetime of this tween instance to the owning object (the world context object this function is called from)
	 **/
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening", meta=(AdvancedDisplay = 5, WorldContext = "Owner"))
	bool CreateNewTween(const UObject* Owner, FGuid& OutTweenID, FTweenParams TweenParams, bool bAttachToOwner = false);

	/**
	 * @brief Create a tween Sequence from parameters and start tweening.
	 * @param Owner The object from which to get the actual world we're tweening in
	 * @param OutSequenceID The output Sequence UID
	 * @param TweenParams The parameters of the tweens in the sequence.
	 * @param SequenceLoops Loop count for the entire sequence. use < 0 for infinite loops
	 * @param bAttachToOwner Bound the lifetime of this tween sequence to the owning object (the world context object this function is called from)
	 **/
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tweening", meta=(AdvancedDisplay = 3, WorldContext = "Owner", AutoCreateRefTerm = "TweenParams"))
	bool CreateNewSequence(const UObject* Owner, FGuid& OutSequenceID, const TArray<FTweenParams>& TweenParams, int32 SequenceLoops = 0, bool bAttachToOwner = false);

	// Start tweening Tween Instance and return it's UID.
	static FGuid Tween(const UObject* Owner, const FPulseTweenInstance& TweenInstance);
};


/**
 * @brief Utility object to easily handle Tween events in blueprint.
 */
UCLASS(NotBlueprintable, BlueprintType)
class UPulseTweenEventListener : public UObject
{
	GENERATED_BODY()

public:
	UPulseTweenEventListener();
	virtual ~UPulseTweenEventListener() override;

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
