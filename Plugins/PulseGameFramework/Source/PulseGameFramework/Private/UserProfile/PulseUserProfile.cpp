// Copyright © by Tyni Boat. All Rights Reserved.


#include "UserProfile/PulseUserProfile.h"

#include "PulseGameFramework.h"
#include "Core/PulseSystemLibrary.h"
#include "Kismet/GameplayStatics.h"


void UBaseUserProfileProvider::Initialize()
{
	OnInitialized();
}

void UBaseUserProfileProvider::SessionCreation()
{
	OnSessionCreation();
}

void UBaseUserProfileProvider::SessionLoaded(UUserProfileSessionSave* SessionFile)
{
	if (!SessionFile)
		return;
	OnSessionLoaded(SessionFile);
}

void UBaseUserProfileProvider::Deinitialize()
{
	OnDeinitialized();
}

void UBaseUserProfileProvider::OnInitialized_Implementation()
{
}

void UBaseUserProfileProvider::OnSessionCreation_Implementation()
{
}

void UBaseUserProfileProvider::OnSessionLoaded_Implementation(UUserProfileSessionSave* SessionFile)
{
}

void UBaseUserProfileProvider::OnDeinitialized_Implementation()
{
}

void UBaseUserProfileProvider::QueryUser(const FString& LocalUserID, const TArray<FString>& RequestParams)
{
	ProviderStatus = EUserProfileProcessStatus::QueryingUser;
	_targetUserLocalID = LocalUserID;
	OnProviderStatusChanged.Broadcast();
	OnQueryUser(LocalUserID, RequestParams);
}

void UBaseUserProfileProvider::OnQueryUser_Implementation(const FString& LocalUserID, const TArray<FString>& RequestParams)
{
	StopAsyncRequest(false, "", 0, {});
}

void UBaseUserProfileProvider::SyncUserProfileData(const FString& LocalUserID, const FUserProfileData& UserData)
{
	ProviderStatus = EUserProfileProcessStatus::SyncingUser;
	_targetUserLocalID = LocalUserID;
	OnProviderStatusChanged.Broadcast();
	OnTryUserDataSynchro(LocalUserID, UserData);
}

void UBaseUserProfileProvider::UploadUserLog(const FString& LocalUserID, const FUserLogs& UserLogs)
{
	OnTryUserLogsUpload(LocalUserID, UserLogs);
}

void UBaseUserProfileProvider::UploadUserAnalytics(const FString& LocalUserID, const FUserAnalytics& UserAnalytics)
{
	OnTryUserAnalyticUpload(LocalUserID, UserAnalytics);
}


void UBaseUserProfileProvider::OnTryUserDataSynchro_Implementation(const FString& LocalUserID, const FUserProfileData& UserData)
{
	StopAsyncRequest(false, "", 0, {});
}

void UBaseUserProfileProvider::OnTryUserLogsUpload_Implementation(const FString& LocalUserID, const FUserLogs& UserLogs)
{
	StopAsyncRequest(false, "", 0, {});
}

void UBaseUserProfileProvider::OnTryUserAnalyticUpload_Implementation(const FString& LocalUserID, const FUserAnalytics& UserAnalytics)
{
	StopAsyncRequest(false, "", 0, {});
}

FText UBaseUserProfileProvider::ResponseCodeMessage_Implementation(int32 ResponseCode)
{
	return {};
}

void UBaseUserProfileProvider::StopAsyncRequest(bool RequestSucceeded, FString UID, int32 ResponseCode, const FUserProfileData& ProfileData, const FString& Token)
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

FString UBaseUserProfileProvider::GetUserToken(const FString& UserLocalID)
{
	if (!_perUserToken.Contains(UserLocalID))
		return "";
	return _perUserToken[UserLocalID];
}

bool UBaseUserProfileProvider::GetOpUserProfileData(FUserProfileData& OutProfileData) const
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

void UBaseUserProfileProvider::SetUserToken(const FString& UserLocalID, const FString& Token)
{
	if (!_perUserToken.Contains(UserLocalID))
		_perUserToken.Add(UserLocalID, Token);
	else
		_perUserToken[UserLocalID] = Token;
}

TArray<FString> UBaseUserProfileProvider::TryGetQueryParamsFromToken_Implementation(const FString& UserLocalID)
{
	return {};
}


UPulseUserProfile* UPulseUserProfile::Get()
{
	return GEngine->GetEngineSubsystem<UPulseUserProfile>();
}

