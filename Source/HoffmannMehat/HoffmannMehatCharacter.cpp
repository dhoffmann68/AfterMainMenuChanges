// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "HoffmannMehatCharacter.h"
#include "HoffmannMehatProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AHoffmannMehatCharacter

AHoffmannMehatCharacter::AHoffmannMehatCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
	DeadZone = 0.1f;


	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	Mesh1P->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void AHoffmannMehatCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Show or hide the two versions of the gun based on whether or not we're using motion controllers.
	if (bUsingMotionControllers)
	{
		VR_Gun->SetHiddenInGame(false, true);
		Mesh1P->SetHiddenInGame(true, true);
	}
	else
	{
		VR_Gun->SetHiddenInGame(true, true);
		Mesh1P->SetHiddenInGame(false, true);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHoffmannMehatCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AHoffmannMehatCharacter::OnFire);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHoffmannMehatCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AHoffmannMehatCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHoffmannMehatCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHoffmannMehatCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHoffmannMehatCharacter::LookUpAtRate);
}

void AHoffmannMehatCharacter::OnFire()
{
	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			if (bUsingMotionControllers)
			{
				const FRotator SpawnRotation = VR_MuzzleLocation->GetComponentRotation();
				const FVector SpawnLocation = VR_MuzzleLocation->GetComponentLocation();
				World->SpawnActor<AHoffmannMehatProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			}
			else
			{
				NumFire++;
				
				//const FRotator SpawnRotation = GetControlRotation();
				const FRotator SpawnRotation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraRotation();
				// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
				//const FVector SpawnLocation = ((FP_MuzzleLocation != nullptr) ? FP_MuzzleLocation->GetComponentLocation() : GetActorLocation()) + SpawnRotation.RotateVector(GunOffset);
				
				
				const FVector SpawnLocation = GetWorld()->GetFirstPlayerController()->PlayerCameraManager->GetCameraLocation();

				//Set Spawn Collision Handling Override
				FActorSpawnParameters ActorSpawnParams;
				ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

				// spawn the projectile at the muzzle
				World->SpawnActor<AHoffmannMehatProjectile>(ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParams);
			}
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void AHoffmannMehatCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHoffmannMehatCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnFire();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AHoffmannMehatCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void AHoffmannMehatCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void AHoffmannMehatCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AHoffmannMehatCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}





