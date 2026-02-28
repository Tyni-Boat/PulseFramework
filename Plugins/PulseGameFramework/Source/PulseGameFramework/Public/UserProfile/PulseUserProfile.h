// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseUserProfileTypes.h"
#include "BasePulseUserProfileProvider.h"
#include "PulseUserProfile.generated.h"



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
	UBasePulseUserProfileProvider* GetUserProfileProvider() const;
	
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
	TObjectPtr<UBasePulseUserProfileProvider> _currentProfileProvider = nullptr;

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
	UBasePulseUserProfileProvider* GetUserProfileProvider_Internal() const;
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
