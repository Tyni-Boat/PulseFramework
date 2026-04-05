// Copyright © by Tyni Boat. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BasePulsePawn.h"
#include "MoverComponent.h"
#include "PulseMoverPawn.generated.h"


UCLASS(BlueprintType, Blueprintable)
class PULSELOCOMOTION_API APulseMoverPawn : public ABasePulsePawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	APulseMoverPawn();

	UPROPERTY(Category="Mover", VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UMoverComponent> MoverComponent;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