float CalcXInputRate(float xRate)
{	
		
		if (xRate > 97.26027397)
		{
			float t = (xRate - 97.26027397) / (100 - 97.26027397);
			return  0.9634322954 + t * (1 - 0.9634322954);
		}

		if (xRate <= 4.109589041)
		{
			float t = xRate / 4.109589041;
			return  t * 0.006498581106;
		}
		else if(xRate <= 6.849315068) //done
		{
			float t = (xRate - 4.109589041) / (6.849315068 - 4.109589041);
			return 0.006498581106 + t * (0.01079688388 - 0.006498581106);

		}
		else if (xRate <= 9.589041096) //done
		{
			float t = (xRate - 6.849315068) / (9.589041096 - 6.849315068);
			return 0.01079688388 + t * (0.0152762469 - 0.01079688388);
		}
		else if (xRate <= 12.32876712)//done
		{
			float t = (xRate - 9.589041096) / (12.32876712 - 9.589041096);
			return 0.0152762469 + t * (0.02009211719 - 0.0152762469);
		}
		else if (xRate <= 15.06849315)
		{
			float t = (xRate - 12.32876712) / (15.06849315 - 12.32876712);
			return 0.02009211719 + t * (0.02538589227 - 0.02009211719);
		}
		else if (xRate <= 17.80821918)
		{
			float t = (xRate - 15.06849315) / (17.80821918 - 15.06849315);
			return 0.02538589227 + t * (0.03133758862 - 0.02538589227);
		}
		else if (xRate <= 20.54794521)
		{
			float t = (xRate - 17.80821918) / (20.54794521 - 17.80821918);
			return 0.03133758862 + t * (0.03812320917 - 0.03133758862);
		}
		else if (xRate <= 23.28767123)
		{
			float t = (xRate - 20.54794521) / (23.28767123 - 20.54794521);
			return 0.03812320917 + t * (0.04586507636 - 0.03812320917);
		}
		else if (xRate <= 26.02739726)
		{
			float t = (xRate - 23.28767123) / (26.02739726 - 23.28767123);
			return 0.04586507636 + t * (0.05483206264 - 0.04586507636);
		}
		else if (xRate <= 28.76712329)
		{
			float t = (xRate - 26.02739726) / (28.76712329 - 26.02739726);
			return 0.05483206264 + t * (0.06498803302 - 0.05483206264);
		}
		else if (xRate <= 31.50684932)
		{
			float t = (xRate - 28.76712329) / (31.50684932 - 28.76712329);
			return 0.06498803302 + t * (0.07658435503 - 0.06498803302);
		}
		else if (xRate <= 34.24657534)
		{
			float t = (xRate - 31.50684932) / (34.24657534 - 31.50684932);
			return 0.07658435503 + t * (0.0900202977 - 0.07658435503);
		}
		else if (xRate <= 36.98630137)
		{
			float t = (xRate - 34.24657534) / (36.98630137 - 34.24657534);
			return 0.0900202977 + t * (0.1050284181 - 0.0900202977);
		}
		else if (xRate <= 39.7260274)
		{
			float t = (xRate - 36.98630137) / (39.7260274 - 36.98630137);
			return 0.1050284181 + t * (0.1220418272 - 0.1050284181);
		}
		else if (xRate <= 42.46575342)
		{
			float t = (xRate - 39.7260274) / (42.46575342 - 39.7260274);
			return 0.1220418272 + t * (0.1411820883 - 0.1220418272);
		}
		else if (xRate <= 45.20547945)
		{
			float t = (xRate - 42.46575342) / (45.20547945 - 42.46575342);
			return 0.1411820883 + t * (0.1628917728 - 0.1411820883);
		}
		else if (xRate <= 47.94520548)
		{
			float t = (xRate - 45.20547945) / (47.94520548 - 45.20547945);
			return 0.1628917728 + t * (0.1865012616 - 0.1628917728);
		}
		else if (xRate <= 50.68493151)
		{
			float t = (xRate - 47.94520548) / (50.68493151 - 47.94520548);
			return 0.1865012616 + t * (0.212862971 - 0.1865012616);
		}
		else if (xRate <= 53.42465753)
		{
			float t = (xRate - 50.68493151) / (53.42465753 - 50.68493151);
			return 0.212862971 + t * (0.2413386541 - 0.212862971);
		}
		else if (xRate <= 56.16438356)
		{
			float t = (xRate - 53.42465753) / (56.16438356 - 53.42465753);
			return 0.2413386541 + t * (0.2703169443 - 0.2413386541);
		}
		else if (xRate <= 58.90410959)
		{
			float t = (xRate - 56.16438356) / (58.90410959 - 56.16438356);
			return 0.2703169443 + t * (0.3015639166 - 0.2703169443);
		}
		else if (xRate <= 61.64383562)
		{
			float t = (xRate - 58.90410959) / (61.64383562 - 58.90410959);
			return 0.3015639166 + t * (0.336835443 - 0.3015639166);
		}
		else if (xRate <= 64.38356164)
		{
			float t = (xRate - 61.64383562) / (64.38356164 - 61.64383562);
			return 0.336835443 + t * (0.3670344828 - 0.336835443);

		}
		else if (xRate <= 67.12328767)
		{
			float t = (xRate - 64.38356164) / (67.12328767 - 64.38356164);
			return 0.3670344828 + t * (0.4035792826 - 0.3670344828);
		}
		else if (xRate <= 69.8630137)
		{
			float t = (xRate - 67.12328767) / (69.8630137 - 67.12328767);
			return 0.4035792826 + t * (0.4458033171 - 0.4035792826);
		}
		else if (xRate <= 72.60273973)
		{
			float t = (xRate - 69.8630137) / (72.60273973 - 69.8630137);
			return 0.4458033171 + t * (0.4866852001 - 0.4458033171);
		}
		else if (xRate <= 75.34246575)
		{
			float t = (xRate - 72.60273973) / (75.34246575 - 72.60273973);
			return 0.4866852001 + t * (0.5276097948 - 0.4866852001);
		}
		else if (xRate <= 78.08219178)
		{
			float t = (xRate - 75.34246575) / (78.08219178 - 75.34246575);
			return 0.5276097948 + t * (0.5727014463 - 0.5276097948);
		}
		else if (xRate <= 80.82191781)
		{
			float t = (xRate - 78.08219178) / (80.82191781 - 78.08219178);
			return 0.5727014463 + t * (0.622165069 - 0.5727014463);
		}
		else if (xRate <= 83.56164384)
		{
			float t = (xRate - 80.82191781) / (83.56164384 - 80.82191781);
			return 0.622165069 + t * (0.6715457413 - 0.622165069);
		}
		else if (xRate <= 86.30136986)
		{
			float t = (xRate - 83.56164384) / (86.30136986 - 83.56164384);
			return 0.6715457413 + t * (0.7253645904 - 0.6715457413);
		}
		else if (xRate <= 89.04109589)
		{
			float t = (xRate - 86.30136986) / (89.04109589 - 86.30136986);
			return 0.7253645904 + t * (0.7785254535 - 0.7253645904);
		}
		else if (xRate <= 91.78082192)
		{
			float t = (xRate - 89.04109589) / (91.78082192 - 89.04109589);
			return 0.7785254535 + t * (0.8395646001 - 0.7785254535);
		}
		else if (xRate <= 94.52054795)
		{
			float t = (xRate - 91.78082192) / (94.52054795 - 91.78082192);
			return 0.8395646001 + t * (0.9016671185 - 0.8395646001);
		}
		else 
		{
			float t = (xRate - 94.52054795) / (97.26027397 - 94.52054795);
			return 0.9016671185 + t * (0.9634322954 - 0.9016671185);
		}
		
}





