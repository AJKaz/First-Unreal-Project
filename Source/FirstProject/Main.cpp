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
#include "MainPlayerController.h"
#include "MainAnimInstance.h"
#include "FirstSaveGame.h"
#include "ItemStorage.h"


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

	// Sets turn rates for inputs & set jump velocity
	BaseTurnRate = 65.f;
	BaseLookUpRate = 65.f;
	MaxJumpVelocity = 550.f;

	// Stop camera from turning character
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Config character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;			// Makes character move in direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.f, 0.0f);	// at THIS rotation rate
	GetCharacterMovement()->JumpZVelocity = MaxJumpVelocity;;
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

	// Sprint/Run/Jump Variables:
	RunningSpeed = 500.f;
	SprintingSpeed = 750.f;
	StaminaDrainRate = 25.f;
	MinSprintStamina = 50.f;
	SJumpCost = 30.f;

	// Combat Variables:
	InterpSpeed = 15.f;

	// Misc bools
	bMovingForward = false;
	bMovingRight = false;
	bShiftKeyDown = false;
	bLMBDown = false;
	bRMBDown = false;
	bInteractDown = false;
	bInterpToEnemy = false;
	bAttacking = false;
	bHasCombatTarget = false;
	bIsInAir = false;
	bESCDown = false;

	// Initalize Enums to Normal:
	MovementStatus = EMovementStatus::EMS_Normal;
	StaminaStatus = EStaminaStatus::ESS_Normal;	
		
}


// Called when the game starts or when spawned
void AMain::BeginPlay() {
	Super::BeginPlay();	

	MainPlayerController = Cast<AMainPlayerController>(GetController());
}

// Called every frame
void AMain::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	if (!IsAlive()) return;

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
				Stamina += DeltaStamina * 0.9;
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
				Stamina += DeltaStamina * 0.9;
			}
			else {
				Stamina += DeltaStamina * 0.9;
			}
			SetMovementStatus(EMovementStatus::EMS_Normal);
		}
		break;
	case EStaminaStatus::ESS_Exhausted:
		SetStaminaStatus(EStaminaStatus::ESS_ExhaustedRecovering);
		Stamina += DeltaStamina * 0.6;
		SetMovementStatus(EMovementStatus::EMS_Normal);
		break;
	case EStaminaStatus::ESS_ExhaustedRecovering:
		if (Stamina + DeltaStamina >= MinSprintStamina) {
			SetStaminaStatus(EStaminaStatus::ESS_Normal);
			Stamina += DeltaStamina * 0.6;
		}
		else {
			Stamina += DeltaStamina * 0.6;
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

	if (CombatTarget) {
		CombatTargetLocation = CombatTarget->GetActorLocation();
		if (MainPlayerController) {
			MainPlayerController->EnemyLocation = CombatTargetLocation;
		}
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
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMain::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind keyboard sprinting
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &AMain::ShiftKeyDown);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &AMain::ShiftKeyUp);

	// Bind Pause
	PlayerInputComponent->BindAction("ESC", IE_Pressed, this, &AMain::ESCDown).bExecuteWhenPaused = true;
	PlayerInputComponent->BindAction("ESC", IE_Released, this, &AMain::ESCUp).bExecuteWhenPaused = true;

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
	PlayerInputComponent->BindAxis("Turn", this, &AMain::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &AMain::LookUp);

	// Binds keyboard turning
	PlayerInputComponent->BindAxis("TurnRate", this, &AMain::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMain::LookUpAtRate);
}

bool AMain::CanMove(float Value) {
	return (Value != 0.0f) && !bAttacking && IsAlive();
}

