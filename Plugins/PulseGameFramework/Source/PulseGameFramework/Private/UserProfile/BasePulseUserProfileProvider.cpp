// Copyright © by Tyni Boat. All Rights Reserved.


#include "UserProfile/BasePulseUserProfileProvider.h"

#include "UserProfile/PulseUserProfile.h"


void UBasePulseUserProfileProvider::Initialize()
{
	OnInitialized();
}

void UBasePulseUserProfileProvider::SessionCreation()
{
	OnSessionCreation();
}

void UBasePulseUserProfileProvider::SessionLoaded(UUserProfileSessionSave* SessionFile)
{
	if (!SessionFile)
		return;
	OnSessionLoaded(SessionFile);
}

void UBasePulseUserProfileProvider::Deinitialize()
{
	OnDeinitialized();
}

void UBasePulseUserProfileProvider::OnInitialized_Implementation()
{
}

void UBasePulseUserProfileProvider::OnSessionCreation_Implementation()
{
}

void UBasePulseUserProfileProvider::OnSessionLoaded_Implementation(UUserProfileSessionSave* SessionFile)
{
}

void UBasePulseUserProfileProvider::OnDeinitialized_Implementation()
{
}

void UBasePulseUserProfileProvider::QueryUser(const FString& LocalUserID, const TArray<FString>& RequestParams)
{
	ProviderStatus = EUserProfileProcessStatus::QueryingUser;
	_targetUserLocalID = LocalUserID;
	OnProviderStatusChanged.Broadcast();
	OnQueryUser(LocalUserID, RequestParams);
}

void UBasePulseUserProfileProvider::OnQueryUser_Implementation(const FString& LocalUserID, const TArray<FString>& RequestParams)
{
	StopAsyncRequest(false, "", 0, {});
}

void UBasePulseUserProfileProvider::SyncUserProfileData(const FString& LocalUserID, const FUserProfileData& UserData)
{
	ProviderStatus = EUserProfileProcessStatus::SyncingUser;
	_targetUserLocalID = LocalUserID;
	OnProviderStatusChanged.Broadcast();
	OnTryUserDataSynchro(LocalUserID, UserData);
}

void UBasePulseUserProfileProvider::UploadUserLog(const FString& LocalUserID, const FUserLogs& UserLogs)
{
	OnTryUserLogsUpload(LocalUserID, UserLogs);
}

void UBasePulseUserProfileProvider::UploadUserAnalytics(const FString& LocalUserID, const FUserAnalytics& UserAnalytics)
{
	OnTryUserAnalyticUpload(LocalUserID, UserAnalytics);
}


void UBasePulseUserProfileProvider::OnTryUserDataSynchro_Implementation(const FString& LocalUserID, const FUserProfileData& UserData)
{
	StopAsyncRequest(false, "", 0, {});
}

void UBasePulseUserProfileProvider::OnTryUserLogsUpload_Implementation(const FString& LocalUserID, const FUserLogs& UserLogs)
{
	StopAsyncRequest(false, "", 0, {});
}

void UBasePulseUserProfileProvider::OnTryUserAnalyticUpload_Implementation(const FString& LocalUserID, const FUserAnalytics& UserAnalytics)
{
	StopAsyncRequest(false, "", 0, {});
}

FText UBasePulseUserProfileProvider::ResponseCodeMessage_Implementation(int32 ResponseCode)
{
	return {};
}

void UBasePulseUserProfileProvider::StopAsyncRequest(bool RequestSucceeded, FString UID, int32 ResponseCode, const FUserProfileData& ProfileData, const FString& Token)
{
	const auto lastStatus = ProviderStatus;
	ProviderStatus = EUserProfileProcessStatus::Idle;
	OnProviderStatusChanged.Broadcast();
	OnAsyncRequestCompleted.Broadcast(lastStatus, _targetUserLocalID, UID, RequestSucceeded, ResponseCode, ProfileData);
	if (RequestSucceeded)
	{
		switch (lastStatus)
		{
		case EUserProfileProcessStatus::QueryingUser:
			{
				SetUserToken(_targetUserLocalID, Token);
				OnQueryUserRequestSucceed.Broadcast(lastStatus, _targetUserLocalID, UID, RequestSucceeded, ResponseCode, ProfileData);
			}
			break;
		case EUserProfileProcessStatus::SyncingUser:
			OnSyncUserRequestSucceed.Broadcast(lastStatus, _targetUserLocalID, UID, RequestSucceeded, ResponseCode, ProfileData);
			break;
		default:
			break;
		}
	}
	_targetUserLocalID = "";
}

FString UBasePulseUserProfileProvider::GetUserToken(const FString& UserLocalID)
{
	if (!_perUserToken.Contains(UserLocalID))
		return "";
	return _perUserToken[UserLocalID];
}

bool UBasePulseUserProfileProvider::GetOpUserProfileData(FUserProfileData& OutProfileData) const
{
	OutProfileData = {};
	if (_targetUserLocalID.IsEmpty())
		return false;
	if (const auto mgr = UPulseUserProfile::Get())
	{
		if (mgr->IsPendingNewUserQuery(_targetUserLocalID, OutProfileData))
			return true;
		FUserProfile Profile;
		if (!mgr->GetUserProfile(_targetUserLocalID, Profile))
			return false;
		OutProfileData = Profile.ProfileData;
		return true;
	}
	return false;
}

void UBasePulseUserProfileProvider::SetUserToken(const FString& UserLocalID, const FString& Token)
{
	if (!_perUserToken.Contains(UserLocalID))
		_perUserToken.Add(UserLocalID, Token);
	else
		_perUserToken[UserLocalID] = Token;
}

TArray<FString> UBasePulseUserProfileProvider::TryGetQueryParamsFromToken_Implementation(const FString& UserLocalID)
{
	return {};
}