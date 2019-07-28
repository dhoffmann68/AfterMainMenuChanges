// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HoffmannMehatGameMode.h"
#include "GameFramework/Actor.h"
#include "SpawnVolume.generated.h"


UCLASS()
class HOFFMANNMEHAT_API ASpawnVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnVolume();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Returns the wheretospawn subobject
	FORCEINLINE class UBoxComponent* GetWhereToSpawn() const { return WhereToSpawn; }

	//Finding a random point 
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	FVector GetRandomPointinVolume();

	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnPickup();



protected:

	UPROPERTY(EditAnywhere, Category = "Spawning")
		TSubclassOf<class ATheFirstActor> WhatToSpawn;

	FTimerHandle SpawnTimer;

	//Minimum Spawn Delay
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Spawning")
		float SpawnDelayRangeLow;

	//Maximum Spawn Delay
	UPROPERTY(EditAnywhere, BluePrintReadWrite, Category = "Spawning")
		float SpawnDelayRangeHigh;

	AHoffmannMehatGameMode * GameModeInstance;

private:
	// creating the box component to specify where objects should be spawned
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (AllowPrivateAccess = "true"))
		class UBoxComponent* WhereToSpawn;

	

	// The current spawn delay
	float SpawnDelay;

};