bool UPulseUserProfile::IsPendingNewUserQuery(const FString& LocalUserID, FUserProfileData& OutUserProfileData) const
{
	if (_pendingNewUserQueries.Contains(LocalUserID))
	{
		OutUserProfileData = _pendingNewUserQueries[LocalUserID];
		return true;
	}
	return false;
}

void UPulseUserProfile::OnSessionSavedFunc(const FString& SlotName, int userIndex, bool bSucess)
{
	_status = EUserProfileProcessStatus::Idle;
	ObserveStatusChanges_Internal();
	UE_LOG(LogPulseUserProfile, Log, TEXT("User session file Saved: %s"), *FString(bSucess ? "Success" : "Failure"));
	if (!bSucess)
		return;
	OnSessionSaved.Broadcast();
}


void UPulseUserProfile::OnSessionLoadedFunc(const FString& SlotName, int userIndex, USaveGame* SessionSaveObject)
{
	_status = EUserProfileProcessStatus::Idle;
	ObserveStatusChanges_Internal();
	if (!SessionSaveObject)
	{
		OnSessionCreation.Broadcast();
		if (_currentProfileProvider)
		{
			_currentProfileProvider->SessionCreation();
		}
		UE_LOG(LogPulseUserProfile, Warning, TEXT("Unable to Load User session file: Null Object loaded"));
		return;
	}
	auto castedObject = Cast<UUserProfileSessionSave>(SessionSaveObject);
	if (!castedObject)
	{
		UE_LOG(LogPulseUserProfile, Warning, TEXT("Unable to Load User session file: Cast failed from %s to UserProfileSessionSave"), *SessionSaveObject->GetClass()->GetName());
		return;
	}
	UE_LOG(LogPulseUserProfile, Log, TEXT("User session file loaded: object: %s, Profiles Count: %d"), *castedObject->GetName(), castedObject->UserProfiles.Num());
	bIsAutoLoginActive = !castedObject->CurrentUserLocalID.IsEmpty();
	for (const auto& profile : castedObject->UserProfiles)
	{
		if (_userProfiles.Contains(profile.LocalID))
			continue;
		AddUser(profile, castedObject->CurrentUserLocalID == profile.LocalID);
	}
	if (_currentProfileProvider)
	{
		for (const auto& tokenPair : castedObject->LastSessionToken)
			_currentProfileProvider->SetUserToken(tokenPair.Key, tokenPair.Value);
		_currentProfileProvider->SessionLoaded(castedObject);
		FUserProfile currentProfile;
		if (GetUserProfile_Internal(CurrentUserProfileLocalID, currentProfile) && GetUserProfileStatus_Internal() == EUserProfileProcessStatus::Idle)
			_currentProfileProvider->SyncUserProfileData(currentProfile.LocalID, currentProfile.ProfileData);
	}
	OnSessionLoaded.Broadcast();
}

void UPulseUserProfile::OnProviderQueryUserCompleted(const EUserProfileProcessStatus& RequestType, const FString& LocalID, const FString& UID, bool Success, int32 ResponseCode,
                                                     const FUserProfileData& ProfileData)
{
	UE_LOG(LogPulseUserProfile, Log, TEXT("Query(Login/Register) User completed: Success: %s User ID: %s"), *FString(Success? "True" : "False"), *LocalID);
	FUserProfileData userData = ProfileData;
	if (_pendingNewUserQueries.Contains(LocalID))
	{
		userData = _pendingNewUserQueries[LocalID];
		_pendingNewUserQueries.Remove(LocalID);
	}
	if (!_userProfiles.Contains(LocalID))
	{
		auto userEntry = FUserEntry(LocalID);
		AddUserProfile_Internal(userEntry.Profile);
	}
	_userProfiles[LocalID].Profile.UserUID = UID;
	_userProfiles[LocalID].Profile.ProfileData = userData;
	_userProfiles[LocalID].Status = Success ? EUserProfileStatus::LoggedIn : _userProfiles[LocalID].Status;
	OnUserQuerySucceed.Broadcast(LocalID, LocalID == CurrentUserProfileLocalID);
	SyncUserProfileDatas(LocalID);
}

