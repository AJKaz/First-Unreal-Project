// Fill out your copyright notice in the Description page of Project Settings.


#include "Main.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Weapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundCue.h"
#include "Kismet/KismetMathLibrary.h"
#include "Enemy.h"


// Sets default values
AMain::AMain() {
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create camera boom (pulls towards player if there's collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 500.0f;			// Camera follows player at this distance
	CameraBoom->bUsePawnControlRotation = true;		// Rotate arm based on controller

	// Set main character's collision capsule size 
	GetCapsuleComponent()->SetCapsuleSize(27.0f, 88.0f);

	// Creates follow camera and attach to boom arm
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach camera to end of boom, and let boom adjust
	// to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false;

	// Sets turn rates for inputs
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;

	// Stop camera from turning character
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Config character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;			// Makes character move in direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f);	// at THIS rotation rate
	GetCharacterMovement()->JumpZVelocity = 550.f;
	GetCharacterMovement()->AirControl = 0.75f;							// Allows movement in air, set to 1.0f for full movement

	/**
	*
	* Player Stats
	*
	*/
	Coins = 0;
	MaxHealth = 100.f;
	Health = MaxHealth;
	MaxStamina = 250.f;
	Stamina = MaxStamina;

	// MOVE THESE TO WEAPON.CPP 
	SHeavyAttackCost = 80.f;
	SLightAttackCost = 45.f;
	SJumpCost = 30.f;

	RunningSpeed = 500.f;
	SprintingSpeed = 750.f;
	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;

	InterpSpeed = 15.f;

	bMovingForward = false;
	bMovingRight = false;
	bShiftKeyDown = false;
	bLMBDown = false;
	bRMBDown = false;
	bInteractDown = false;
	bInterpToEnemy = false;
	bAttacking = false;

	// Initalize Enums to Normal:
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;	
		
}


// Called when the game starts or when spawned
void AMain::BeginPlay() {
	Super::BeginPlay();	
}

// Called every frame
void AMain::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	float DeltaStamina = StaminaDrainRate * DeltaTime;
	switch (StaminaStatus) {
	case EStaminaStatus::ESS_Normal:
		if (bShiftKeyDown && (bMovingForward || bMovingRight)) {
			// Use stamina
			if (Stamina - DeltaStamina <= MinSprintStamina) {
				SetStaminaStatus(EStaminaStatus::ESS_BelowMinimum);
				Stamina -= DeltaStamina;
			}
			else {
				Stamina -= DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Sprinting);
		}
		else {
			// Replenish stamina
			if (Stamina + DeltaStamina >= MaxStamina) {
				Stamina = MaxStamina;
			}
			else {
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_BelowMinimum:
		if (bShiftKeyDown && (bMovingForward || bMovingRight)) {
			// Use Stamina
			if (Stamina - DeltaStamina <= 0.f) {
				SetStaminaStatus(EStaminaStatus::ESS_Exhausted);
				Stamina = 0;
				SetMovementStatus(EMovementStatus::EMS_Normal);
			}
			else {
				Stamina -= DeltaStamina;
				SetMovementStatus(EMovementStatus::EMS_Sprinting);
			}
		}
		else {
			// replenish stamina
			if (Stamina + DeltaStamina >= MinSprintStamina) {
				SetStaminaStatus(EStaminaStatus::ESS_Normal);
				Stamina += DeltaStamina;
			}
			else {
				Stamina += DeltaStamina;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_Exhausted:
		if (bShiftKeyDown && (bMovingForward || bMovingRight)) {
			Stamina = 0.f;
		}
		else {
			SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		if (Stamina + DeltaStamina >= MinSprintStamina) {
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			Stamina += DeltaStamina;
		}
		else {
			Stamina += DeltaStamina;
		}
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	default:
		;
	}

	// Interp to the enemy (aim assist):
	if (bInterpToEnemy && CombatTarget) {
		FRotator LookAtYaw = GetLookAtRotationYaw(CombatTarget->GetActorLocation());
		FRotator InterpRotation = FMath::RInterpTo(GetActorRotation(), LookAtYaw, DeltaTime, InterpSpeed);
		SetActorRotation(InterpRotation);
	}
}

FRotator AMain::GetLookAtRotationYaw(FVector Target) {
	FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Target);
	FRotator LookAtRotationYaw(0.f, LookAtRotation.Yaw, 0.f);
	return LookAtRotationYaw;
}

// Called to bind functionality to input
void AMain::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	// Bind keyboard movement
	PlayerInputComponent->BindAxis("MoveForward", this, &AMain::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMain::MoveRight);

	// Bind keyboard jumping
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind keyboard sprinting
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	// Bind left mouse button
	PlayerInputComponent->BindAction("LMB", IE_Pressed, this, &AMain::LMBDown);
	PlayerInputComponent->BindAction("LMB", IE_Released, this, &AMain::LMBUp);

	// Bind right mouse button
	PlayerInputComponent->BindAction("RMB", IE_Pressed, this, &AMain::RMBDown);
	PlayerInputComponent->BindAction("RMB", IE_Released, this, &AMain::RMBUp);

	// Bind interact key ("e");
	PlayerInputComponent->BindAction("Interact", IE_Pressed, this, &AMain::InteractDown);
	PlayerInputComponent->BindAction("Interact", IE_Released, this, &AMain::InteractUp);

	// Binds mouse turning
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	// Binds keyboard turning
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);
}

/// <summary>
/// Moves player depending on direction camera is facing AND input they press
/// </summary>
void AMain::MoveForward(float value) {
	if ((Controller != nullptr) && (value != 0.0f) && !bAttacking) {
		bMovingForward = true;
		// Finds out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, value);
	}
	else {
		bMovingForward = false;
	}
}


void AMain::MoveRight(float value) {
	if ((Controller != nullptr) && (value != 0.0f) && !bAttacking) {
		bMovingRight = true;
		// Finds out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, value);
	}
	else {
		bMovingRight = false;
	}
}

void AMain::TurnAtRate(float rate) {
	AddControllerYawInput(rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());	
}

void AMain::LookUpAtRate(float rate) {
	AddControllerPitchInput(rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMain::LMBDown() {
	bLMBDown = true;

	// If Player has a weapon equipped:
	if (EquippedWeapon) {
		LightAttack();
	}
}

void AMain::LMBUp() {
	bLMBDown = false;
}

void AMain::RMBDown() {
	bRMBDown = true;
	// If Player has a weapon equipped:
	if (EquippedWeapon) {
		HeavyAttack();
	}
}

void AMain::RMBUp() {
	bRMBDown = false;
}

void AMain::InteractDown() {
	bInteractDown = true;
	if (ActiveOverlappingItem) {
		AWeapon* Weapon = Cast<AWeapon>(ActiveOverlappingItem);
		if (Weapon) {
			Weapon->Equip(this);
			SetActiveOverlappingItem(nullptr);
		}
	}
}
void AMain::InteractUp() {
	bInteractDown = false;
}

void AMain::DecrementHealth(float Amount) {
	// If player has no health, decrement health (for health bar), and die
	if (Health - Amount <= 0.f) {
		Health -= Amount;
		Die();
	}
	else {
		Health -= Amount;
	}
}

void AMain::Die() {

}

void AMain::IncrementCoins(int32 Amount) {
	Coins += Amount;
}

void AMain::SetMovementStatus(EMovementStatus Status) {
	MovementStatus = Status;
	if (MovementStatus == EMovementStatus::EMS_Sprinting) {
		GetCharacterMovement()->MaxWalkSpeed = SprintingSpeed;
	}
	else {
		GetCharacterMovement()->MaxWalkSpeed = RunningSpeed;
	}
}

void AMain::ShiftKeyDown() {
	bShiftKeyDown = true;
}

void AMain::ShiftKeyUp() {
	bShiftKeyDown = false;
}

void AMain::SetEquippedWeapon(AWeapon* WeaponToSet) {
	// TEMPORARILY DESTROY WEAPON WHEN PICKING A NEW WEAPON
	if (EquippedWeapon) {
		EquippedWeapon->Destroy();
	}
	EquippedWeapon = WeaponToSet;
}

void AMain::LightAttack() {	
	if (!bAttacking && Stamina >= SLightAttackCost) {
		Stamina -= SLightAttackCost;
		bAttacking = true;
		bInterpToEnemy = true;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage) {
			AnimInstance->Montage_Play(CombatMontage, 1.5f);
			AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
		}
	}	
}

void AMain::HeavyAttack() {	
	if (!bAttacking && Stamina >= SHeavyAttackCost) {
		Stamina -= SHeavyAttackCost;
		bAttacking = true;
		bInterpToEnemy = true;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage) {
			AnimInstance->Montage_Play(CombatMontage, 0.9f);
			AnimInstance->Montage_JumpToSection(FName("Attack_2"), CombatMontage);
		}
	}	
}

void AMain::AttackEnd() {
	bAttacking = false;
	bInterpToEnemy = false;
	if (bLMBDown) {
		LightAttack();
	}
	else if (bRMBDown) {
		HeavyAttack();
	}

}

void AMain::PlaySwingSound() {
	if (EquippedWeapon->SwingSound) {
		UGameplayStatics::PlaySound2D(this, EquippedWeapon->SwingSound);
	}	
}
