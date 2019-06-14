// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class VRDEMO_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneComponent* VRRoot;

	UPROPERTY()
		class UMotionControllerComponent* LeftController;

	UPROPERTY()
		class UMotionControllerComponent* RightController;
	

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY()
		class UPostProcessComponent* PostProcessComponent;

	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinderMaterialBase;

	UPROPERTY()
		class UMaterialInstanceDynamic* BlinderMaterialInstance;

	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity;

	UPROPERTY(EditAnywhere, Category = "Teleport")
	float MaxTeleportDistance = 1000;

	UPROPERTY(EditAnywhere, Category = "Teleport")
	float TeleportProjectileSpeed = 800;

	UPROPERTY(EditAnywhere, Category = "Teleport")
		float TeleportSimulationTime = 10;

	UPROPERTY(EditAnywhere, Category = "Teleport")
	float TeleportProjectileRadius = 10;

	UPROPERTY(EditAnywhere, Category = "Teleport")
		float TeleportFadeTime = 2;

	UPROPERTY(EditAnywhere, Category = "Teleport")
		FVector TeleportProjectionExtent = FVector(100, 100, 100);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	void MoveForward(float throttle);
	void MoveRight(float throttle);
	
	void UpdateDestinationMarker();

	void BeginTeleport();
	void EndTeleport();
	
	void UpdateBlinders();
	FVector2D GetBlinderCenter();

	bool FindTeleportDestination(FVector &OutLocation);
	bool FindTeleportDestinationByHand(FVector &OutLocation);

	bool FindParabolicTeleportDestinationByHand(FVector &OutLocation);
};