void UPulseUserProfile::OnProviderSyncUserCompleted(const EUserProfileProcessStatus& RequestType, const FString& LocalID, const FString& UID, bool Success, int32 ResponseCode,
                                                    const FUserProfileData& ProfileData)
{
	UE_LOG(LogPulseUserProfile, Log, TEXT("Sync User Data completed: Success: %s User ID: %s Is Valid User: %s"), *FString(Success? "True" : "False"), *LocalID,
	       *FString(_userProfiles.Contains(LocalID)? "True" : "False"));
	if (!_userProfiles.Contains(LocalID))
		return;
	_userProfiles[LocalID].Profile.ProfileData = ProfileData;
	OnUserSyncSucceed.Broadcast(LocalID, LocalID == CurrentUserProfileLocalID);
	SaveUserProfile_Internal();
}

void UPulseUserProfile::ObserveStatusChanges_Internal()
{
	const auto newStatus = GetUserProfileStatus_Internal();
	if (newStatus == _lastObservedStatus)
		return;
	UE_LOG(LogPulseUserProfile, Log, TEXT("User Profile Manager State changed from %s to %s"), *UEnum::GetValueAsString(_lastObservedStatus), *UEnum::GetValueAsString(newStatus));
	OnSystemStatusChanged.Broadcast(_lastObservedStatus, newStatus);
	_lastObservedStatus = newStatus;
}

EUserProfileProcessStatus UPulseUserProfile::GetUserProfileStatus_Internal() const
{
	auto status = _status;
	if (_currentProfileProvider)
		status = _currentProfileProvider->ProviderStatus != EUserProfileProcessStatus::Idle ? _currentProfileProvider->ProviderStatus : status;
	return status;
}

UBaseUserProfileProvider* UPulseUserProfile::GetUserProfileProvider_Internal() const
{
	if (_currentProfileProvider)
		return _currentProfileProvider;
	return nullptr;
}

void UPulseUserProfile::Deinitialize()
{
	SaveUserProfile_Internal();
	SetCurrentProvider_Internal(nullptr);
	Super::Deinitialize();
}

void UPulseUserProfile::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogPulseUserProfile, Log, TEXT("Pulse user profile sub-system initialization"));
	_userProfiles.Empty();
	if (const auto projectSettings = GetProjectSettings())
	{
		// Create provider Instance
		SetCurrentProvider_Internal(projectSettings->UserProfileProviderClass);
		// Load saved session
		LoadUserProfile_Internal();
	}
}

EUserProfileProcessStatus UPulseUserProfile::GetSystemStatus() const
{
	return GetUserProfileStatus_Internal();
}

FUserProfile UPulseUserProfile::GetCurrentUserProfile(bool& OutValidUser) const
{
	FUserProfile userProfile = {};
	OutValidUser = GetUserProfile_Internal(CurrentUserProfileLocalID, userProfile);
	return userProfile;
}

bool UPulseUserProfile::GetUserProfile(const FString& LocalUserID, FUserProfile& OutProfile) const
{
	return GetUserProfile_Internal(LocalUserID, OutProfile);
}

bool UPulseUserProfile::GetUserProfilesList(TArray<FString>& OutUserLocalIDs) const
{
	return GetUserProfileList_Internal(OutUserLocalIDs);
}

UBaseUserProfileProvider* UPulseUserProfile::GetUserProfileProvider() const
{
	return GetUserProfileProvider_Internal();
}

bool UPulseUserProfile::GetUserLoginStatus(const FString& LocalUserID, EUserProfileStatus& OutStatus) const
{
	return GetUserLoginStatus_Internal(LocalUserID, OutStatus);
}

bool UPulseUserProfile::GetCurrentUserLoginStatus(EUserProfileStatus& OutStatus) const
{
	return GetUserLoginStatus_Internal(CurrentUserProfileLocalID, OutStatus);
}

void UPulseUserProfile::SaveUserProfileSession()
{
	SaveUserProfile_Internal();
}

void UPulseUserProfile::LoadUserProfileSession()
{
	LoadUserProfile_Internal();
}

void UPulseUserProfile::SetAutoLogin(bool enable)
{
	if (GetUserProfileStatus_Internal() == EUserProfileProcessStatus::LoadingSession)
		return;
	if (bIsAutoLoginActive == enable)
		return;
	bIsAutoLoginActive = enable;
	SaveUserProfile_Internal();
}

void UPulseUserProfile::LogInOrRegisterUser(const FUserProfile& UserProfile, TArray<FString> RequestParams)
{
	LogInOrRegisterUser_Internal(UserProfile, RequestParams);
}

