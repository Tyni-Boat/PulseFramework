// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PulseUserProfileTypes.h"
#include "Components/ActorComponent.h"
#include "PulseUserProfileEventListener.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PULSEGAMEFRAMEWORK_API UPulseUserProfileEventListener : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UPulseUserProfileEventListener();
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

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
