// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HoffmannMehatGameMode.generated.h"

UCLASS(minimalapi)
class AHoffmannMehatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHoffmannMehatGameMode();
	int numTargetsRemaining;
};