void UPulseUserProfile::LogInOrRegisterCurrentUser(TArray<FString> RequestParams)
{
	if (!_userProfiles.Contains(CurrentUserProfileLocalID))
		return;
	LogInOrRegisterUser_Internal(_userProfiles[CurrentUserProfileLocalID].Profile, RequestParams);
}

void UPulseUserProfile::LogOutCurrentUser()
{
	if (_userProfiles.Contains(CurrentUserProfileLocalID))
	{
		_userProfiles[CurrentUserProfileLocalID].Status = EUserProfileStatus::Guest;
	}
	ChangeUser_Internal("");
}

void UPulseUserProfile::SyncUserProfileDatas(const FString& LocalUserID)
{
	TrySyncUser_Internal(LocalUserID);
}

void UPulseUserProfile::SyncCurrentUserProfileDatas()
{
	TrySyncUser_Internal(CurrentUserProfileLocalID);
}

bool UPulseUserProfile::SetCurrentUser(const FString& LocalUserID)
{
	return ChangeUser_Internal(LocalUserID);
}

bool UPulseUserProfile::DeleteUser(const FString& LocalUserID)
{
	return RemoveUserProfile_Internal(LocalUserID);
}

bool UPulseUserProfile::AddUser(const FUserProfile& UserProfile, bool SetAsCurrent)
{
	return AddUserProfile_Internal(UserProfile, SetAsCurrent);
}

bool UPulseUserProfile::CreateNewUserProfile(const FString& Pseudo, bool SetAsCurrent)
{
	return CreateNewUserProfile_Internal(Pseudo, SetAsCurrent);
}


void UPulseUserProfile::SaveUserProfile_Internal()
{
	auto status = GetUserProfileStatus_Internal();
	UE_LOG(LogPulseUserProfile, Log, TEXT("Trying to save User session file: status= %s"), *UEnum::GetValueAsString(status));
	if (GetUserProfileStatus_Internal() != EUserProfileProcessStatus::Idle)
		return;
	if (auto SaveGameInstance = UGameplayStatics::CreateSaveGameObject(UUserProfileSessionSave::StaticClass()))
	{
		auto castedInstance = Cast<UUserProfileSessionSave>(SaveGameInstance);
		if (!castedInstance)
			return;
		_status = EUserProfileProcessStatus::SavingSession;
		ObserveStatusChanges_Internal();
		for (const auto& entry : _userProfiles)
		{
			if (entry.Value.Status == EUserProfileStatus::None)
				continue;
			if (entry.Value.Profile.LocalID != entry.Key)
				continue;
			castedInstance->UserProfiles.Add(entry.Value.Profile);
			if (_currentProfileProvider)
				castedInstance->LastSessionToken.Add(entry.Key, _currentProfileProvider->GetUserToken(entry.Key));
		}
		UE_LOG(LogPulseUserProfile, Log, TEXT("User session file Save: object: %s, Profiles Count: %d"), *castedInstance->GetName(), castedInstance->UserProfiles.Num());
		castedInstance->CurrentUserLocalID = bIsAutoLoginActive ? CurrentUserProfileLocalID : "";
		// Set up the (optional) delegate.
		FAsyncSaveGameToSlotDelegate SavedDelegate;
		// USomeUObjectClass::SaveGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, bool bSuccess
		SavedDelegate.BindUObject(this, &UPulseUserProfile::OnSessionSavedFunc);
		// Start async save process.
		UGameplayStatics::AsyncSaveGameToSlot(SaveGameInstance, SESSION_SAVE_SLOT, 0, SavedDelegate);
	}
}

void UPulseUserProfile::LoadUserProfile_Internal()
{
	auto status = GetUserProfileStatus_Internal();
	UE_LOG(LogPulseUserProfile, Log, TEXT("Trying to load user session file. Profile status: %s"), *UEnum::GetValueAsString(status));
	if (status != EUserProfileProcessStatus::Idle)
		return;
	// Set up the delegate.
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	// USomeUObjectClass::LoadGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData
	LoadedDelegate.BindUObject(this, &UPulseUserProfile::OnSessionLoadedFunc);
	// Start the load process
	_status = EUserProfileProcessStatus::LoadingSession;
	ObserveStatusChanges_Internal();
	UGameplayStatics::AsyncLoadGameFromSlot(SESSION_SAVE_SLOT, 0, LoadedDelegate);
}

