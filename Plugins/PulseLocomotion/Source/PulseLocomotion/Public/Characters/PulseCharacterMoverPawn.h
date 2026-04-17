// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BaseMoverPulsePawn.h"
#include "DefaultMovementSet/CharacterMoverComponent.h"
#include "PulseCharacterMoverPawn.generated.h"



UCLASS(BlueprintType, Blueprintable)
class PULSELOCOMOTION_API APulseCharacterMoverPawn : public ABaseMoverPulsePawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APulseCharacterMoverPawn();

	UPROPERTY(Category="Collision", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(Category="Mover", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterMoverComponent> CharacterMoverComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
