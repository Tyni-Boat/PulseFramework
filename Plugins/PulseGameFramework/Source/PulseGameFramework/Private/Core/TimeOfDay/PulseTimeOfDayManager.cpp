// Copyright © by Tyni Boat. All Rights Reserved.


#include "Core/TimeOfDaySubModule/PulseTimeOfDayManager.h"
#include "Core/PulseCoreModule.h"
#include "Kismet/KismetSystemLibrary.h"


void UPulseTimeOfDayManager::OnSaveTime_Internal(const FString& SlotName, const int32 UserIndex, USaveMetaWrapper* SaveMetaDataWrapper, bool bAutoSave)
{
	auto coreModule = Cast<UPulseCoreModule>(_OwningModule);
	if (!coreModule)
		return;
	auto saveTime = NewObject<USaveTimeManagerData>(this);
	saveTime->SavedTime = GetGameTime();
	coreModule->GetSaveManager()->WriteAsSavedValue(saveTime, UserIndex);
}

void UPulseTimeOfDayManager::OnLoadGameTime_Internal(ELoadSaveResponse Response, int32 UserIndex, UPulseSaveData* LoadedSaveData)
{
	if (Response != ELoadSaveResponse::Success)
		return;
	if (UserIndex != _loadGameTimeFromUserIndex)
		return;
	if (LoadedSaveData == nullptr)
		return;
	auto coreModule = Cast<UPulseCoreModule>(_OwningModule);
	if (!coreModule)
		return;
	UObject* outObj = nullptr;
	if (!coreModule->GetSaveManager()->ReadSavedValue(USaveTimeManagerData::StaticClass(), outObj, UserIndex))
		return;
	if (auto saveTime = Cast<USaveTimeManagerData>(outObj))
	{
		SetGameTime(saveTime->SavedTime);
	}
}

void UPulseTimeOfDayManager::OnNetInit_Implementation()
{
	TArray<FReplicatedEntry> outNetDatas;
	UKismetSystemLibrary::PrintString(this, "Net Init");
	if (IIPulseNetObject::Execute_TryGetNetRepValue(this, _replicationTag, outNetDatas, false))
	{
		SetGameTime(FDateTime::FromUnixTimestampDecimal(outNetDatas[0].DoubleValue), outNetDatas[0].Float31Value.X, false);
	}
}

void UPulseTimeOfDayManager::OnNetValueReplicated_Implementation(const FGameplayTag Tag, FReplicatedEntry Value, EReplicationEntryOperationType OpType)
{
	if (Tag != _replicationTag)
		return;
	SetGameTime(FDateTime::FromUnixTimestampDecimal(Value.DoubleValue), Value.Float31Value.X);
}

FName UPulseTimeOfDayManager::GetSubModuleName() const
{
	return Super::GetSubModuleName();
}

bool UPulseTimeOfDayManager::WantToTick() const
{
	return true;
}

bool UPulseTimeOfDayManager::TickWhenPaused() const
{
	return true;
}

void UPulseTimeOfDayManager::InitializeSubModule(UPulseModuleBase* OwningModule)
{
	Super::InitializeSubModule(OwningModule);
	auto coreModule = Cast<UPulseCoreModule>(OwningModule);
	if (!coreModule)
		return;
	bool bindSaveMgr = false;
	if (auto settings = coreModule->GetProjectConfig())
	{
		_replicationTag = settings->DateTimeReplicationKey;
		SetGameTime(settings->DefaultGameDate);
		_bAutoTickTime = settings->bTickGameTime;
		_loadGameTimeFromUserIndex = settings->LoadGameTimeFromSaveUserIndex;
		bindSaveMgr = settings->bSaveAndLoadGameTimeAutomatically;
		if (settings->bUseSystemTimeNowOnInitialize)
			SetGameTime(FDateTime::Now());
	}
	if (bindSaveMgr)
	{
		if (auto saveMgr = coreModule->GetSaveManager())
		{
			saveMgr->OnGameAboutToSave.AddDynamic(this, &UPulseTimeOfDayManager::OnSaveTime_Internal);
			saveMgr->OnGameLoaded.AddDynamic(this, &UPulseTimeOfDayManager::OnLoadGameTime_Internal);
		}
	}
}

void UPulseTimeOfDayManager::DeinitializeSubModule()
{
	if (auto coreModule = Cast<UPulseCoreModule>(_OwningModule))
	{
		if (auto saveMgr = coreModule->GetSaveManager())
		{
			saveMgr->OnGameAboutToSave.RemoveDynamic(this, &UPulseTimeOfDayManager::OnSaveTime_Internal);
			saveMgr->OnGameLoaded.RemoveDynamic(this, &UPulseTimeOfDayManager::OnLoadGameTime_Internal);
		}
	}
	Super::DeinitializeSubModule();
}

