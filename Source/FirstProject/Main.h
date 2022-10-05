// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Main.generated.h"

UENUM(BlueprintType)
enum class EMovementStatus : uint8 {
	EMS_Normal UMETA(DisplayName = "Normal"),
	EMS_Sprinting UMETA(DisplayName = "Sprinting"),
	EMS_MAX UMETA(DisplayName = "DefaultMAX")
};

UCLASS()
class FIRSTPROJECT_API AMain : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMain();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Enums")
	EMovementStatus MovementStatus;

	/** Set movement status and running speed */
	void SetMovementStatus(EMovementStatus Status);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Running")
	float RunningSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Running")
	float SprintingSpeed;

	bool bShiftKeyDown;

	/** Press down to enable sprinting*/
	void ShiftKeyDown();

	/** Released to stop sprinting */
	void ShiftKeyUp();

	// Position camera behind the player
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	// Follow Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	// Scales turning functions for camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera);
	float BaseTurnRate;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera);
	float BaseLookUpRate;

	/**
	/*
	/* Player Stats
	/*
	*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")	// set to BluePrintReadOnly
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player Stats")
	float MaxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")	// set to BluePrintReadOnly
	float Stamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Stats")	// set to BluePrintReadOnly
	int32 Coins;
	

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Called for forwards/backwards input
	void MoveForward(float value);

	// Called for side to side input
	void MoveRight(float value);

	/** Called via input to turn at a given rate
	*  @param rate This is a normalized rate, 1.0 means 100% of desired turn rate
	*/
	void TurnAtRate(float rate);

	/** Called via input to look up/down at a given rate
	*  @param rate This is a normalized rate, 1.0 means 100% of desired look up/down rate
	*/
	void LookUpAtRate(float rate);

	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


	void DecrementHealth(float Amount);

	void Die();

	void IncrementCoins(int32 Amount);

};
