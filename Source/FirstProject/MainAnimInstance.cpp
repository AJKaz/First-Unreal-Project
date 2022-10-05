// Fill out your copyright notice in the Description page of Project Settings.


#include "MainAnimInstance.h"
#include "Main.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMainAnimInstance::NativeInitializeAnimation() {
	if (Pawn == nullptr) {
		// Returns Pawn that owns this animation instance IF there is one
		Pawn = TryGetPawnOwner();
		if (Pawn) {
			Main = Cast<AMain>(Pawn);
		}
	}
}

void UMainAnimInstance::UpdateAnimationProperties() {
	if (Pawn == nullptr) {
		Pawn = TryGetPawnOwner();
	}
	if (Pawn) {
		// returns pawn's velocity at that given frame
		FVector Speed = Pawn->GetVelocity();
		FVector LateralSpeed = FVector(Speed.X, Speed.Y, 0.f);
		MovementSpeed = LateralSpeed.Size();

		bIsInAir = Pawn->GetMovementComponent()->IsFalling();

		if (Main == nullptr) {
			Main = Cast<AMain>(Pawn);
		}
	}
}
