// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PulseCoreTypes.generated.h"


#pragma region Enums

UENUM(BlueprintType)
enum class ELogicComparator : uint8
{
	Equal = 0 UMETA(DisplayName = "=="),
	NotEqual = 1 UMETA(DisplayName = "!="),
	GreaterThan = 2 UMETA(DisplayName = ">"),
	LessThan = 3 UMETA(DisplayName = "<"),
	GreaterThanOrEqualTo = 4 UMETA(DisplayName = ">="),
	LessThanOrEqualTo = 5 UMETA(DisplayName = "<=")
};

UENUM(BlueprintType)
enum class EBooleanOperator : uint8
{
	Not = 0 UMETA(DisplayName = "Invert logic"),
	Or = 1 UMETA(DisplayName = "One or the other"),
	And = 2 UMETA(DisplayName = "One and the other"),
	Xor = 3 UMETA(DisplayName = "Either one or the other"),
	Nor = 4 UMETA(DisplayName = "Inverted Or"),
	Nand = 5 UMETA(DisplayName = "Inverted And"),
	NXor = 6 UMETA(DisplayName = "Inverted Xor")
};


UENUM(BlueprintType)
enum class ENumericOperator : uint8
{
	Set = 0 UMETA(DisplayName = "->"),
	Add = 1 UMETA(DisplayName = "+"),
	Sub = 2 UMETA(DisplayName = "-"),
	Mul = 3 UMETA(DisplayName = "X"),
	Div = 4 UMETA(DisplayName = "/"),
	Mod = 5 UMETA(DisplayName = "%"),
};

// Enum of the type of user, from the last synchro
UENUM(BlueprintType)
enum class EUserProfileStatus : uint8
{
	None = 0 UMETA(DisplayName = "Invalid"),
	Guest = 1 UMETA(DisplayName = "Guest User"),
	LoggedIn = 2 UMETA(DisplayName = "LoggedIn user")
};


#pragma endregion Enums


#pragma region Structs


// store the user's game parameters such as graphic quality, resolution, sound levels, etc...
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUserGameConfig
{
	GENERATED_BODY()

public:
	FUserGameConfig(){}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameConfig")
	TMap<FName, float> GraphicParams;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameConfig")
	TMap<FName, float> AudioParams;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameConfig")
	TMap<FName, float> ControlsParams;
};

// track user behaviour in the game
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUserAnalytics
{
	GENERATED_BODY()

public:
	FUserAnalytics(){}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserAnalytics")
	double lastServerLoginTimeStamp = 0.0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserAnalytics")
	float lastSessionDuration = 0.0f;
};

