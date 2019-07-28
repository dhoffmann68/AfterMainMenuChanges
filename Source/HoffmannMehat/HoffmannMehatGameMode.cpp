// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HoffmannMehatGameMode.h"
#include "HoffmannMehatHUD.h"
#include "HoffmannMehatCharacter.h"
#include "UObject/ConstructorHelpers.h"

AHoffmannMehatGameMode::AHoffmannMehatGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	//static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	//DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AHoffmannMehatHUD::StaticClass();

	numTargetsRemaining = 0;
}
