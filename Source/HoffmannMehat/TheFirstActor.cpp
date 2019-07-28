// Fill out your copyright notice in the Description page of Project Settings.

#include "TheFirstActor.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ATheFirstActor::ATheFirstActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create the static mesh component 
	TheFirstActor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("The First Actor"));
	RootComponent = TheFirstActor;
	

}

// Called when the game starts or when spawned
void ATheFirstActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATheFirstActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

