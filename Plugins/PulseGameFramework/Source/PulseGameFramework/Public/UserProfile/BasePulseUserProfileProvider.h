// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseUserProfileTypes.h"
#include "UObject/Object.h"
#include "BasePulseUserProfileProvider.generated.h"

/**
 * Base class for any pulse user profile provider.
 */
UCLASS(Abstract, Blueprintable)
class PULSEGAMEFRAMEWORK_API UBasePulseUserProfileProvider : public UObject
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
