// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include <functional>
#include <rapidjson/document.h>

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/CoreConcepts.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Core/PulseCoreTypes.h"
#include "GameplayTagsModule.h"
#include "JsonObjectConverter.h"
#include "StructUtils/InstancedStruct.h"
#include "PulseSystemLibrary.generated.h"

/**
 * Library of tools to Make some actions.
 */
UCLASS()
class PULSEGAMEFRAMEWORK_API UPulseSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Get the vector CameraForward * Input.Y + CameraRight * Input.X. Snap to angle must not exceed 90 and normal must not be aligned with neither camera right nor forward vectors.
	UFUNCTION(BlueprintPure, Category="PulseCore|Tools|Camera", meta=(WorldContext = "WorldContext", DisplayName="CameraRelativeInput", AdvancedDisplay = 3))
	static bool TryGetCameraRelativeInput(const UObject* WorldContext, const FVector2D& Input, FVector& OutDirection, const int32 PlayerIndex = 0,
	                                      const FVector& Normal = FVector(0), const float SnapToAngle = 0);

	// Rotate a component around a desired YawAxis by Input.X and around an orthogonal Tilt Axis by Input.Y limited by TiltLimits
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tools|Camera", meta=(DisplayName="RotateByInput", AdvancedDisplay = 2, AutoCreateRefTerm = "SweepResult"))
	static void RotateComponentByInputs(USceneComponent*& Component, const FVector2D Inputs, FHitResult& SweepResult, const FVector& YawAxis = FVector(0, 0, 1),
	                                    const FVector2D& TiltLimits = FVector2D(-45, 35), bool bInvertX = false, bool bInvertY = true, bool bUseSweep = false,
	                                    ETeleportType TeleportType = ETeleportType::None);


	// Simulate a key press.
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tools|Inputs")
	static void SimulateKey(const FKey Key, EInputEvent Event);


	// Return the time in the montage where an anim notify of the defined class will be triggerred for the first time.  
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tools|Animation", meta=(DisplayName = "FirstNotifyTrigger"))
	static float GetMontageFirstNotifyTriggerTime(const UAnimMontage* montage, TSubclassOf<UAnimNotify> notifyClass);


	// Return the time in the montage where an anim notify State of the defined class will be Started(X), How long it will be(Y) and When will it finnish (Z) for the first time.  
	UFUNCTION(BlueprintCallable, Category="PulseCore|Tools|Animation", meta=(DisplayName = "FirstNotifyState"))
	static FVector GetMontageFirstNotifyStateTime(const UAnimMontage* montage, TSubclassOf<UAnimNotifyState> notifyClass);


	// Get the start time (X) and the desired playback speed (Y) for a montage to try to have the Location reach MatchPoint at the AnimationMatchTime.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Tools|Animation", meta=(DisplayName = "MontageMatchStart"))
	static FVector2D GetMontageStartTimeFromSpeed(const FVector& Location, const FVector& Velocity, const FVector& MatchPoint, const float AnimationMatchTime);


	// Return true if tag container contains this tag or one of it's child tags
	UFUNCTION(BlueprintPure, Category = "PulseCore|Tools|GameplayTags")
	static bool HasChildTag(const FGameplayTagContainer& Container, FGameplayTag Tag);

	// Remove this and all child tags from the container
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|GameplayTags")
	static bool RemoveChildTags(UPARAM(ref)
	                            FGameplayTagContainer& Container, FGameplayTag Tag);

	// Add this and all child tags from the container
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|GameplayTags")
	static bool AddChildTags(UPARAM(ref)
	                         FGameplayTagContainer& Container, FGameplayTag Tag);

	// Make A FName Tag by concat names separated by "."
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|GameplayTags")
	static FName ConstructNametag(TArray<FName> TagParts, const FString& Separator = ".");

	// Try to extract tag parts separated by "."
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|GameplayTags")
	static bool ExtractNametagParts(FName Tag, TArray<FName>& OutTagParts, const FString& Separator = ".");


	// Enable or disable an actor without destroying it
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Actors")
	static bool EnableActor(AActor* Actor, bool Enable);

	// Check if an actor is enabled
	UFUNCTION(BlueprintPure, Category = "PulseCore|Tools|Actors")
	static bool IsActorEnabled(const AActor* Actor);

	// Add a component to an actor at runtime. Returns true if the component was added successfully.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Actors")
	static bool AddComponentAtRuntime(AActor* Actor, UActorComponent* Component, bool bReplicate = false);

	// Extract component at runtime. Returns true if the component was removed successfully.
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Actors")
	static bool RemoveComponentAtRuntime(AActor* Actor, UActorComponent* Component);

	// Execute the delegate for every actor of class, that match condition; found in the world.
	static void ForeachActorClass(const UObject* WorldContext,  TSubclassOf<AActor> Class, TFunction<void(AActor*)> Action, TFunction<bool(AActor*)> Condition = nullptr);
	
	// Execute the delegate for every actor of class, that match condition; found in the world.
	static void ForeachActorInterface(const UObject* WorldContext,  TSubclassOf<UInterface> Interface, TFunction<void(AActor*)> Action, TFunction<bool(AActor*)> Condition = nullptr);


	// Serialize an object to byte array
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool SerializeObjectToBytes(UObject* object, TArray<uint8>& outBytes);

	// Serialize an object to byte array
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool DeserializeObjectFromBytes(const TArray<uint8>& bytes, UObject* OutObject);

	// Serialize an structure to Json string
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool SerializeStructToJson(const FInstancedStruct& StructData, FString& OutJson);

	// Deserialize an struct from json string
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool DeserializeStructFromJson(const FString& JsonString, FInstancedStruct& OutStruct, UScriptStruct* StructType);


	// Will be save to Saved/Data
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool SaveJsonToLocal(const FString& FileName, const FString& JsonString);

	// Will be load from Saved/Data
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool LoadJsonFromLocal(const FString& FileName, FString& OutJsonString);

	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool LoadJsonFromLocalPath(const FString& FilePath, FString& OutJsonString);

	// Check if an asset type derives from class
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|Serialization")
	static bool IsAssetTypeDerivedFrom(FPrimaryAssetType AssetType, TSubclassOf<UObject> ClassToCheck);


	// Check if a file exist
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|File Management")
	static bool FileExist(const FString& FilePath);

	// Async get file md5
	static void FileComputeMD5Async(const FString& FilePath, FOnMD5Computed OnComplete, int32 BufferSize = 1048576);

	// Try to delete a file
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|File Management")
	static bool FileDelete(const FString& FilePath);

	// Determine if a path is writable
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|File Management")
	static bool FileIsPathWritable(const FString& DirectoryPath);

	// Determine if a path is writable
	UFUNCTION(BlueprintCallable, Category = "PulseCore|Tools|File Management")
	static void FileGetAllFilesInDirectory(const FString& Directory, TArray<FString>& OutFiles, bool bRecursive = false, const FString& Extension = TEXT(""));

	// Get the byte size to string.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Tools|File Management")
	static FString FileSizeToString(const int64& ByteSize);


	// Get the byte size to string.
	UFUNCTION(BlueprintPure, Category = "PulseCore|Tools|Text")
	static bool TextRegex(const FString& InText, const FString& Regex);