/// <summary>
/// Moves player depending on direction camera is facing AND input they press
/// </summary>
void AMain::MoveForward(float Value) {
	if (CanMove(Value)) {
		bMovingForward = true;
		// Finds out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
	else {
		bMovingForward = false;
	}
}


void AMain::MoveRight(float Value) {
	if (CanMove(Value)) {
		bMovingRight = true;
		// Finds out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
	else {
		bMovingRight = false;
	}
}

void AMain::Turn(float Value) {
	if (CanMove(Value)) {
		AddControllerYawInput(Value);
	}
}

void AMain::LookUp(float Value) {
	if (CanMove(Value)) {
		AddControllerPitchInput(Value);
	}
}

void AMain::SetIsInAir(bool InAir) { bIsInAir = InAir; }

void AMain::Jump() {
	if (!bIsInAir && IsAlive()) {
		if (Stamina >= SJumpCost) {
			GetCharacterMovement()->JumpZVelocity = MaxJumpVelocity;
			//Super::Jump(); This works but shows error in visual studio
			Stamina -= SJumpCost;
			ACharacter::Jump();
		}
		else {
			// If characted doesn't have enough stamina, let them 
			// partially jump depending on how much stamina they have
			GetCharacterMovement()->JumpZVelocity = (Stamina / SJumpCost) * MaxJumpVelocity;
			Stamina = 0;
			ACharacter::Jump();									
		}		
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

	// If player is dead, don't accept LMB input
	if (!IsAlive()) return;

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

	// If player is dead, don't let them attack
	if (!IsAlive()) return;

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

	// If player is dead, don't let them pick up items
	if (!IsAlive()) return;

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

void AMain::IncrementCoins(int32 Amount) {
	Coins += Amount;
}

void AMain::IncrementHealth(float Amount) {
	if (Health + Amount >= MaxHealth) 
		Health = MaxHealth;	
	else 
		Health += Amount;	
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
	float StaminaCost = EquippedWeapon->SLightAttackCost;
	if (!bAttacking && (Stamina >= StaminaCost) && (MovementStatus != EMovementStatus::EMS_Dead)) {
		Stamina -= StaminaCost;
		bAttacking = true;
		bInterpToEnemy = true;
		EquippedWeapon->bLightAttack = true;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance && CombatMontage) {
			AnimInstance->Montage_Play(CombatMontage, 1.5f);
			AnimInstance->Montage_JumpToSection(FName("Attack_1"), CombatMontage);
		}
	}	
}

void AMain::HeavyAttack() {	
	float StaminaCost = EquippedWeapon->SHeavyAttackCost;
	if (!bAttacking && (Stamina >= StaminaCost) && (MovementStatus != EMovementStatus::EMS_Dead)) {
		Stamina -= StaminaCost;
		bAttacking = true;
		bInterpToEnemy = true;
		EquippedWeapon->bLightAttack = false;
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

float AMain::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) {
	// If player has no health, decrement health (for health bar visuals), and die
	if (Health - DamageAmount <= 0.f) {
		Health -= DamageAmount;
		Die();
		if (DamageCauser) {
			AEnemy* Enemy = Cast<AEnemy>(DamageCauser);
			if (Enemy) {
				Enemy->bHasValidTarget = false;
			}
		}
	}
	else {
		Health -= DamageAmount;
	}

	return DamageAmount;
}

void AMain::DecrementHealth(float Amount) {
	// If player has no health, decrement health (for health bar visuals), and die
	if (Health - Amount <= 0.f) {
		Health -= Amount;
		Die();		
	}
	else {
		Health -= Amount;
	}
}

void AMain::Die() {	
	if (MovementStatus == EMovementStatus::EMS_Dead) return;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && CombatMontage) {
		AnimInstance->Montage_Play(CombatMontage, 1.0f);
		AnimInstance->Montage_JumpToSection(FName("Death"));		
	}
	SetMovementStatus(EMovementStatus::EMS_Dead);
}

void AMain::DeathEnd() {
	GetMesh()->bPauseAnims = true;
	GetMesh()->bNoSkeletonUpdate = true;
}

void AMain::UpdateCombatTarget() {
	TArray<AActor*> OverlappingActors;	
	GetOverlappingActors(OverlappingActors, EnemyFilter);
	
	// If there are no enemies:
	if (OverlappingActors.Num() == 0) {
		if (MainPlayerController)
			MainPlayerController->RemoveEnemyHealthBar();			
		return;
	}
	
	// Find enemy closest to player:
	AEnemy* ClosestEnemy = Cast<AEnemy>(OverlappingActors[0]);
	if (ClosestEnemy) {
		FVector Location = GetActorLocation();
		float MinDistance = (ClosestEnemy->GetActorLocation() - Location).Size();

		for (auto Actor : OverlappingActors) {
			AEnemy* Enemy = Cast<AEnemy>(Actor);
			if (Enemy) {
				float DistanceToActor = (Enemy->GetActorLocation() - Location).Size();
				if (DistanceToActor < MinDistance) {
					MinDistance = DistanceToActor;
					ClosestEnemy = Enemy;
				}
			}			
		}
		// Set combat target & display enemy health
		if (MainPlayerController) {
			MainPlayerController->DisplayEnemyHealthBar();
		}
		SetCombatTarget(ClosestEnemy);
		bHasCombatTarget = true;
	}
}

void AMain::SwitchLevel(FName LevelName) {
	UWorld* World = GetWorld();
	if (World) {
		FString CurrentLevel = World->GetMapName();
		FName CurrentLevelName(*CurrentLevel);
		if (CurrentLevelName != LevelName) {
			UGameplayStatics::OpenLevel(World, LevelName);
		}
	}
}

void AMain::SaveGame() {
	UFirstSaveGame* SaveGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	/* Save player's stats ie health, stamina, coins, enemies killed, etc */
	SaveGameInstance->CharacterStats.Health = Health;
	SaveGameInstance->CharacterStats.MaxHealth = MaxHealth;
	SaveGameInstance->CharacterStats.Stamina = Stamina;
	SaveGameInstance->CharacterStats.MaxStamina = MaxStamina;
	SaveGameInstance->CharacterStats.Coins = Coins;

	if (EquippedWeapon) {
		SaveGameInstance->CharacterStats.WeaponName = EquippedWeapon->Name;
	}

	/* Save player's location and rotation */
	SaveGameInstance->CharacterStats.Location = GetActorLocation();
	SaveGameInstance->CharacterStats.Rotation = GetActorRotation();

	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->PlayerName, SaveGameInstance->UserIndex);

}

void AMain::LoadGame(bool SetPosition) {
	UFirstSaveGame* LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::CreateSaveGameObject(UFirstSaveGame::StaticClass()));

	LoadGameInstance = Cast<UFirstSaveGame>(UGameplayStatics::LoadGameFromSlot(LoadGameInstance->PlayerName, LoadGameInstance->UserIndex));

	Health = LoadGameInstance->CharacterStats.Health;
	MaxHealth = LoadGameInstance->CharacterStats.MaxHealth;
	Stamina = LoadGameInstance->CharacterStats.Stamina;
	MaxStamina = LoadGameInstance->CharacterStats.MaxStamina;
	Coins = LoadGameInstance->CharacterStats.Coins;

	if (WeaponStorage) {
		AItemStorage* Weapons = GetWorld()->SpawnActor<AItemStorage>(WeaponStorage);
		if (Weapons) {
			FString WeaponName = LoadGameInstance->CharacterStats.WeaponName;
			if (Weapons->WeaponMap.Contains(WeaponName)) {
				AWeapon* WeaponToEquip = GetWorld()->SpawnActor<AWeapon>(Weapons->WeaponMap[WeaponName]);
				WeaponToEquip->Equip(this);
			}	
		}		
	}

	if (SetPosition) {
		SetActorLocation(LoadGameInstance->CharacterStats.Location);
		SetActorRotation(LoadGameInstance->CharacterStats.Rotation);
	}	
}

void AMain::ESCDown() {
	bESCDown = true;
	if (MainPlayerController) {
		MainPlayerController->TogglePauseMenu();
		if (MainPlayerController->bPauseMenuVisible) {
			MainPlayerController->SetPause(true);
		}
		else {
			MainPlayerController->SetPause(false);
		}
	}
}

void AMain::ESCUp() {
	bESCDown = false;
}
