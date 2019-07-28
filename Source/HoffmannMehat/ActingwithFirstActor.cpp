// Fill out your copyright notice in the Description page of Project Settings.

#include "ActingwithFirstActor.h"
#include "TheFirstActor.h"
#include "Components/StaticMeshComponent.h"

//Sets the default values 
AActingwithFirstActor::AActingwithFirstActor()
{

	GetMesh()->SetSimulatePhysics(false);
}
