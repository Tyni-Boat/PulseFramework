// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseCoreTypes.h"
#include "GameFramework/SaveGame.h"
#include "PulseUserProfile.generated.h"


UENUM(BlueprintType)
enum class EUserProfileProcessStatus: uint8
{
	Unknow,
	Idle,
	QueryingUser,
	SyncingUser,
	SavingSession,
	LoadingSession,
};


USTRUCT()
struct FUserEntry
{
	GENERATED_BODY()
public:

	FUserEntry(){}
	FUserEntry(const FString& LocalUID) { Profile = FUserProfile(LocalUID); Status = EUserProfileStatus::Guest; }
	
	FUserProfile Profile = {};
	EUserProfileStatus Status = EUserProfileStatus::None;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnSyncUserRequestCompleted, const EUserProfileProcessStatus&, RequestType, const FString&, LocalID, const FString&, UID, bool, Success, int32, ResponseCode, const FUserProfileData&, ProfileData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUserProfileStatusChanged, EUserProfileProcessStatus, LastStatus, EUserProfileProcessStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUserProfileUpdated, const FString&, LocalUserID, bool, IsCurrentUser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserChangeEvents, const FString&, LocalUID);
#define SESSION_SAVE_SLOT "UserSessionSave"



UCLASS(NotBlueprintType, NotBlueprintType)
class UUserProfileSessionSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|UserProfile")
	FString CurrentUserLocalID = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|UserProfile")
	TArray<FUserProfile> UserProfiles = {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PulseCore|UserProfile")
	TMap<FString, FString> LastSessionToken;
};

// Base class for any user profile provider.
UCLASS(Abstract, Blueprintable)
class UBaseUserProfileProvider : public UObject
{
	GENERATED_BODY()

protected:

	FString _targetUserLocalID;
	TMap<FString, FString> _perUserToken;
	
public:
	
	UPROPERTY()
	FPulseTriggerEvent OnProviderStatusChanged;