float CalcYInputRate(float yRate)
{

	if (yRate > 97.26027397)
	{
		float t = (yRate - 97.26027397) / (100 - 97.26027397);
		return  0.928047968 + t * (1 - 0.928047968);
	}
	//TODO: Special Case
	if (yRate <= 4.109589041)
	{
		float t = yRate / 4.109589041;
		return   t * 0.006258930096;
	}
	else if (yRate <= 6.849315068) //done
	{
		float t = (yRate - 4.109589041) / (6.849315068 - 4.109589041);
		return 0.006258930096 + t * (0.01040934973 - 0.006258930096);

	}
	else if (yRate <= 9.589041096) //done
	{
		float t = (yRate - 6.849315068) / (9.589041096 - 6.849315068);
		return 0.01040934973 + t * (0.01470324358 - 0.01040934973);
	}
	else if (yRate <= 12.32876712)//done
	{
		float t = (yRate - 9.589041096) / (12.32876712 - 9.589041096);
		return 0.01470324358 + t * (0.01936120531 - 0.01470324358);
	}
	else if (yRate <= 15.06849315)
	{
		float t = (yRate - 12.32876712) / (15.06849315 - 12.32876712);
		return 0.01936120531 + t * (0.02447423441 - 0.01936120531);
	}
	else if (yRate <= 17.80821918)
	{
		float t = (yRate - 15.06849315) / (17.80821918 - 15.06849315);
		return 0.02447423441 + t * (0.03019137823 - 0.02447423441);
	}
	else if (yRate <= 20.54794521)
	{
		float t = (yRate - 17.80821918) / (20.54794521 - 17.80821918);
		return 0.03019137823 + t * (0.03667623285 - 0.03019137823);
	}
	else if (yRate <= 23.28767123)
	{
		float t = (yRate - 20.54794521) / (23.28767123 - 20.54794521);
		return 0.03667623285 + t * (0.04422503016 - 0.03667623285);
	}
	else if (yRate <= 26.02739726)
	{
		float t = (yRate - 23.28767123) / (26.02739726 - 23.28767123);
		return 0.04422503016 + t * (0.05270127119 - 0.04422503016);
	}
	else if (yRate <= 28.76712329)
	{
		float t = (yRate - 26.02739726) / (28.76712329 - 26.02739726);
		return 0.05270127119 + t * (0.06265743073 - 0.05270127119);
	}
	else if (yRate <= 31.50684932)
	{
		float t = (yRate - 28.76712329) / (31.50684932 - 28.76712329);
		return 0.06265743073 + t * (0.07391096726 - 0.06265743073);
	}
	else if (yRate <= 34.24657534)
	{
		float t = (yRate - 31.50684932) / (34.24657534 - 31.50684932);
		return 0.07391096726 + t * (0.08681291288 - 0.07391096726);
	}
	else if (yRate <= 36.98630137)
	{
		float t = (yRate - 34.24657534) / (36.98630137 - 34.24657534);
		return 0.08681291288 + t * (0.1013975833 - 0.08681291288);
	}
	else if (yRate <= 39.7260274)
	{
		float t = (yRate - 36.98630137) / (39.7260274 - 36.98630137);
		return 0.1013975833 + t * (0.1176818451 - 0.1013975833);
	}
	else if (yRate <= 42.46575342)
	{
		float t = (yRate - 39.7260274) / (42.46575342 - 39.7260274);
		return 0.1176818451 + t * (0.1358759266 - 0.1176818451);
	}
	else if (yRate <= 45.20547945)
	{
		float t = (yRate - 42.46575342) / (45.20547945 - 42.46575342);
		return 0.1358759266 + t * (0.1566929134 - 0.1358759266);
	}
	else if (yRate <= 47.94520548)
	{
		float t = (yRate - 45.20547945) / (47.94520548 - 45.20547945);
		return 0.1566929134 + t * (0.1793716199 - 0.1566929134);
	}
	else if (yRate <= 50.68493151)
	{
		float t = (yRate - 47.94520548) / (50.68493151 - 47.94520548);
		return 0.1793716199 + t * (0.2055178519 - 0.1793716199);
	}
	else if (yRate <= 53.42465753)
	{
		float t = (yRate - 50.68493151) / (53.42465753 - 50.68493151);
		return 0.2055178519 + t * (0.2310116086 - 0.2055178519);
	}
	else if (yRate <= 56.16438356)
	{
		float t = (yRate - 53.42465753) / (56.16438356 - 53.42465753);
		return 0.2310116086 + t * (0.2598395822 - 0.2310116086);
	}
	else if (yRate <= 58.90410959)
	{
		float t = (yRate - 56.16438356) / (58.90410959 - 56.16438356);
		return 0.2598395822 + t * (0.2888243832 - 0.2598395822);
	}
	else if (yRate <= 61.64383562)
	{
		float t = (yRate - 58.90410959) / (61.64383562 - 58.90410959);
		return 0.2888243832 + t * (0.322603057 - 0.2888243832);
	}
	else if (yRate <= 64.38356164)
	{
		float t = (yRate - 61.64383562) / (64.38356164 - 61.64383562);
		return 0.322603057 + t * (0.3555385401 - 0.322603057);

	}
	else if (yRate <= 67.12328767)
	{
		float t = (yRate - 64.38356164) / (67.12328767 - 64.38356164);
		return 0.3555385401 + t * (0.3899776036 - 0.3555385401);
	}
	else if (yRate <= 69.8630137)
	{
		float t = (yRate - 67.12328767) / (69.8630137 - 67.12328767);
		return 0.3899776036 + t * (0.4307359307 - 0.3899776036);
	}
	else if (yRate <= 72.60273973)
	{
		float t = (yRate - 69.8630137) / (72.60273973 - 69.8630137);
		return 0.4307359307 + t * (0.4683927371 - 0.4307359307);
	}
	else if (yRate <= 75.34246575)
	{
		float t = (yRate - 72.60273973) / (75.34246575 - 72.60273973);
		return 0.4683927371 + t * (0.5041621426 - 0.4683927371);
	}
	else if (yRate <= 78.08219178)
	{
		float t = (yRate - 75.34246575) / (78.08219178 - 75.34246575);
		return 0.5041621426 + t * (0.5580929487 - 0.5041621426);
	}
	else if (yRate <= 80.82191781)
	{
		float t = (yRate - 78.08219178) / (80.82191781 - 78.08219178);
		return 0.5580929487 + t * (0.5991397849 - 0.5580929487);
	}
	else if (yRate <= 83.56164384)
	{
		float t = (yRate - 80.82191781) / (83.56164384 - 80.82191781);
		return 0.5991397849 + t * (0.6326067212 - 0.5991397849);
	}
	else if (yRate <= 86.30136986)
	{
		float t = (yRate - 83.56164384) / (86.30136986 - 83.56164384);
		return 0.6326067212 + t * (0.7103518613 - 0.6326067212);
	}
	else if (yRate <= 89.04109589)
	{
		float t = (yRate - 86.30136986) / (89.04109589 - 86.30136986);
		return 0.7103518613 + t * (0.7546045504 - 0.7103518613);
	}
	else if (yRate <= 91.78082192)
	{
		float t = (yRate - 89.04109589) / (91.78082192 - 89.04109589);
		return 0.7546045504 + t * (0.8056680162 - 0.7546045504);
	}
	else if (yRate <= 94.52054795)
	{
		float t = (yRate - 91.78082192) / (94.52054795 - 91.78082192);
		return 0.8056680162 + t * (0.8822039265 - 0.8056680162);
	}
	else
	{
		float t = (yRate - 94.52054795) / (97.26027397 - 94.52054795);
		return 0.8822039265 + t * (0.928047968 - 0.8822039265);
	}

}











