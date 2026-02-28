// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/PulseCoreTypes.h"
#include "GameFramework/SaveGame.h"
#include "PulseUserProfileTypes.generated.h"


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


DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(FOnSyncUserRequestCompleted, const EUserProfileProcessStatus&, RequestType, const FString&, LocalID, const FString&, UID, bool, Success, int32, ResponseCode, const FUserProfileData&, ProfileData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUserProfileStatusChanged, EUserProfileProcessStatus, LastStatus, EUserProfileProcessStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUserProfileUpdated, const FString&, LocalUserID, bool, IsCurrentUser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserChangeEvents, const FString&, LocalUID);
#define SESSION_SAVE_SLOT "UserSessionSave"