void UPulseUserProfile::LogInOrRegisterUser_Internal(const FUserProfile& UserProfile, const TArray<FString>& RequestParams)
{
	if (!_currentProfileProvider)
		return;
	if (UserProfile.LocalID.IsEmpty())
		return;
	if (_pendingNewUserQueries.Contains(UserProfile.LocalID))
		return;
	if (!_userProfiles.Contains(UserProfile.LocalID))
		_pendingNewUserQueries.Add(UserProfile.LocalID, UserProfile.ProfileData);
	UE_LOG(LogPulseUserProfile, Log, TEXT("Trying to Query(Login/Register) User: User ID: %s"), *UserProfile.LocalID);
	_currentProfileProvider->QueryUser(UserProfile.LocalID, RequestParams);
}

void UPulseUserProfile::TrySyncUser_Internal(const FString& LocalUserID) const
{
	if (!_currentProfileProvider)
		return;
	FUserProfile userProf;
	if (!GetUserProfile_Internal(LocalUserID, userProf))
		return;
	UE_LOG(LogPulseUserProfile, Log, TEXT("Trying to Sync User profile datas: User ID: %s"), *userProf.LocalID);
	_currentProfileProvider->SyncUserProfileData(userProf.LocalID, userProf.ProfileData);
}

void UPulseUserProfile::SetCurrentProvider_Internal(TSubclassOf<UObject> NewProviderClass)
{
	bool hadAProvider = false;
	if (_currentProfileProvider)
	{
		hadAProvider = true;
		if (NewProviderClass == _currentProfileProvider.GetClass())
			return;
		_currentProfileProvider->OnQueryUserRequestSucceed.RemoveDynamic(this, &UPulseUserProfile::OnProviderQueryUserCompleted);
		_currentProfileProvider->OnSyncUserRequestSucceed.RemoveDynamic(this, &UPulseUserProfile::OnProviderSyncUserCompleted);
		_currentProfileProvider->OnProviderStatusChanged.RemoveDynamic(this, &UPulseUserProfile::ObserveStatusChanges_Internal);
		_currentProfileProvider->OnDeinitialized();
	}
	if (hadAProvider)
		OnUserProviderPreChanged.Broadcast();
	_currentProfileProvider = nullptr;
	if (NewProviderClass)
		_currentProfileProvider = NewObject<UBaseUserProfileProvider>((UObject*)GetTransientPackage(), NewProviderClass);
	if (_currentProfileProvider)
	{
		_currentProfileProvider->OnQueryUserRequestSucceed.AddDynamic(this, &UPulseUserProfile::OnProviderQueryUserCompleted);
		_currentProfileProvider->OnSyncUserRequestSucceed.AddDynamic(this, &UPulseUserProfile::OnProviderSyncUserCompleted);
		_currentProfileProvider->OnProviderStatusChanged.AddDynamic(this, &UPulseUserProfile::ObserveStatusChanges_Internal);
		_currentProfileProvider->OnInitialized();
	}
	if (hadAProvider)
		OnUserProviderPostChanged.Broadcast();
	UE_LOG(LogPulseUserProfile, Log, TEXT("User profiler provider set to %s"), *(_currentProfileProvider ? _currentProfileProvider.GetClass()->GetName() : TEXT("Null")));
}

bool UPulseUserProfile::ChangeUser_Internal(const FString& LocalUserID)
{
	if (LocalUserID == CurrentUserProfileLocalID)
		return false;
	if (!LocalUserID.IsEmpty())
		if (!_userProfiles.Contains(LocalUserID))
			return false;
	const auto lastUser = CurrentUserProfileLocalID;
	CurrentUserProfileLocalID = LocalUserID;
	OnCurrentUserChanged.Broadcast(CurrentUserProfileLocalID);
	return true;
}

bool UPulseUserProfile::GetUserProfile_Internal(const FString& LocalUserID, FUserProfile& OutProfile) const
{
	OutProfile = {};
	if (!_userProfiles.Contains(LocalUserID))
		return false;
	OutProfile = _userProfiles[LocalUserID].Profile;
	return true;
}

bool UPulseUserProfile::GetUserProfileList_Internal(TArray<FString>& OutLocalUserIDs) const
{
	OutLocalUserIDs.Empty();
	for (const auto& entry : _userProfiles)
	{
		OutLocalUserIDs.Add(entry.Key);
	}
	return OutLocalUserIDs.Num() > 0;
}