	// The Status of the provider
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="PulseCore|UserProfile")
	EUserProfileProcessStatus ProviderStatus = EUserProfileProcessStatus::Idle;	

	// Called when ever stop async request function is called. ignoring if the request was successful or not.
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile")
	FOnSyncUserRequestCompleted OnAsyncRequestCompleted;
	
	// Must be called when Query (Login/register) request completed 
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile")
	FOnSyncUserRequestCompleted OnQueryUserRequestSucceed;

	// Must be called when Synchronisation request completed 
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile")
	FOnSyncUserRequestCompleted OnSyncUserRequestSucceed;



	void Initialize();
	// Called when the provider is set as the current User Profile provider 
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnInitialized();
	virtual void OnInitialized_Implementation();

	void SessionCreation();
	// Called when upon the creation of session file
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnSessionCreation();
	virtual void OnSessionCreation_Implementation();

	void SessionLoaded(UUserProfileSessionSave* SessionFile);
	// Called when upon the loading of session file
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnSessionLoaded(UUserProfileSessionSave* SessionFile);
	virtual void OnSessionLoaded_Implementation(UUserProfileSessionSave* SessionFile);

	void Deinitialize();
	// Called when the provider is no longer set as the current User Profile provider 
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnDeinitialized();
	virtual void OnDeinitialized_Implementation();


	// Called to Query (Login/register) a user.
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	void QueryUser(const FString& LocalUserID, const TArray<FString>& RequestParams);
	
	// Called to Query (Login/register) a user. StopAsyncRequest must be called down the line
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnQueryUser(const FString& LocalUserID, const TArray<FString>& RequestParams);
	virtual void OnQueryUser_Implementation(const FString& LocalUserID, const TArray<FString>& RequestParams);

	// Called to attempt user data synchronization
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	void SyncUserProfileData(const FString& LocalUserID,  const FUserProfileData& UserData);
	
	// Called to attempt user Log Upload
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	void UploadUserLog(const FString& LocalUserID,  const FUserLogs& UserLogs);
	
	// Called to attempt user analytics upload
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	void UploadUserAnalytics(const FString& LocalUserID,  const FUserAnalytics& UserAnalytics);

	// Called to attempt user data synchronization. StopAsyncRequest must be called down the line
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnTryUserDataSynchro(const FString& LocalUserID,  const FUserProfileData& UserData);
	virtual void OnTryUserDataSynchro_Implementation(const FString& LocalUserID, const FUserProfileData& UserData);

	// Called to attempt user Logs Upload. 
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnTryUserLogsUpload(const FString& LocalUserID,  const FUserLogs& UserLogs);
	virtual void OnTryUserLogsUpload_Implementation(const FString& LocalUserID, const FUserLogs& UserLogs);

	// Called to attempt user Analytics Upload
	UFUNCTION(BlueprintNativeEvent, Category="PulseCore|UserProfile")
	void OnTryUserAnalyticUpload(const FString& LocalUserID,  const FUserAnalytics& UserAnalytics);
	virtual void OnTryUserAnalyticUpload_Implementation(const FString& LocalUserID, const FUserAnalytics& UserAnalytics);

	// Get Textual Informations about a response code
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="PulseCore|UserProfile")
	FText ResponseCodeMessage(int32 ResponseCode);
	virtual FText ResponseCodeMessage_Implementation(int32 ResponseCode);
	
	// Must be called right after server request response to change status to Idle. Only successful requests will trigger events
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile", meta=(AdvancedDisplay=1, AutoCreateRefTerm="ProfileData"))
	void StopAsyncRequest(bool RequestSucceeded, FString UID, int32 ResponseCode, const FUserProfileData& ProfileData, const FString& Token = "");
	
	// Get the token for a user
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	FString GetUserToken(const FString& UserLocalID);
	
	// Get the profile data of the user currently being query or synced. 
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	bool GetOpUserProfileData(FUserProfileData& OutProfileData) const;
	
	// Set the token for a user
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	void SetUserToken(const FString& UserLocalID, const FString& Token);
	
	// Try to get a user's login parameters from its token. 
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="PulseCore|UserProfile")
	TArray<FString> TryGetQueryParamsFromToken(const FString& UserLocalID);
	TArray<FString> TryGetQueryParamsFromToken_Implementation(const FString& UserLocalID);
};


