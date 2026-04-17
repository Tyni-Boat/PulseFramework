// Copyright © by Tyni Boat. All Rights Reserved.


#include "Characters/PulseCharacterMoverPawn.h"

#include "Components/CapsuleComponent.h"


// Sets default values
APulseCharacterMoverPawn::APulseCharacterMoverPawn()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	FName PawnCollisionProfileName(TEXT("Pawn"));
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	CapsuleComponent->SetCollisionProfileName(PawnCollisionProfileName);
	CapsuleComponent->SetGenerateOverlapEvents(true);
	CapsuleComponent->SetCanEverAffectNavigation(true);
	CapsuleComponent->SetCapsuleHalfHeight(90.f);
	CapsuleComponent->SetCapsuleRadius(35.f);
	SetRootComponent(CapsuleComponent);
	if (_MasterMeshComp)
	{
		_MasterMeshComp->SetupAttachment(CapsuleComponent);
		_MasterMeshComp->SetRelativeLocation({0,0,-92.f});
	}
	CharacterMoverComponent = CreateDefaultSubobject<UCharacterMoverComponent>("CharacterMoverComponent");
}

// Called when the game starts or when spawned
void APulseCharacterMoverPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APulseCharacterMoverPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void APulseCharacterMoverPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

