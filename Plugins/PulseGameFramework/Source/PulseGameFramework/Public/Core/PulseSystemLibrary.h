// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include <rapidjson/document.h>

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/CoreConcepts.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Kismet/GameplayStatics.h"
#include "GameplayTagsManager.h"
#include "GameplayTagsModule.h"
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
	UFUNCTION(BlueprintPure, Category="PulseTool|Camera", meta=(WorldContext = "WorldContext", DisplayName="CameraRelativeInput", AdvancedDisplay = 3))
	static bool TryGetCameraRelativeInput(const UObject* WorldContext, const FVector2D& Input, FVector& OutDirection, const int32 PlayerIndex = 0,
	                                      const FVector& Normal = FVector(0), const float SnapToAngle = 0);

	// Rotate a component around a desired YawAxis by Input.X and around an orthogonal Tilt Axis by Input.Y limited by TiltLimits
	UFUNCTION(BlueprintCallable, Category="PulseTool|Camera", meta=(DisplayName="RotateByInput", AdvancedDisplay = 2, AutoCreateRefTerm = "SweepResult"))
	static void RotateComponentByInputs(USceneComponent*& Component, const FVector2D Inputs, FHitResult& SweepResult, const FVector& YawAxis = FVector(0, 0, 1),
	                                    const FVector2D& TiltLimits = FVector2D(-45, 35), bool bInvertX = false, bool bInvertY = true, bool bUseSweep = false,
	                                    ETeleportType TeleportType = ETeleportType::None);


	// Simulate a key press.
	UFUNCTION(BlueprintCallable, Category="PulseTool|Inputs")
	static void SimulateKey(const FKey Key, EInputEvent Event);


	// Return the time in the montage where an anim notify of the defined class will be triggerred for the first time.  
	UFUNCTION(BlueprintCallable, Category="PulseTool|Animation", meta=(DisplayName = "FirstNotifyTrigger"))
	static float GetMontageFirstNotifyTriggerTime(const UAnimMontage* montage, TSubclassOf<UAnimNotify> notifyClass);


	// Return the time in the montage where an anim notify State of the defined class will be Started(X), How long it will be(Y) and When will it finnish (Z) for the first time.  
	UFUNCTION(BlueprintCallable, Category="PulseTool|Animation", meta=(DisplayName = "FirstNotifyState"))
	static FVector GetMontageFirstNotifyStateTime(const UAnimMontage* montage, TSubclassOf<UAnimNotifyState> notifyClass);


	// Get the start time (X) and the desired playback speed (Y) for a montage to try to have the Location reach MatchPoint at the AnimationMatchTime.
	UFUNCTION(BlueprintPure, Category = "PulseTool|Animation", meta=(DisplayName = "MontageMatchStart"))
	static FVector2D GetMontageStartTimeFromSpeed(const FVector& Location, const FVector& Velocity, const FVector& MatchPoint, const float AnimationMatchTime);


	// Return true if tag container contains this tag or one of it's child tags
	UFUNCTION(BlueprintPure, Category = "PulseTool|GameplayTags")
	static bool HasChildTag(const FGameplayTagContainer& Container, FGameplayTag Tag);

	// Remove this and all child tags from the container
	UFUNCTION(BlueprintCallable, Category = "PulseTool|GameplayTags")
	static bool RemoveChildTags(UPARAM(ref)
	                            FGameplayTagContainer& Container, FGameplayTag Tag);

	// Add this and all child tags from the container
	UFUNCTION(BlueprintCallable, Category = "PulseTool|GameplayTags")
	static bool AddChildTags(UPARAM(ref)
	                         FGameplayTagContainer& Container, FGameplayTag Tag);




	// Enable or disable an actor without destroying it
	UFUNCTION(BlueprintCallable, Category = "PulseTool|Actors")
	static bool EnableActor(AActor* Actor, bool Enable);

	// Check if an actor is enabled
	UFUNCTION(BlueprintPure, Category = "PulseTool|Actors")
	static bool IsActorEnabled(const AActor* Actor);

	// Add a component to an actor at runtime. Returns true if the component was added successfully.
	UFUNCTION(BlueprintCallable, Category = "PulseTool|Actors")
	static bool AddComponentAtRuntime(AActor* Actor, UActorComponent* Component);

	// Extract component at runtime. Returns true if the component was removed successfully.
	UFUNCTION(BlueprintCallable, Category = "PulseTool|Actors")
	static bool RemoveComponentAtRuntime(AActor* Actor, UActorComponent* Component);


	// Serialize an object to byte array
	UFUNCTION(BlueprintCallable, Category = "PulseTool|Serialization")
	static bool SerializeObjectToBytes(UObject* object, TArray<uint8>& outBytes);

	// Serialize an object to byte array
	UFUNCTION(BlueprintCallable, Category = "PulseTool|Serialization")
	static bool DeserializeObjectFromBytes(const TArray<uint8>& bytes, UObject* OutObject);