/**
 * Handle user profile.
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseUserProfile : public UEngineSubsystem, public IIPulseCore
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PulseCore|UserProfile|System")
	FString CurrentUserProfileLocalID = "";

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="PulseCore|UserProfile|Session")
	bool bIsAutoLoginActive = false;

	// Called when the user session file is saved locally 
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Session")
	FPulseTriggerEvent OnSessionSaved;

	// Called when the user session file is loaded from local storage
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Session")
	FPulseTriggerEvent OnSessionLoaded;

	// Called when a new user session file is created
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Session")
	FPulseTriggerEvent OnSessionCreation;

	// Called when the provider query (Login/Register) user operation complete
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|OnlineOps")
	FOnUserProfileUpdated OnUserQuerySucceed;

	// Called when the provider sync operation (profile data update) complete for a user
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|OnlineOps")
	FOnUserProfileUpdated OnUserSyncSucceed;

	// Called when the user provider is about to be changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FPulseTriggerEvent OnUserProviderPreChanged;

	// Called when the user provider is changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FPulseTriggerEvent OnUserProviderPostChanged;

	// Called when a new user is added
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnUserAdded;

	// Called when the current user profile changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnUserUpdated;

	// Called when the current user profile changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnUserDeleted;

	// Called when the current user profile changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnCurrentUserChanged;

	// Called whenever the system state changes
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserProfileStatusChanged OnSystemStatusChanged;
	


	bool IsPendingNewUserQuery(const FString& LocalUserID, FUserProfileData& OutUserProfileData) const;
	
	// Get the Current Status of The Subsystem;
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	EUserProfileProcessStatus GetSystemStatus() const;

	// Get the Current user profile;
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	 FUserProfile GetCurrentUserProfile(bool& OutValidUser) const;

	// Get the user profile for a specific local ID.
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	bool GetUserProfile(const FString& LocalUserID, FUserProfile& OutProfile) const;

	// Get the list of all local users;
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	bool GetUserProfilesList(TArray<FString>& OutUserLocalIDs) const;
	
	// Get the provider class object currently in use.
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	UBaseUserProfileProvider* GetUserProfileProvider() const;
	
	// Check if The user is Logged In
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	bool GetUserLoginStatus(const FString& LocalUserID, EUserProfileStatus& OutStatus) const;
	
	// Check if The current user is Logged In
	UFUNCTION(BlueprintPure, Category="PulseCore|UserProfile")
	bool GetCurrentUserLoginStatus(EUserProfileStatus& OutStatus) const;
	
	// Save the user Session
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void SaveUserProfileSession();
	
	// Load user profile from the session save.
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void LoadUserProfileSession();
	
	// Automatic login of the last user
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void SetAutoLogin(bool enable);
	
	// LogIn/Register a User (at least a localID). Request Params can be email, password, username, etc...
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void LogInOrRegisterUser(const FUserProfile& UserProfile, TArray<FString> RequestParams);
	
	// LogIn/Register the current User. Request Params can be email, password, username, etc...
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void LogInOrRegisterCurrentUser(TArray<FString> RequestParams);
	
	// Set the current user to "null"
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void LogOutCurrentUser();
	
	// Try to synchronize user data
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void SyncUserProfileDatas(const FString& LocalUserID);
	
	// Try to synchronize the current user data
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 void SyncCurrentUserProfileDatas();
	
	// Try to set this user as the current user
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 bool SetCurrentUser(const FString& LocalUserID);
	
	// Try to delete a user
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 bool DeleteUser(const FString& LocalUserID);
	
	// Add or Update a user
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	 bool AddUser(const FUserProfile& UserProfile, bool SetAsCurrent = false);
	
	// Create New User Profile
	UFUNCTION(BlueprintCallable, Category="PulseCore|UserProfile")
	bool CreateNewUserProfile(const FString& Pseudo, bool SetAsCurrent = false);
	
	static UPulseUserProfile* Get();
	
protected:

	UPROPERTY()
	TObjectPtr<UBaseUserProfileProvider> _currentProfileProvider = nullptr;

	EUserProfileProcessStatus _status = EUserProfileProcessStatus::Idle;
	
	EUserProfileProcessStatus _lastObservedStatus = EUserProfileProcessStatus::Idle;

	TMap<FString, FUserEntry> _userProfiles;

	TMap<FString, FUserProfileData> _pendingNewUserQueries;

	UFUNCTION()
	void OnSessionSavedFunc(const FString& SlotName, int userIndex, bool bSucess);
	UFUNCTION()
	void OnSessionLoadedFunc(const FString& SlotName, int userIndex, USaveGame* SessionSaveObject);
	UFUNCTION()
	void OnProviderQueryUserCompleted(const EUserProfileProcessStatus& RequestType, const FString& LocalID, const FString& UID, bool Success, int32 ResponseCode, const FUserProfileData& ProfileData);
	UFUNCTION()
	void OnProviderSyncUserCompleted(const EUserProfileProcessStatus& RequestType, const FString& LocalID, const FString& UID, bool Success, int32 ResponseCode, const FUserProfileData& ProfileData);
	UFUNCTION()
	void ObserveStatusChanges_Internal();
	
	EUserProfileProcessStatus GetUserProfileStatus_Internal() const;
	UBaseUserProfileProvider* GetUserProfileProvider_Internal() const;
	void SaveUserProfile_Internal();
	void LoadUserProfile_Internal();
	void LogInOrRegisterUser_Internal(const FUserProfile& UserProfile, const TArray<FString>& RequestParams);
	void TrySyncUser_Internal(const FString& LocalUserID) const;
	void SetCurrentProvider_Internal(TSubclassOf<UObject> NewProviderClass);
	bool ChangeUser_Internal(const FString& LocalUserID);
	bool GetUserProfile_Internal(const FString& LocalUserID, FUserProfile& OutProfile) const;
	bool GetUserProfileList_Internal(TArray<FString>& OutLocalUserIDs) const;
	bool CreateNewUserProfile_Internal(const FString& Pseudo, bool SetAsCurrent = false);
	bool AddUserProfile_Internal(const FUserProfile& UserProfile, bool SetAsCurrent = false);
	bool RemoveUserProfile_Internal(const FString& LocalUserId);
	bool GetUserLoginStatus_Internal(const FString& LocalUserID, EUserProfileStatus& OutStatus) const; 
	
public:
	
	virtual void Deinitialize() override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};


/**
 * @brief Utility object to easily handle User profile events in blueprint.
 * **/
