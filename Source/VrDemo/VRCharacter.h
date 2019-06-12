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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class USceneComponent* VRRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UStaticMeshComponent* DestinationMarker;

	UPROPERTY(EditAnywhere, Category = "Teleport")
	float MaxTeleportDistance = 1000;

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

	bool FindTeleportDestination(FVector &OutLocation);
};
