// Copyright © by Tyni Boat. All Rights Reserved.


#include "Characters/BaseMoverPulsePawn.h"



// Sets default values
ABaseMoverPulsePawn::ABaseMoverPulsePawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}


// Called when the game starts or when spawned
void ABaseMoverPulsePawn::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABaseMoverPulsePawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ABaseMoverPulsePawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ABaseMoverPulsePawn::ProduceInput_Implementation(int32 SimTimeMs, FMoverInputCmdContext& InputCmdResult)
{
	IMoverInputProducerInterface::ProduceInput_Implementation(SimTimeMs, InputCmdResult);
	FCharacterDefaultInputs& CharacterInputs = InputCmdResult.InputCollection.FindOrAddMutableDataByType<FCharacterDefaultInputs>();

	if (GetController() == nullptr)
	{
		if (GetLocalRole() == ENetRole::ROLE_Authority && GetRemoteRole() == ENetRole::ROLE_SimulatedProxy)
		{
			static const FCharacterDefaultInputs DoNothingInput;
			// If we get here, that means this pawn is not currently possessed and we're choosing to provide default do-nothing input
			CharacterInputs = DoNothingInput;
		}

		// We don't have a local controller so we can't run the code below. This is ok. Simulated proxies will just use previous input when extrapolating
		return;
	}

	CharacterInputs.ControlRotation = FRotator::ZeroRotator;

	CharacterInputs.SetMoveInput(EMoveInputType::Velocity, LocomotionInputVector);

	static float RotationMagMin(1e-3);

	const bool bHasAffirmativeMoveInput = (CharacterInputs.GetMoveInput().Size() >= RotationMagMin);

	// Figure out intended orientation
	CharacterInputs.OrientationIntent = FVector::ZeroVector;


	// set the intent to the actors movement direction
	//CharacterInputs.OrientationIntent = CharacterInputs.GetMoveInput().GetSafeNormal();

	CharacterInputs.bIsJumpPressed = false;
	CharacterInputs.bIsJumpJustPressed = false;

	// Convert inputs to be relative to the current movement base (depending on options and state)
	CharacterInputs.bUsingMovementBase = false;
}