UCLASS(NotBlueprintable, BlueprintType)
class UPulseUserProfileEventListener : public UObject
{
	GENERATED_BODY()

public:

	UPulseUserProfileEventListener();
	virtual ~UPulseUserProfileEventListener() override;

protected:

	void BindCurrentProvider();
	void UnBindCurrentProvider();
	UFUNCTION()
	void OnSessionSaved_Func();
	UFUNCTION()
	void OnSessionLoaded_Func();
	UFUNCTION()
	void OnSessionCreation_Func();
	UFUNCTION()
	void OnUserQuerySucceed_Func(const FString& LocalUserID, bool IsCurrentUser);
	UFUNCTION()
	void OnUserSyncSucceed_Func(const FString& LocalUserID, bool IsCurrentUser);
	UFUNCTION()
	void OnUserProviderPreChanged_Func();
	UFUNCTION()
	void OnUserProviderChanged_Func();
	UFUNCTION()
	void OnUserAdded_Func(const FString& LocalUID);
	UFUNCTION()
	void OnUserUpdated_Func(const FString& LocalUID);
	UFUNCTION()
	void OnUserDeleted_Func(const FString& LocalUID);
	UFUNCTION()
	void OnCurrentUserChanged_Func(const FString& LocalUID);
	UFUNCTION()
	void OnSystemStatusChanged_Func(EUserProfileProcessStatus LastStatus, EUserProfileProcessStatus NewStatus);
	UFUNCTION()
	void OnProviderAsyncRequestCompleted_Func(const EUserProfileProcessStatus& RequestType, const FString& LocalID, const FString& UID, bool Success, int32 ResponseCode, const FUserProfileData& ProfileData);

public:
	
	// Called when the user session file is saved locally 
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Session")
	FPulseTriggerEvent OnSessionSaved;

	// Called when the user session file is loaded from local storage
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Session")
	FPulseTriggerEvent OnSessionLoaded;

	// Called when a new user session file is created
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Session")
	FPulseTriggerEvent OnSessionCreation;

	// Called when the provider query (Login/Register) user operation complete
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|OnlineOps")
	FOnUserProfileUpdated OnUserQuerySucceed;

	// Called when the provider sync operation (profile data update) complete for a user
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|OnlineOps")
	FOnUserProfileUpdated OnUserSyncSucceed;

	// Called when the user provider is changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FPulseTriggerEvent OnUserProviderChanged;

	// Called when a new user is added
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnUserAdded;

	// Called when the current user profile changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnUserUpdated;

	// Called when the current user profile changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnUserDeleted;

	// Called when the current user profile changed
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserChangeEvents OnCurrentUserChanged;

	// Called whenever the system state changes
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|System")
	FOnUserProfileStatusChanged OnSystemStatusChanged;

	// Called by the user provider, whenever stop async request function is called. ignoring if the request was successful or not.
	UPROPERTY(BlueprintAssignable, Category="PulseCore|UserProfile|Provider")
	FOnSyncUserRequestCompleted OnProviderAsyncRequestCompleted;	
};
