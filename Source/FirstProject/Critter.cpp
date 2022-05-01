// Fill out your copyright notice in the Description page of Project Settings.


#include "Critter.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"

// Sets default values
ACritter::ACritter()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(GetRootComponent());

	//Initialize Camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetRootComponent());	//attaches camera to root component
	Camera->SetRelativeLocation(FVector(-300.f, 0.f, 300.f));
	Camera->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));

	//Player 0 is first player (in multiplayer) and only player for singleplayer
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	CurrentVelocity = FVector(0.f);
	MaxSpeed = 100.f;

}

// Called when the game starts or when spawned
void ACritter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACritter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewLocation = GetActorLocation() + (CurrentVelocity * DeltaTime);
	SetActorLocation(NewLocation);

}

// Called to bind functionality to input
void ACritter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ACritter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ACritter::MoveRight);

}

void ACritter::MoveForward(float Value) {
	//FMath::Clamp() Ensures Value is never below -1 or above 1
	CurrentVelocity.X = FMath::Clamp(Value, -1.f, 1.f) * MaxSpeed;

}

void ACritter::MoveRight(float Value) {
	CurrentVelocity.Y = FMath::Clamp(Value, -1.f, 1.f) * MaxSpeed;

}