/*
TODO:	still using tempDeadzone variable

*/
void AHoffmannMehatCharacter::TurnAtRate(float xRate)
{
	
	float sign = 1;
	if (xRate < 0)
	{
		sign = -1;
	}
	
	
	float activeRange = (xRate - DeadZone) / (1 - DeadZone);
	if (activeRange < 0)
	{
		activeRange = 0;
	}


	/*
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("X: %f     Y: %f"), xRate, InputComponent->GetAxisValue(TEXT("LookUpRate"))));

	}
	*/
	
	/**
	float yRate = InputComponent->GetAxisValue(TEXT("LookUpRate"));

	float magnitude = sqrt((pow(xRate, 2) + pow(yRate, 2)));

	if (magnitude > 1)
	{
		magnitude = 1;
	}

	float activeRange = (magnitude - DeadZone) / (1 - DeadZone);
	if (activeRange < 0)
	{
		activeRange = 0;
	}

	float angle = atan(yRate / xRate);
	float newXrate = cos(angle);
	float newYrate = sin(angle);
	*/

	float finalXrate = CalcXInputRate(abs(xRate)*100) * sign;


	//FString log = FString::Printf(TEXT("%f: %f"), xRate, finalXrate);
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, log);


	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::SanitizeFloat(finalXrate));
	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::SanitizeFloat(BaseTurnRate));
	// calculate delta for this frame from the rate information
	AddControllerYawInput(finalXrate  * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHoffmannMehatCharacter::LookUpAtRate(float yRate)
{
	float isNegative = 0;

	if (yRate < 0)
	{
		isNegative = 1;
	}

	/*
	float xRate = InputComponent->GetAxisValue(TEXT("TurnRate"));

	float magnitude = sqrt((pow(xRate, 2) + pow(yRate, 2)));

	if (magnitude > 1)
	{
		magnitude = 1;
	}


	float activeRange = (magnitude - DeadZone) / (1 - DeadZone);
	if (activeRange < 0)
	{
		activeRange = 0;
	}

	float angle = atan(yRate / xRate);
	float newXrate = cos(angle);
	float newYrate = sin(angle);

	float finalXrate = newXrate * activeRange;
	*/
	//float finalYrate = newYrate * activeRange; 
	float finalYrate = CalcYInputRate(abs(yRate) * 100);

	if (finalYrate < 0)
	{
		if (!isNegative)
		{
			finalYrate *= -1;
		}
	}
	else
	{
		if (isNegative)
		{
			finalYrate *= -1;
		}
	}

	FString log = FString::Printf(TEXT("%f: %f"), yRate, finalYrate);
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, log);

	//GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::SanitizeFloat(BaseLookUpRate));
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(finalYrate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AHoffmannMehatCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AHoffmannMehatCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AHoffmannMehatCharacter::EndTouch);

		//Commenting this out to be more consistent with FPS BP template.
		//PlayerInputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AHoffmannMehatCharacter::TouchUpdate);
		return true;
	}
	
	return false;
}
