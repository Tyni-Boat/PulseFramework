// Copyright © by Tyni Boat. All Rights Reserved.


#include "UserProfile/PulseUserProfileEventListener.h"

#include "UserProfile/PulseUserProfile.h"


// Sets default values for this component's properties
UPulseUserProfileEventListener::UPulseUserProfileEventListener()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPulseUserProfileEventListener::PostInitProperties()
{
	Super::PostInitProperties();
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

void UPulseUserProfileEventListener::BeginDestroy()
{
	Super::BeginDestroy();
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


