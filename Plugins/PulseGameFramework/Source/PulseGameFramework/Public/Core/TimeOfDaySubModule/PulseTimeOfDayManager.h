// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseSubModuleBase.h"
#include "Core/PulseNetworkingModule/IPulseNetObject.h"
#include "Core/SaveGameSubModule/IPulseSavableObject.h"
#include "Core/SaveGameSubModule/PulseSaveManager.h"
#include "PulseTimeOfDayManager.generated.h"


#pragma region Additionnal Types

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGameTimeChanged, FDateTime, NewTime, FTimespan, TimeOffset);


UCLASS()
class USaveTimeManagerData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Time)
	FDateTime SavedTime;
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Time)
	float AutoTickSpeed = 1;
};

UCLASS()
class USaveTimeNetData : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Time)
	FDateTime GameTime;
	UPROPERTY(visibleAnywhere, BlueprintReadOnly, Category=Time)
	float LerpSpeed;
};

#pragma endregion


/**
 * Pulse Sub-Module for managing the Time of Day
 */
UCLASS(BlueprintType)
class PULSEGAMEFRAMEWORK_API UPulseTimeOfDayManager : public UPulseSubModuleBase, public IIPulseNetObject, public IIPulseSavableObject
{
	GENERATED_BODY()

private:
	FDateTime _currentTime = FDateTime();
	FDateTime _newSetTime = FDateTime();
	float _changeTimeLerpSpeed = 1.0f;
	float _tickSpeed = 1.0f;
	bool _bAutoTickTime = true;
	bool _bTriggerEventOnTick = true;
	bool _interpolateTime = false;
	bool _bCanReplicate = false;
	bool _bCanSave = false;
	FName _TimeReplicationTag = "Pulse.Core.Time.DayTime";
	FName _TimeSpeedReplicationTag = "Pulse.Core.Time.Speed";
	
public:
	
	virtual void OnNetInit_Implementation() override;
	virtual void OnNetValueReplicated_Implementation(const FName Tag, FReplicatedEntry Value, EReplicationEntryOperationType OpType) override;

	virtual UClass* GetSaveClassType_Implementation() override;
	virtual void OnLoadedObject_Implementation(UObject* LoadedObject) override;
	virtual UObject* OnSaveObject_Implementation(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave) override;
	
	virtual FName GetSubModuleName() const override;
	virtual bool WantToTick() const override;
	virtual bool TickWhenPaused() const override;
	virtual void InitializeSubModule(UPulseModuleBase* OwningModule) override;
	virtual void DeinitializeSubModule() override;
	virtual void TickSubModule(float DeltaTime, float CurrentTimeDilation = 1, bool bIsGamePaused = false) override;
	

	UPROPERTY(BlueprintAssignable, Category = "TimeOfDay")
	FOnGameTimeChanged OnGameTimeChanged;


	UFUNCTION(BlueprintPure, Category = "TimeOfDay")
	FORCEINLINE FDateTime GetGameTime() const { return _currentTime; }

	UFUNCTION(BlueprintPure, Category = "TimeOfDay")
	FORCEINLINE FTimespan GetTimeRemaining(FDateTime Time) const { return Time - _currentTime; }

	UFUNCTION(BlueprintPure, Category = "TimeOfDay", meta = (AdvancedDisplay = 0))
	FORCEINLINE FTimespan GetTimeRemainingToNextDay(bool bAtsameHour = false) const
	{
		return GetTimeRemainingToNextDay_Internal(GetGameTime(), bAtsameHour);
	}
	FTimespan GetTimeRemainingToNextDay_Internal(FDateTime Date, bool bAtsameHour) const;

	UFUNCTION(BlueprintPure, Category = "TimeOfDay", meta = (AdvancedDisplay = 0))
	FTimespan GetTimeRemainingToNextWeek(bool bAtsameDay = false, bool bAtsameHour = false) const
	{
		return GetTimeRemainingToNextWeek_Internal(GetGameTime(), bAtsameDay, bAtsameHour);
	}
	FTimespan GetTimeRemainingToNextWeek_Internal(FDateTime Date, bool bAtsameDay = false, bool bAtsameHour = false) const;

	UFUNCTION(BlueprintPure, Category = "TimeOfDay", meta = (AdvancedDisplay = 1))
	FORCEINLINE FTimespan GetTimeRemainingToNextWeekDay(int32 Day, bool bAtsameHour = false) const
	{
		return GetTimeRemainingToNextWeekDay_Internal(GetGameTime(), Day, bAtsameHour);
	}
	FTimespan GetTimeRemainingToNextWeekDay_Internal(FDateTime Date, int32 Day, bool bAtsameHour = false) const;

	UFUNCTION(BlueprintPure, Category = "TimeOfDay", meta = (AdvancedDisplay = 0))
	FORCEINLINE FTimespan GetTimeRemainingToNextMonth(bool bAtsameDay = false, bool bAtsameHour = false) const
	{
		return GetTimeRemainingToNextMonth_Internal(GetGameTime(), bAtsameDay, bAtsameHour);
	}
	FTimespan GetTimeRemainingToNextMonth_Internal(FDateTime Date, bool bAtsameDay = false, bool bAtsameHour = false) const;

	UFUNCTION(BlueprintPure, Category = "TimeOfDay", meta = (AdvancedDisplay = 1))
	FORCEINLINE FTimespan GetTimeRemainingToNextCalendarMonth(int32 Month, bool bAtsameDay = false, bool bAtsameHour = false) const
	{
		return GetTimeRemainingToNextCalendarMonth_Internal(GetGameTime(), Month, bAtsameDay, bAtsameHour);
	}
	FTimespan GetTimeRemainingToNextCalendarMonth_Internal(FDateTime Date, int32 Month, bool bAtsameDay = false, bool bAtsameHour = false) const;

	UFUNCTION(BlueprintCallable, Category = "TimeOfDay", meta = (AdvancedDisplay = 1))
	void SetGameTime(FDateTime Time, float LerpSpeed = 0);

	UFUNCTION(BlueprintCallable, Category = "TimeOfDay")
	void SetAutoTickGameTime(bool AutoTick);

	UFUNCTION(BlueprintCallable, Category = "TimeOfDay")
	void SetAutoTickGameTimeSpeed(float TickSpeed = 1);

	UFUNCTION(BlueprintCallable, Category = "TimeOfDay")
	void SetTriggerEventOnGameTimeTick(bool EnableTrigger);
	
};
