// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MoverSimulationTypes.h"
#include "Actors/PulsePawnBase.h"
#include "GameFramework/Pawn.h"
#include "BaseMoverPulsePawn.generated.h"

UCLASS(Abstract, NotBlueprintable, BlueprintType)
class PULSELOCOMOTION_API ABaseMoverPulsePawn : public APulsePawnBase, public IMoverInputProducerInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABaseMoverPulsePawn();

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
