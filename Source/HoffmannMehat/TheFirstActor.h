// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheFirstActor.generated.h"

UCLASS()
class HOFFMANNMEHAT_API ATheFirstActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATheFirstActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FORCEINLINE class UStaticMeshComponent* GetMesh() const { return TheFirstActor; }


private: 
	// the static mesh to be the object, 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "The First Object", meta = (AllowPrivateAccess = "true"))
	class UStaticMeshComponent* TheFirstActor;
};
