// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "Components/CapsuleComponent.h"
#include "NavigationSystem.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Curves/CurveFloat.h"


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker"));
	DestinationMarker->SetupAttachment(GetRootComponent());

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent"));
	PostProcessComponent->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (BlinderMaterialBase != NULL) {
		BlinderMaterialInstance = UMaterialInstanceDynamic::Create(BlinderMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinderMaterialInstance);
	}
	
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector ActorLoc = Camera->GetComponentLocation();

	FVector NewCamOffset = Camera->GetComponentLocation() - GetActorLocation();
	//stop it from moving in Z
	NewCamOffset.Z = 0;
	AddActorWorldOffset(NewCamOffset);
	VRRoot->AddWorldOffset(-NewCamOffset);

	UpdateDestinationMarker();
	UpdateBlinders();
}

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);
}

void AVRCharacter::MoveForward(float throttle) {
	AddMovementInput(throttle * Camera->GetForwardVector());
}

void AVRCharacter::MoveRight(float throttle) {
	AddMovementInput(throttle * Camera->GetRightVector());
}

void AVRCharacter::UpdateDestinationMarker() {
	
	
	FVector TeleportLoc;
		if(FindTeleportDestination(TeleportLoc)){
			DestinationMarker->SetVisibility(true);
			DestinationMarker->SetWorldLocation(TeleportLoc);
		}
		else {
			DestinationMarker->SetVisibility(false);
		}
}

void AVRCharacter::UpdateBlinders() {
	if (RadiusVsVelocity != nullptr) {
		float Velocity = GetVelocity().Size();
		float Radius = RadiusVsVelocity->GetFloatValue(Velocity);

		BlinderMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);
	}
}
	

bool AVRCharacter::FindTeleportDestination(FVector &OutLocation) {
	FHitResult HitResult;

	FVector Start = Camera->GetComponentLocation();
	FVector End = Start + Camera->GetForwardVector() * MaxTeleportDistance;

	//DrawDebugLine(GetWorld(), Start, End, FColor(255, 0, 0));
	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility);
	if (!bHit) {
		return bHit;
	}

	const UNavigationSystemV1* navSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	FNavLocation NavLocation;

	bool bOnNavMesh = navSystem->ProjectPointToNavigation(HitResult.Location, NavLocation, TeleportProjectionExtent);
	if (!bOnNavMesh) {
		return bOnNavMesh;
	}

	OutLocation = NavLocation.Location;
	return bOnNavMesh && bHit;
}

void AVRCharacter::BeginTeleport() {

	// get player controller
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(0, 1, TeleportFadeTime, FLinearColor::Black);
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::EndTeleport, TeleportFadeTime, false);
}

void AVRCharacter::EndTeleport() {

	FVector TeleportLocation = DestinationMarker->GetComponentLocation();
	TeleportLocation.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SetActorLocation(TeleportLocation);

	

	// get player controller
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr) {
		PC->PlayerCameraManager->StartCameraFade(1, 0, TeleportFadeTime, FLinearColor::Black);
	}
	
}
