// Fill out your copyright notice in the Description page of Project Settings.

#include "SpawnVolume.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TheFirstActor.h"
#include "TimerManager.h"


// Sets default values
ASpawnVolume::ASpawnVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WhereToSpawn = CreateDefaultSubobject<UBoxComponent>(TEXT("Where To Spawn"));
	RootComponent = WhereToSpawn;

	//Setting spawn delay range
	SpawnDelayRangeLow = 1.0f;
	SpawnDelayRangeHigh = 4.5f;

}

// Called when the game starts or when spawned
void ASpawnVolume::BeginPlay()
{
	Super::BeginPlay();
	//SpawnDelay = FMath::FRandRange(SpawnDelayRangeLow, SpawnDelayRangeHigh);
	//GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::SpawnPickup, SpawnDelay, false);
	
	/**
		TO DO

		- Need to implement synchronization capabilities when accessing testVar

	*/
	GameModeInstance = (AHoffmannMehatGameMode*)GetWorld()->GetAuthGameMode();
	
}

// Called every frame
void ASpawnVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector ASpawnVolume::GetRandomPointinVolume()
{
	FVector SpawnOrigin = WhereToSpawn->Bounds.Origin;
	FVector SpawnExtent = WhereToSpawn->Bounds.BoxExtent;

	return UKismetMathLibrary::RandomPointInBoundingBox(SpawnOrigin, SpawnExtent);
}


void ASpawnVolume::SpawnPickup()
{
	// if we set something to spawn
	if (WhatToSpawn != NULL) {
		// checking if it is a valid world
		UWorld* const World = GetWorld();
		if (World) {
			//set the spawn parameters
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.Instigator = Instigator;

			//Get a random location to spawn at 
			FVector SpawnLocation = GetRandomPointinVolume();

			//Get a random rotation
			FRotator SpawnRotation;
			SpawnRotation.Yaw =  360.0f;
			SpawnRotation.Pitch = 360.0f;
			SpawnRotation.Roll = 360.0f;

			// spawn the pickup
			ATheFirstActor* const SpawnedPickup = World->SpawnActor<ATheFirstActor>(WhatToSpawn, SpawnLocation, SpawnRotation, SpawnParams);

			//SpawnDelay = FMath::FRandRange(SpawnDelayRangeLow, SpawnDelayRangeHigh);
			//GetWorldTimerManager().SetTimer(SpawnTimer, this, &ASpawnVolume::SpawnPickup, SpawnDelay, false);

		}
	}
}
