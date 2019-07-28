// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "HoffmannMehatHUD.generated.h"

UCLASS()
class AHoffmannMehatHUD : public AHUD
{
	GENERATED_BODY()

public:
	AHoffmannMehatHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