public:
	template <pulseCore::concepts::IsUStruct T>
	static FString UStructToJsonString(const T& StructData)
	{
		FString OutJson;
		FJsonObjectConverter::UStructToJsonObjectString<T>(
			StructData,
			OutJson,
			0, 0
		);

		return OutJson;
	}

	template <pulseCore::concepts::IsUStruct T>
	static bool JsonStringToUStruct(const FString& JsonString, T& OutStruct)
	{
		return FJsonObjectConverter::JsonObjectStringToUStruct<T>(
			JsonString,
			&OutStruct,
			0, 0
		);
	}

	template <typename T>
	static void ArrayCopyIntoFirst(TArray<T>& CollectionA, const TArray<T>& CollectionB)
	{
		for (int32 i = 0; i < CollectionB.Num(); ++i)
		{
			CollectionA.Add(CollectionB[i]);
		}
	}

	template <typename T>
	static bool ArraySetAtIndexWhileAdding(TArray<T>& Collection, const int32 Index, const T& Value, T DefaultValue = T())
	{
		if (Index < 0)
			return false;
		if (Collection.IsValidIndex(Index))
		{
			Collection[Index] = Value;
			return true;
		}
		for (int32 i = Collection.Num(); i <= Index; ++i)
		{
			if (i == Index)
			{
				Collection.Add(Value);
				break;
			}
			Collection.Add(DefaultValue);
		}
		return true;
	}

	template <typename T>
	static bool ArrayMatchSize(TArray<T>& Collection, const int32 Size, bool bCanReduceArraySize = true, T FillWith = T())
	{
		if (Size < 0)
			return false;
		if (Collection.Num() >= Size)
		{
			if (Collection.Num() == Size)
				return true;
			if (!bCanReduceArraySize)
				return true;
			for (int i = Collection.Num() - 1; i >= Size; --i)
				Collection.RemoveAt(i);
			return true;
		}
		for (int32 i = Collection.Num(); i < Size; ++i)
		{
			Collection.Add(FillWith);
		}
		return true;
	}

	template <typename T>
	static T ArraySum(const TArray<T>& Collection)
	{
		T result = {};
		for (int32 i = 0; i < Collection.Num(); ++i)
		{
			result += Collection[i];
		}
		return result;
	}

	template <typename T, typename Q>
	static Q ArraySumBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
	{
		Q result = {};
		if (!Delegate)
			return result;
		for (int32 i = 0; i < Collection.Num(); ++i)
		{
			result += Delegate(Collection[i]);
		}
		return result;
	}

	template <typename T>
	static T ArrayAverage(const TArray<T>& Collection)
	{
		if (Collection.Num() <= 0)
			return T();
		T result = ArraySum(Collection);
		result /= Collection.Num();
		return result;
	}

	template <typename T, typename Q>
	static Q ArrayAverageBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
	{
		if (Collection.Num() <= 0)
			return T();
		Q result = ArraySumBy(Collection, Delegate);
		result /= Collection.Num();
		return result;
	}

	template <typename T>
	static T ArrayMax(const TArray<T>& Collection)
	{
		T result = TNumericLimits<T>::Lowest();
		for (int32 i = 0; i < Collection.Num(); i++)
		{
			if (Collection[i] >= result)
				result = Collection[i];
		}
		return result;
	}

	template <typename T, typename Q>
	static Q ArrayMaxBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
	{
		Q result = TNumericLimits<Q>::Lowest();
		if (!Delegate)
			return result;
		for (int32 i = 0; i < Collection.Num(); i++)
		{
			Q temp = Delegate(Collection[i]);
			if (temp >= result)
				result = temp;
		}
		return result;
	}

	template <typename T>
	static T ArrayMin(const TArray<T>& Collection)
	{
		T result = TNumericLimits<T>::Max();
		for (int32 i = 0; i < Collection.Num(); i++)
		{
			if (Collection[i] <= result)
				result = Collection[i];
		}
		return result;
	}

	template <typename T, typename Q>
	static Q ArrayMinBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
	{
		Q result = TNumericLimits<Q>::Max();
		if (!Delegate)
			return result;
		for (int32 i = 0; i < Collection.Num(); i++)
		{
			Q temp = Delegate(Collection[i]);
			if (temp <= result)
				result = temp;
		}
		return result;
	}

	template <typename T>
	static bool ArrayCompareElements(const TArray<T>& CollectionA, const TArray<T>& CollectionB)
	{
		if (CollectionA.Num() != CollectionB.Num())
			return false;
		for (int32 i = 0; i < CollectionA.Num(); ++i)
			if (CollectionA[i] != CollectionB[i])
				return false;
		return true;
	}

	template <typename T>
	static T* ArrayTryGetItemAt(TArray<T>& Collection, const int32 Index)
	{
		if (Collection.IsValidIndex(Index))
			return &Collection[Index];
		return nullptr;
	}

	template <typename T>
	static float ArrayIndexPercentage(TArray<T>& Collection, const int32 Index, bool IncludeIndexUnitPercentage = true)
	{
		if (!Collection.IsValidIndex(Index))
			return -1;
		float unit = 1.0f / Collection.Num();
		return unit * (Index + (IncludeIndexUnitPercentage ? 1 : 0));
	}

	template <typename T, typename Q>
	static bool MapAddOrUpdateValue(TMap<T, Q>& Map, const T& Key, const Q& NewValue)
	{
		if (Map.Contains(Key))
		{
			Map[Key] = NewValue;
			return true;
		}
		else
		{
			Map.Add(Key, NewValue);
			return true;
		}
	}
};