public:
	// Get A Pulse Module
	template <pulseCore::concepts::IsModule T>
	static T* GetPulseModule(const UObject* WorldContext)
	{
		return GetPulseModule<T>(WorldContext, T::StaticClass());
	}

	template <pulseCore::concepts::IsModule T, typename Q>
		requires pulseCore::concepts::IsSubclassOf<T, Q>
	static T* GetPulseModule(const UObject* WorldContext, Q Class)
	{
		const auto gameInstance = Cast<UGameInstance>(WorldContext) ? Cast<UGameInstance>(WorldContext) : UGameplayStatics::GetGameInstance(WorldContext);
		if (!gameInstance)
			return nullptr;
		T* pModule = gameInstance->GetSubsystem<T>();
		return pModule;
	}

	// Get A pulse Sub-Module
	template <pulseCore::concepts::IsModule T, pulseCore::concepts::IsSubModule Q>
	static Q* GetPulseSubModule(const UObject* WorldContext)
	{
		return GetPulseSubModule<T, Q>(WorldContext, T::StaticClass(), Q::StaticClass());
	}

	template <pulseCore::concepts::IsModule T, pulseCore::concepts::IsSubModule Q, typename U, typename V>
		requires pulseCore::concepts::AreSubclassesOf<T, U, Q, V>
	static Q* GetPulseSubModule(const UObject* WorldContext, U ModuleClass, V SubModuleClass)
	{
		T* pModule = GetPulseModule<T>(WorldContext, ModuleClass);
		if (!pModule)
			return nullptr;
		Q* pSubModule = pModule->GetSubModule<Q>(SubModuleClass);
		return pSubModule;
	}

	template <typename T>
	static bool SetAtIndexWhileAdding(TArray<T>& Collection, const int32 Index, const T& Value, T DefaultValue = T())
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
	static bool MatchCollectionSize(TArray<T>& Collection, const int32 Size, bool bCanReduceArraySize = true, T FillWith = T())
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

	template <pulseCore::concepts::IsNumber T>
	static T SumCollection(const TArray<T>& Collection)
	{
		T result = {};
		for (int32 i = 0; i < Collection.Num(); ++i)
		{
			result += Collection[i];
		}
		return result;
	}
	template <typename T, pulseCore::concepts::IsNumber Q>
	static Q SumCollectionBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
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

	template <pulseCore::concepts::IsNumber T>
	static T AverageCollection(const TArray<T>& Collection)
	{
		if (Collection.Num() <= 0)
			return T();
		T result = SumCollection(Collection);
		result /= Collection.Num();
		return result;
	}
	template <typename T, pulseCore::concepts::IsNumber Q>
	static Q AverageCollectionBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
	{
		if (Collection.Num() <= 0)
			return T();
		Q result = SumCollectionBy(Collection, Delegate);
		result /= Collection.Num();
		return result;
	}

	template <pulseCore::concepts::IsNumber T>
	static T MaxInCollection(const TArray<T>& Collection)
	{
		T result = TNumericLimits<T>::Lowest();
		for (int32 i = 0; i < Collection.Num(); i++)
		{
			if (Collection[i] >= result)
				result = Collection[i];
		}
		return result;
	}
	template <typename T, pulseCore::concepts::IsNumber Q>
	static Q MaxInCollectionBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
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

	template <pulseCore::concepts::IsNumber T>
	static T MinInCollection(const TArray<T>& Collection)
	{
		T result = TNumericLimits<T>::Max();
		for (int32 i = 0; i < Collection.Num(); i++)
		{
			if (Collection[i] <= result)
				result = Collection[i];
		}
		return result;
	}
	template <typename T, pulseCore::concepts::IsNumber Q>
	static Q MinInCollectionBy(const TArray<T>& Collection, std::function<Q(const T&)> Delegate)
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
	static bool CompareCollectionsElements(const TArray<T>& CollectionA, const TArray<T>& CollectionB)
	{
		if (CollectionA.Num() != CollectionB.Num())
			return false;
		for (int32 i = 0; i < CollectionA.Num(); ++i)
			if (CollectionA[i] != CollectionB[i])
				return false;
		return true;
	}

	template <typename T>
	static T* TryGetItemAt(TArray<T>& Collection, const int32 Index)
	{
		if (Collection.IsValidIndex(Index))
			return &Collection[Index];
		return nullptr;
	}

	template <typename T, typename Q>
	static bool AddOrReplace(TMap<T, Q>& Map, const T& Key, const Q& NewValue)
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
