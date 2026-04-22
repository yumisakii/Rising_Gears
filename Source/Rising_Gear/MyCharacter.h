// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyCharacter.generated.h"

UCLASS()
class RISING_GEAR_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();

	// Core Engine Overrides
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Jump() override;

	// --- Grappling System ---
	UFUNCTION(BlueprintImplementableEvent, Category = "Grappling")
	void OnUpdateGrappleRope(FVector StartLocation, FVector EndLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Grappling")
	void OnLaunchGrappleRope();

protected:
	virtual void BeginPlay() override;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* FirstPersonCameraComponent;

	// Enhanced Input Actions
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* ShootGrapplingHookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	class UInputAction* DashAction;

	// Fov Camera
	void UpdateCameraFOV(float DeltaTime);

	// --- Jump System ---
	float TimeSinceLeftGround;
	float JumpBufferTimeLeft = 0.0f;
	virtual void Landed(const FHitResult& Hit) override;
	void UpdateJumpValues(float DeltaTime);

	// --- Grappling System ---
	UPROPERTY(BlueprintReadOnly, Category = "Grappling")
	class AActor* CurrentGrappleTarget;

	UPROPERTY(EditDefaultsOnly, Category = "Grappling")
	TSubclassOf<AActor> GrappleRopeClass;

	UPROPERTY(BlueprintReadOnly)
	AActor* CurrentGrappleRope;

	bool bIsGrappling;
	FVector GrappleHookLocation;

	float CurrentRopeLength;
	
	float BaseForwardMomentumForce = 1000.f;
	float CurrentForwardMomentumForce;

	bool PerformGrappleSweep(FHitResult& OutHitResult, FVector& OutStartLocation, FVector& OutEndLocation);
	void ShootGrapplingHook();
	void StopGrappling();
	void HandleGrapplingMovement(float DeltaTime);
	void UpdateGrappleTarget();

	// --- Dash System ---

	// Dash State
	bool bIsDashing;
	bool bCanDash;
	FVector DashStartLocation;
	FVector DashDirection;
	FVector PreviousDashLocation;

	// Dash Configuration
	float TargetDashDistance = 450.0f;
	float DashSpeed = 6000.0f;
	float DashCooldownDuration = 2.0f;

	// Dash Management
	struct FTimerHandle DashCooldownTimerHandle;

	void Dash();
	void HandleDash(float DeltaTime);
	void StopDash();
	void ResetDashCooldown();
};