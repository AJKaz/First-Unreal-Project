// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosive.h"
#include "Main.h"

AExplosive::AExplosive() {
	Damage = 15.f;
}

void AExplosive::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) {
	Super::OnOverlapBegin(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	UE_LOG(LogTemp, Warning, TEXT("Explosive::OnOverlapBegin"));

	if (OtherActor) {
		// Cast OtherActor to AMain, returns null if can't
		AMain* Main = Cast<AMain>(OtherActor);
		if (Main) {
			Main->DecrementHealth(Damage);

			// Destroy item and everything related, memory management, yay!
			Destroy();
		}
	}

}

void AExplosive::OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
	Super::OnOverlapEnd(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	UE_LOG(LogTemp, Warning, TEXT("Explosive::OnOverlapEnd"));
}
