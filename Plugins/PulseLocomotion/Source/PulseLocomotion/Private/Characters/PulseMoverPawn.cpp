// Copyright © by Tyni Boat. All Rights Reserved.


#include "Characters/PulseMoverPawn.h"

#include "MoverComponent.h"


// Sets default values
APulseMoverPawn::APulseMoverPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	MoverComponent = CreateDefaultSubobject<UMoverComponent>("MoverComponent");
}

// Called when the game starts or when spawned
void APulseMoverPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APulseMoverPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APulseMoverPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

