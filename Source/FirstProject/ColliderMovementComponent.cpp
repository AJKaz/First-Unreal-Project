// Fill out your copyright notice in the Description page of Project Settings.


#include "ColliderMovementComponent.h"

void UColliderMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
	//Call parent TickComponent()
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Checks to see if we should do nothing (exit function early)
	if (!PawnOwner || !UpdatedComponent || ShouldSkipUpdate(DeltaTime)) {
		return;
	}

	FVector DesiredMovementThisFrame = ConsumeInputVector().GetClampedToMaxSize(1.0f);

	//Checks to make sure the vector is not almost 0
	if (!DesiredMovementThisFrame.IsNearlyZero()) {
		FHitResult Hit;
		SafeMoveUpdatedComponent(DesiredMovementThisFrame, UpdatedComponent->GetComponentRotation(), true, Hit);
		
		// If we bump into something, slide along the side of it
		if (Hit.IsValidBlockingHit()) {
			SlideAlongSurface(DesiredMovementThisFrame, 1.f - Hit.Time, Hit.Normal, Hit);
		}
	}

}