bool UPulseUserProfile::CreateNewUserProfile_Internal(const FString& Pseudo, bool SetAsCurrent)
{
	if (Pseudo.IsEmpty())
		return false;
	FUserEntry newEntry = FUserEntry(FGuid::NewGuid().ToString());
	newEntry.Profile.ProfileData.Pseudo = Pseudo;
	return AddUserProfile_Internal(newEntry.Profile, SetAsCurrent);
}

bool UPulseUserProfile::AddUserProfile_Internal(const FUserProfile& UserProfile, bool SetAsCurrent)
{
	FGuid guid;
	if (UserProfile.LocalID.IsEmpty() || !FGuid::Parse(UserProfile.LocalID, guid))
		return false;
	if (!guid.IsValid())
		return false;
	FUserEntry newEntry = FUserEntry();
	newEntry.Profile = UserProfile;
	if (_userProfiles.Contains(UserProfile.LocalID))
	{
		_userProfiles[UserProfile.LocalID] = newEntry;
		OnUserUpdated.Broadcast(UserProfile.LocalID);
	}
	else
	{
		_userProfiles.Add(UserProfile.LocalID, newEntry);
		OnUserAdded.Broadcast(UserProfile.LocalID);
	}
	if (SetAsCurrent)
		ChangeUser_Internal(UserProfile.LocalID);
	return true;
}

bool UPulseUserProfile::RemoveUserProfile_Internal(const FString& LocalUserId)
{
	if (LocalUserId == CurrentUserProfileLocalID)
		ChangeUser_Internal("");
	if (!_userProfiles.Contains(LocalUserId))
		return false;
	_userProfiles.Remove(LocalUserId);
	OnUserDeleted.Broadcast(LocalUserId);
	return true;
}

bool UPulseUserProfile::GetUserLoginStatus_Internal(const FString& LocalUserID, EUserProfileStatus& OutStatus) const
{
	if (!_userProfiles.Contains(LocalUserID))
		return false;
	OutStatus = _userProfiles[LocalUserID].Status;
	return true;
}




UPulseUserProfileEventListener::UPulseUserProfileEventListener()
{
	const auto userSubSystem = UPulseUserProfile::Get();
	if (!userSubSystem)
		return;
	userSubSystem->OnSessionSaved.AddDynamic(this, &UPulseUserProfileEventListener::OnSessionSaved_Func);
	userSubSystem->OnSessionLoaded.AddDynamic(this, &UPulseUserProfileEventListener::OnSessionSaved_Func);
	userSubSystem->OnSessionCreation.AddDynamic(this, &UPulseUserProfileEventListener::OnSessionSaved_Func);
	userSubSystem->OnUserQuerySucceed.AddDynamic(this, &UPulseUserProfileEventListener::OnUserQuerySucceed_Func);
	userSubSystem->OnUserSyncSucceed.AddDynamic(this, &UPulseUserProfileEventListener::OnUserSyncSucceed_Func);
	userSubSystem->OnUserProviderPreChanged.AddDynamic(this, &UPulseUserProfileEventListener::OnUserProviderPreChanged_Func);
	userSubSystem->OnUserProviderPostChanged.AddDynamic(this, &UPulseUserProfileEventListener::OnUserProviderChanged_Func);
	userSubSystem->OnUserAdded.AddDynamic(this, &UPulseUserProfileEventListener::OnUserAdded_Func);
	userSubSystem->OnUserUpdated.AddDynamic(this, &UPulseUserProfileEventListener::OnUserUpdated_Func);
	userSubSystem->OnUserDeleted.AddDynamic(this, &UPulseUserProfileEventListener::OnUserDeleted_Func);
	userSubSystem->OnCurrentUserChanged.AddDynamic(this, &UPulseUserProfileEventListener::OnCurrentUserChanged_Func);
	userSubSystem->OnSystemStatusChanged.AddDynamic(this, &UPulseUserProfileEventListener::OnSystemStatusChanged_Func);
	BindCurrentProvider();
}

