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
#include "Components/SplineComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Curves/CurveFloat.h"
#include "MotionControllerComponent.h"
#include "Kismet/GameplayStatics.h"


// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot"));
	VRRoot->SetupAttachment(GetRootComponent());

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VRRoot);

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath"));
	TeleportPath->SetupAttachment(RightController);

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

	//Handle Player not being in the center of playspace
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
	TArray<FVector> Path;

	//if(FindTeleportDestination(TeleportLoc)){ 
	//if (FindTeleportDestinationByHand(TeleportLoc)) {
	if (FindParabolicTeleportDestinationByHand(Path, TeleportLoc)) {

		DestinationMarker->SetVisibility(true);
		DestinationMarker->SetWorldLocation(TeleportLoc);
		UpdateSpline(Path);
	}
	else {
		DestinationMarker->SetVisibility(false);
	}

	
}

void AVRCharacter::UpdateSpline(TArray<FVector> &Path) {
	TeleportPath->ClearSplinePoints(false);

	for (int32 i = 0; i < Path.Num(); i++) {
		
		FVector LocalPos = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);
		FSplinePoint Point(i, LocalPos, ESplinePointType::Curve);
		TeleportPath->AddPoint(Point, false);
	}
	TeleportPath->UpdateSpline();
}

void AVRCharacter::UpdateBlinders() {
	if (RadiusVsVelocity != nullptr) {
		float Velocity = GetVelocity().Size();
		float Radius = RadiusVsVelocity->GetFloatValue(Velocity);

		BlinderMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);

		FVector2D Center = GetBlinderCenter();
		BlinderMaterialInstance->SetVectorParameterValue(TEXT("Center"), FLinearColor(Center.X, Center.Y, 0));
	}
}
	
FVector2D AVRCharacter::GetBlinderCenter() {
	FVector MovementDirection = GetVelocity().GetSafeNormal();
	//center it if we aren't moving or barely moving
	if (MovementDirection.IsNearlyZero()) {
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0) {
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 100;
	}
	else {
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 100;
	}
	

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr) {
		return FVector2D(0.5, 0.5);
	}
	FVector2D ScreenStationaryLocation;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocation);

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocation.X /= SizeX;
	ScreenStationaryLocation.Y /= SizeY;

	return ScreenStationaryLocation;
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

bool AVRCharacter::FindTeleportDestinationByHand(FVector &OutLocation) {
	FHitResult HitResult;

	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();
	Look - Look.RotateAngleAxis(45, RightController->GetRightVector());
	FVector End = Start + Look * MaxTeleportDistance;

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

bool AVRCharacter::FindParabolicTeleportDestinationByHand(TArray<FVector>&OutPath, FVector &OutLocation) {
	FHitResult HitResult;

	FVector Start = RightController->GetComponentLocation();
	FVector Look = RightController->GetForwardVector();
	

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,
		Start,
		Look * TeleportProjectileSpeed,
		TeleportSimulationTime,
		ECollisionChannel::ECC_Visibility,
		this	
	);
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;
	//Note: This is a Hack to fix the bad collision meshes
	Params.bTraceComplex = true;
	FPredictProjectilePathResult Result;	

	//DrawDebugLine(GetWorld(), Start, End, FColor(255, 0, 0));
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);
	if (!bHit) {
		return bHit;
	}

	const UNavigationSystemV1* navSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(this);
	FNavLocation NavLocation;

	bool bOnNavMesh = navSystem->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);
	if (!bOnNavMesh) {
		return bOnNavMesh;
	}
	for (FPredictProjectilePathPointData PointData : Result.PathData) {
		OutPath.Add(PointData.Location);
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