void UPulseTimeOfDayManager::TickSubModule(float DeltaTime, float CurrentTimeDilation, bool bIsGamePaused)
{
	Super::TickSubModule(DeltaTime, CurrentTimeDilation, bIsGamePaused);
	// Lerp the time
	if (_interpolateTime)
	{
		auto oldTime = _currentTime;
		auto c_ticks = _currentTime.ToUnixTimestampDecimal();
		auto n_ticks = _newSetTime.ToUnixTimestampDecimal();
		c_ticks = _changeTimeLerpSpeed > 0
			          ? FMath::FInterpConstantTo(c_ticks, n_ticks, DeltaTime, _changeTimeLerpSpeed)
			          : n_ticks;
		_currentTime = _currentTime.FromUnixTimestampDecimal(c_ticks);
		if (FMath::IsNearlyEqual(c_ticks, n_ticks))
			_interpolateTime = false;
		OnGameTimeChanged.Broadcast(_currentTime, _currentTime - oldTime);
	}

	// Auto tick time
	if (_bAutoTickTime && !_interpolateTime)
	{
		auto oldTime = _currentTime;
		auto ticks = _currentTime.ToUnixTimestampDecimal();
		ticks += DeltaTime * _tickSpeed;
		_currentTime = _currentTime.FromUnixTimestampDecimal(ticks);
		if (_bTriggerEventOnTick)
			OnGameTimeChanged.Broadcast(_currentTime, _currentTime - oldTime);
	}
}


FTimespan UPulseTimeOfDayManager::GetTimeRemainingToNextDay_Internal(FDateTime Date, bool bAtsameHour) const
{
	int32 hours = bAtsameHour ? 24 : 23 - Date.GetHour();
	bool downAdjustement = bAtsameHour;
	int32 minutes = downAdjustement ? 0 : 59 - Date.GetMinute();
	int32 seconds = downAdjustement ? 0 : 59 - Date.GetSecond();
	return FTimespan(hours, minutes, seconds);
}

FTimespan UPulseTimeOfDayManager::GetTimeRemainingToNextWeek_Internal(FDateTime Date, bool bAtsameDay, bool bAtsameHour) const
{
	int32 days = bAtsameDay ? 7 : 7 - (int32)Date.GetDayOfWeek();
	FTimespan result = FTimespan(FMath::Max(days - 1, 0), 0, 0, 0);
	result += GetTimeRemainingToNextDay_Internal(Date + result, bAtsameHour);
	return result;
}

FTimespan UPulseTimeOfDayManager::GetTimeRemainingToNextWeekDay_Internal(FDateTime Date, int32 Day, bool bAtsameHour) const
{
	int32 dayWeek = Day % 7;
	int32 days = (dayWeek > (int32)Date.GetDayOfWeek()) ? dayWeek - (int32)Date.GetDayOfWeek() : (7 - (int32)Date.GetDayOfWeek()) + dayWeek;
	FTimespan result = FTimespan(FMath::Max(days - 1, 0), 0, 0, 0);
	result += GetTimeRemainingToNextDay_Internal(Date + result, bAtsameHour);
	return result;
}

FTimespan UPulseTimeOfDayManager::GetTimeRemainingToNextMonth_Internal(FDateTime Date, bool bAtsameDay, bool bAtsameHour) const
{
	int32 yearAdd = (Date.GetMonth() + 1) / 12;
	int32 month = (Date.GetMonth() + 1) % 12;
	auto newDate = FDateTime(Date.GetYear() + yearAdd, month, bAtsameDay ? Date.GetDay() : 1);
	newDate += Date.GetTimeOfDay();
	newDate -= FTimespan(1, 0, 0, 0);
	FTimespan result = (newDate - Date) + GetTimeRemainingToNextDay_Internal(newDate, bAtsameHour);
	return result;
}

FTimespan UPulseTimeOfDayManager::GetTimeRemainingToNextCalendarMonth_Internal(FDateTime Date, int32 Month, bool bAtsameDay, bool bAtsameHour) const
{
	int32 calendarMonth = (Month % 12) + 1;
	int32 monthAdd = (calendarMonth > (int32)Date.GetMonthOfYear()) ? calendarMonth - (int32)Date.GetMonthOfYear() : (12 - (int32)Date.GetMonthOfYear()) + calendarMonth;
	int32 yearAdd = calendarMonth <= (int32)Date.GetMonthOfYear() ? 1 : 0;
	int32 month = calendarMonth <= (int32)Date.GetMonthOfYear() ? (Date.GetMonth() + monthAdd) % 12 : Date.GetMonth() + monthAdd;
	auto newDate = FDateTime(Date.GetYear() + yearAdd, month, bAtsameDay ? Date.GetDay() : 1);
	newDate += Date.GetTimeOfDay();
	newDate -= FTimespan(1, 0, 0, 0);
	FTimespan result = (newDate - Date) + GetTimeRemainingToNextDay_Internal(newDate, bAtsameHour);
	return result;
}

void UPulseTimeOfDayManager::SetGameTime(FDateTime Time, float LerpSpeed, bool TryReplicate)
{
	if (Time == _newSetTime) return; // No change in time, do nothing
	_newSetTime = Time;
	_changeTimeLerpSpeed = LerpSpeed * 10000;
	_interpolateTime = true;
	// Replicate if authority
	if (TryReplicate && _replicationTag.IsValid())
	{
		FReplicatedEntry entry = FReplicatedEntry().WithDouble(Time.ToUnixTimestampDecimal()).WithFloat(LerpSpeed);
		IIPulseNetObject::Execute_ReplicateValue(this, _replicationTag, entry);
	}
}

void UPulseTimeOfDayManager::SetAutoTickGameTime(bool AutoTick)
{
	_bAutoTickTime = AutoTick;
}

void UPulseTimeOfDayManager::SetAutoTickGameTimeSpeed(float TickSpeed)
{
	_tickSpeed = TickSpeed;
}

void UPulseTimeOfDayManager::SetTriggerEventOnGameTimeTick(bool EnableTrigger)
{
	_bTriggerEventOnTick = EnableTrigger;
}