UPulseUserProfileEventListener::~UPulseUserProfileEventListener()
{
	UnBindCurrentProvider();
	const auto userSubSystem = UPulseUserProfile::Get();
	if (!userSubSystem)
		return;
	userSubSystem->OnSessionSaved.RemoveDynamic(this, &UPulseUserProfileEventListener::OnSessionSaved_Func);
	userSubSystem->OnSessionLoaded.RemoveDynamic(this, &UPulseUserProfileEventListener::OnSessionSaved_Func);
	userSubSystem->OnSessionCreation.RemoveDynamic(this, &UPulseUserProfileEventListener::OnSessionSaved_Func);
	userSubSystem->OnUserQuerySucceed.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserQuerySucceed_Func);
	userSubSystem->OnUserSyncSucceed.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserSyncSucceed_Func);
	userSubSystem->OnUserProviderPreChanged.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserProviderPreChanged_Func);
	userSubSystem->OnUserProviderPostChanged.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserProviderChanged_Func);
	userSubSystem->OnUserAdded.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserAdded_Func);
	userSubSystem->OnUserUpdated.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserUpdated_Func);
	userSubSystem->OnUserDeleted.RemoveDynamic(this, &UPulseUserProfileEventListener::OnUserDeleted_Func);
	userSubSystem->OnCurrentUserChanged.RemoveDynamic(this, &UPulseUserProfileEventListener::OnCurrentUserChanged_Func);
	userSubSystem->OnSystemStatusChanged.RemoveDynamic(this, &UPulseUserProfileEventListener::OnSystemStatusChanged_Func);
}

void UPulseUserProfileEventListener::BindCurrentProvider()
{
	const auto userSubSystem = UPulseUserProfile::Get();
	if (!userSubSystem)
		return;
	if (auto provider = userSubSystem->GetUserProfileProvider())
	{
		provider->OnAsyncRequestCompleted.AddDynamic(this, &UPulseUserProfileEventListener::OnProviderAsyncRequestCompleted_Func);
	}
}

void UPulseUserProfileEventListener::UnBindCurrentProvider()
{
	const auto userSubSystem = UPulseUserProfile::Get();
	if (!userSubSystem)
		return;
	if (auto provider = userSubSystem->GetUserProfileProvider())
	{
		provider->OnAsyncRequestCompleted.RemoveDynamic(this, &UPulseUserProfileEventListener::OnProviderAsyncRequestCompleted_Func);
	}
}

void UPulseUserProfileEventListener::OnSessionSaved_Func()
{
	OnSessionSaved.Broadcast();
}

void UPulseUserProfileEventListener::OnSessionLoaded_Func()
{
	OnSessionLoaded.Broadcast();
}

void UPulseUserProfileEventListener::OnSessionCreation_Func()
{
	OnSessionCreation.Broadcast();
}

void UPulseUserProfileEventListener::OnUserQuerySucceed_Func(const FString& LocalUserID, bool IsCurrentUser)
{
	OnUserQuerySucceed.Broadcast(LocalUserID, IsCurrentUser);
}

void UPulseUserProfileEventListener::OnUserSyncSucceed_Func(const FString& LocalUserID, bool IsCurrentUser)
{
	OnUserSyncSucceed.Broadcast(LocalUserID, IsCurrentUser);
}

void UPulseUserProfileEventListener::OnUserProviderPreChanged_Func()
{
	UnBindCurrentProvider();
}

void UPulseUserProfileEventListener::OnUserProviderChanged_Func()
{
	BindCurrentProvider();
	OnUserProviderChanged.Broadcast();
}

void UPulseUserProfileEventListener::OnUserAdded_Func(const FString& LocalUID)
{
	OnUserAdded.Broadcast(LocalUID);
}

void UPulseUserProfileEventListener::OnUserUpdated_Func(const FString& LocalUID)
{
	OnUserUpdated.Broadcast(LocalUID);
}

void UPulseUserProfileEventListener::OnUserDeleted_Func(const FString& LocalUID)
{
	OnUserDeleted.Broadcast(LocalUID);
}

void UPulseUserProfileEventListener::OnCurrentUserChanged_Func(const FString& LocalUID)
{
	OnCurrentUserChanged.Broadcast(LocalUID);
}

void UPulseUserProfileEventListener::OnSystemStatusChanged_Func(EUserProfileProcessStatus LastStatus, EUserProfileProcessStatus NewStatus)
{
	OnSystemStatusChanged.Broadcast(LastStatus, NewStatus);
}

void UPulseUserProfileEventListener::OnProviderAsyncRequestCompleted_Func(const EUserProfileProcessStatus& RequestType, const FString& LocalID, const FString& UID, bool Success,
	int32 ResponseCode, const FUserProfileData& ProfileData)
{
	OnProviderAsyncRequestCompleted.Broadcast(RequestType, LocalID, UID, Success, ResponseCode, ProfileData);
}
