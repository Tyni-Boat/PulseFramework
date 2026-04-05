// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LocomotionTypes.h"
#include "MoverSimulationTypes.h"
#include "Core/PulseCoreTypes.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"
#include "BasePulsePawn.generated.h"

UCLASS(Abstract, NotBlueprintable, BlueprintType)
class PULSELOCOMOTION_API ABasePulsePawn : public APawn, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABasePulsePawn();

#pragma region Balance

protected:
	// Normalized vector representing the current balance of a pawn. The character is fully balanced if the angle with actor Up direction is 0,
	// Critically balance if angle >= Critical balance and unbalanced if angle >= Balance limit.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Balance")
	FVector CurrentBalance = FVector(0, 0, 1);

	// The balance the current balance will automatically try to recover to 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Balance")
	FVector TargetBalance = FVector(0, 0, 1);

	// last frame balance state
	EPulseLocomotionBalanceState _lastBalanceState = EPulseLocomotionBalanceState::PerfectlyBalanced;

	// used for balance auto recovery.
	FQuaternionSpringState _balanceSpringState;

public:
	// Enable od disable the whole balance system. when disable the target balance cannot be set and both the current and target balance are perfectly balanced. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balance")
	bool bUseBalanceSystem = true;

	// In degree, if the Current balance vector and the actor up vector angle get pass this value, the balance is considered critical. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balance")
	float CriticalBalance = 45;

	// In degree, if the Current balance vector and the actor up vector angle get pass this value, the balance is considered completely unbalanced. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balance")
	float BalanceLimit = 60;

	// The spring (X), damping (Y) and delta speed (Z) used to try to reach a perfect balance from a mostly balanced state. Set speed to <= 0 to disable this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balance")
	FVector NormalBalanceRecovery = FVector(120, 1, 1);

	// The spring (X), damping (Y) and delta speed (Z) used to try to reach a mostly balanced state from a critical/unbalanced state. Set speed to <= 0 to disable this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Balance")
	FVector CriticalBalanceRecovery = FVector(200, 0.5, 1);

	// Triggered when the pawn balance changes. 
	UPROPERTY(BlueprintAssignable, Category="Balance")
	FPulsePawnBalanceEvent OnBalanceChanged;


	// Get the state of the current balance.
	UFUNCTION(BlueprintPure, Category="Balance")
	EPulseLocomotionBalanceState GetCurrentBalanceState() const;

	// Get the state of the target balance.
	UFUNCTION(BlueprintPure, Category="Balance")
	EPulseLocomotionBalanceState GetTargetBalanceState() const;

	// Get the current balance ration; <=0 -critical balance; 1-Perfect balance.
	UFUNCTION(BlueprintPure, Category="Balance")
	float GetBalanceRatio() const;

	// Get the velocity needed to reach perfect balance.
	UFUNCTION(BlueprintPure, Category="Balance")
	FVector GetBalanceRecoveryVelocity(const float PawnHalfHeight) const;

	/**
	 * @brief Set the target balance the current will try to follow.
	 * @param WorldDirection In-world direction used to determiner the direction where to shift the target balance.
	 * @param AngleDegree The angle in degrees of how much to shift from the perfect balance (up vector)
	 * @param Operation How to affect the target balance. Note that multiplication, division and modulo doesn't modify the direction.
	 **/
	UFUNCTION(BlueprintCallable, Category="Balance")
	void SetTargetBalance(const FVector& WorldDirection, const float AngleDegree, const ENumericOperator Operation = ENumericOperator::Set);

	// Draw debug arrow for the balance
	UFUNCTION(BlueprintCallable, Category="Balance", meta=(AutoCreateRefTerm = "RelativeLocation"))
	void DrawDebugBalance(const FVector& RelativeLocation, const float ArrowLenght = 100, const float ArrowSize = 50, const float Thickness = 2,
	                      FLinearColor PerfectColor = FLinearColor(0,0.5,1,1), FLinearColor NormaColor = FLinearColor(0,1,0.5,1), FLinearColor CriticalColor = FLinearColor(1,0.6,0,1),
	                      FLinearColor UnbalanceColor = FLinearColor(1,0.2,0,1));

protected:
	// Tick the balance logic.
	void BalanceLogicTick(const float DeltaTime);

#pragma endregion

#pragma region Limb

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Limb")
	TArray<FPulseLimbDefinition> Limbs = {};
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Limb")
	TObjectPtr<USkeletalMeshComponent> DebugSKM = nullptr;

	void DebugLimbs(const float DeltaTime);

#pragma endregion 

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mover")
	FVector LocomotionInputVector = FVector(0);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// used by the mover component to handle inputs
	virtual void ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult) override;
};