// log important user activity in the game.
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUserLogs
{
	GENERATED_BODY()

public:
	FUserLogs(){}

	inline void Log(const int32 Verbosity, const FName Category, FString* LogMessage)
	{
		const auto finalMsg = FString::Printf(TEXT("[%s][%d][%s]-%p"),
			*FDateTime::Now().ToString(),
			Verbosity,
			*Category.ToString(),
			LogMessage);
		Logs.Add(finalMsg);
		if (Logs.Num() > MaxLogSize)
			Logs.RemoveAt(0);
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserLogs")
	TArray<FString> Logs;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserLogs")
	int32 MaxLogSize = 500;
};

// store data of the user. The server always have authority on it.
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUserProfileData
{
	GENERATED_BODY()

public:
	FUserProfileData(){}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FUserGameConfig GameConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FString Pseudo;

	// Can be used for predefined avatar icon, banners, badges, etc...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	TArray<int32> ProfileCosmeticIndexes;
};

// The profile of a user in the game
USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FUserProfile
{
	GENERATED_BODY()

public:
	FUserProfile(){}
	
	FUserProfile(FGuid GuestGUID){ LocalID = GuestGUID.ToString(); }
	
	FUserProfile(const FString& GuestGUID){ LocalID = GuestGUID; }
	
	inline bool IsUserValid() const { auto blUID = !UserUID.IsEmpty()? UserUID : LocalID; blUID.RemoveSpacesInline(); return !blUID.IsEmpty(); }
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FString UserUID = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FString LocalID = "";
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FUserProfileData ProfileData = {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FUserAnalytics Analytics = {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserProfileData")
	FUserLogs Logs = {};
};

USTRUCT(BlueprintType)
struct PULSEGAMEFRAMEWORK_API FCodedOperation
{
	GENERATED_BODY()

public:
	FCodedOperation();

	virtual ~FCodedOperation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coded Operation")
	ELogicComparator Comparator = ELogicComparator::Equal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coded Operation")
	float AValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Coded Operation")
	float BValue = 0;

	bool Evaluate() const;
};

#pragma endregion Structs

#pragma region Macros

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPulseTriggerEvent);
DECLARE_MULTICAST_DELEGATE(FPulseTriggerEventRaw);
DECLARE_DELEGATE_TwoParams(FOnMD5Computed, bool /*bSuccess*/, const FString& /*MD5*/);

#pragma endregion Macros

#pragma region Classes

UCLASS(Abstract)
class PULSEGAMEFRAMEWORK_API UBaseGameCondition : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Condition")
	FCodedOperation ConditionParam;

	UFUNCTION(BlueprintPure, Category = "Condition")
	virtual bool EvaluateCondition(const int32 Code, bool bInvalidCodeFallbackResponse = false) const;
};


UCLASS(config = Game, defaultconfig, Category = Settings, meta = (DisplayName = "Pulse Core Config"))
class UCoreProjectSetting : public UDeveloperSettings
{
	GENERATED_BODY()

public:
#pragma region User Profile

	UPROPERTY(EditAnywhere, Config, Category = "User Profile", meta=(AllowedClasses = "/Script/PulseGameFramework.BasePulseUserProfileProvider"))
	TSubclassOf<UObject> UserProfileProviderClass;

#pragma endregion

#pragma region Pooling System

	UPROPERTY(EditAnywhere, Config, Category = "Pooling System")
	int32 GlobalPoolLimit = 100;

#pragma endregion

#pragma region Tweening

	UPROPERTY(EditAnywhere, Config, Category = "Tweening")
	bool bUseMultiThreadedTween = true;
	
	UPROPERTY(EditAnywhere, Config, Category = "Tweening", meta=(ClampMin = 1, UIMin = 1))
	int32 TweenCountToMultiThreadProcessing = 1000;

#pragma endregion

#pragma region Downmloading

	UPROPERTY(EditAnywhere, Config, Category = "Downloader")
	int32 MaxConcurrentDownloads = 3;
	
	UPROPERTY(EditAnywhere, Config, Category = "Downloader")
	int32 DownloadChunkMBSize = 100;
	
	UPROPERTY(EditAnywhere, Config, Category = "Downloader")
	int32 DownloadChunkRetries = 3;
	
	UPROPERTY(EditAnywhere, Config, Category = "Downloader")
	int32 MaxConcurrentChunks = 3;
	
	UPROPERTY(EditAnywhere, Config, Category = "Downloader")
	int32 FileInfosQueryTimeOutSeconds = 5;

#pragma endregion

#pragma region Save System

	// Verify the save cache upon reading. Slower but can prevent one to read an object that'd been modified by a previous reader.
	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	bool bUseSaveCacheInvalidationOnRead = true;

	// Verify the hash of a save data uon loading.
	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	bool bUseLoadHashVerification = true;

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	int32 LocalSaveSlotCount = 5;

	UPROPERTY(EditAnywhere, Config, Category = "Save System")
	int32 LocalPerSlotBufferCount = 3;

	UPROPERTY(EditAnywhere, Config, Category = "Save System", meta=(AllowedClasses = "/Script/PulseGameFramework.SaveMetaProcessor", AllowAbstract = false))
	TSubclassOf<UObject> SaveMetaProcessorClass;

	UPROPERTY(EditAnywhere, Config, Category = "Save System", meta=(AllowedClasses = "/Script/PulseGameFramework.LocalGameSaveProvider", AllowAbstract = false))
	TSubclassOf<UObject> LocalSaveProviderClass;

	UPROPERTY(EditAnywhere, Config, Category = "Save System", meta=(AllowedClasses = "/Script/PulseGameFramework.GameSaveProvider", DisallowedClasses = "/Script/PulseGameFramework.LocalGameSaveProvider", AllowAbstract = false))
	TArray<TSubclassOf<UObject>> SaveProviderClasses;

#pragma endregion

#pragma region Pulse Asset Management

	// To Use the Pulse Asset Manager, Copy this and Paste to DefaultEngine.ini, at [/Script/Engine.Engine] Section
	UPROPERTY(VisibleAnywhere, Config, Category = "Asset Management")
	FString UsePulseAssetManager = "/Script/PulseGameFramework.PulseAssetManager";
	
	// The list of the additional asset content provider. usually CDNs
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management", meta=(AllowedClasses = "/Script/PulseGameFramework.PulseContentProvider", AllowAbstract = false))
	TArray<TSubclassOf<UObject>> AdditionalContentProviders;
	
	// The Main Directory Where newly created Pulse Assets will be created and Query from at Editor Time.
	// Useful when using with an editor window to manage assets.
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management")
	FDirectoryPath MainGameAssetSavePath;

	// Must be filled In Editor. Is used to load initial assets manifest.
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management")
	FSoftObjectPath LocalAssetManifest;

	// Do the asset manager have to construct the manifest himself if no local manifest found?
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management")
	bool bConstructManifest = false;

	// The size of the buffer (in MB) when verifying downloaded bundle hash
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management")
	float Md5CheckBufferSize = 1;

	// The current manifest type version
	UPROPERTY(EditAnywhere, Config, Category = "Asset Management")
	int32 ManifestTypeVersion = 0;
	
#pragma endregion

#pragma region Buffing System

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)")
	bool bSaveAndLoadActiveBuffs = true;

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)")
	bool bReplicateActiveBuffs = false;

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)")
	int32 BuffHandlePoolingLimit = 1000;

	UPROPERTY(EditAnywhere, Config, Category = "Operation Modifiers (Buff)", meta=(ToolTip = "Veru experimental feature. use with caution"))
	bool bAutoApplyGameplayEffects = false;
	
#pragma endregion

#pragma region Network Manager

	UPROPERTY(EditAnywhere, Config, Category = "Network Manager|Replication")
	bool bNetworkManagerAlwaysRelevant = true;
	
	UPROPERTY(EditAnywhere, Config, Category = "Network Manager|Replication")
	float NetUpdateFrequency = 100.0f;
	
	UPROPERTY(EditAnywhere, Config, Category = "Network Manager|Replication")
	float NetPriority = 2.8f;
	
#pragma endregion

#pragma region Inventory Manager

	UPROPERTY(EditAnywhere, Config, Category = "Pulse Inventory")
	bool bReplicateInventory = false;
	
	UPROPERTY(EditAnywhere, Config, Category = "Pulse Inventory")
	bool bSaveInventory = false;
	
#pragma endregion
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIPulseCore : public UInterface
{
	GENERATED_BODY()
};

class IIPulseCore
{
	GENERATED_BODY()
public:

	bool GetCurrentUserProfile(FUserProfile& OutProfile) const;

	UCoreProjectSetting* GetProjectSettings() const;
};

#pragma endregion Classes


#pragma region Delegates

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerJoinGame, APlayerController*, NewPlayer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeaveGame, APlayerController*, Player);

#pragma endregion Delegates