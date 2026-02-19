// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseTween.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/Object.h"
#include "PulseTweenFloatNode.generated.h"


/**
 * Tween from 0.0f to 1.0f
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class PULSEGAMEFRAMEWORK_API UPulseTweenFloatNode : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

protected:
	UFUNCTION()
	void OnUpdate_Internal(TSet<FGuid> Set);	
	UFUNCTION()
	void OnStarted_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnPaused_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnResumed_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnPingPong_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnLooped_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnCompleted_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnSequenceCompleted_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnSequenceMoveNext_Internal(TSet<FGuid> Set);
	UFUNCTION()
	void OnCancelled_Internal(TSet<FGuid> Set);
	
public:
	TArray<FTweenParams> TweenParams;
	int32 LoopSeq = 0;
	FGuid TweenGUID = {};
	FGuid SequenceGUID = {};
	bool isSequence = false;
	UPROPERTY() TObjectPtr<UObject> Owner;
	UPROPERTY() TObjectPtr<UPulseTween> TweenManager;

	UPROPERTY(BlueprintAssignable)
	FTweenUpdateEvent OnTweenUpdate;
	
	UPROPERTY(BlueprintAssignable)
	FPulseTriggerEvent OnTweenCompleted;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnTweenStarted;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnTweenPaused;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnTweenResumed;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnTweenCancelled;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnTweenPingPongApex;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnTweenLooped;
	
	UPROPERTY(BlueprintAssignable, AdvancedDisplay)
	FPulseTriggerEvent OnSequenceCompleted;

	virtual void Activate() override;
	virtual void BeginDestroy() override;

	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tweening")
	void Pause();
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tweening")
	void Resume();
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tweening")
	void Restart();
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tweening")
	void Stop();
	
	/**
	 * @brief Tween a float parameter between 0 and 1
	* @param WorldContext The world context object
	 * @param TweenParameters Parameters of the tween
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = "5", WorldContext = "WorldContext", AutoCreateRefTerm = "TweenParameters"), Category = "PulseCore|Tweening")
	static UPulseTweenFloatNode* PulseTweenValue(UObject* WorldContext, const FTweenParams& TweenParameters);
	
	/**
	 * @brief Make a sequence of multiple float Tween
	* @param WorldContext The world context object
	 * @param SequenceParameters parameters of tween composing the sequence
	 * @param SequenceLoop Loop the entire sequence
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", AdvancedDisplay = "5", WorldContext = "WorldContext", AutoCreateRefTerm = "SequenceParameters"), Category = "PulseCore|Tweening")
	static UPulseTweenFloatNode* PulseTweenSequenceValue(UObject* WorldContext, const TArray<FTweenParams>& SequenceParameters, int32 SequenceLoop = 0);
};
