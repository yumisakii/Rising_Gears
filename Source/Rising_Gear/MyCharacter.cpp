#include "MyCharacter.h"

#include "Camera/CameraComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "DrawDebugHelpers.h"

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootComponent);
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 64.f));

	bIsGrappling = false;
	bIsDashing = false;
	bCanDash = true;

	CurrentGrappleTarget = nullptr;
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetCharacterMovement()->IsFalling()) TimeSinceLeftGround += DeltaTime;
	else TimeSinceLeftGround = 0.f;




	UpdateGrappleTarget();

	HandleGrapplingMovement(DeltaTime);
	HandleDash(DeltaTime);
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(ShootGrapplingHookAction, ETriggerEvent::Started, this, &AMyCharacter::ShootGrapplingHook);
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AMyCharacter::Dash);
	}
}

void AMyCharacter::Jump()
{
	if (bIsGrappling || bIsDashing)
	{
		StopGrappling();
		StopDash();

		FVector ForwardBoost = FirstPersonCameraComponent->GetForwardVector();
		ForwardBoost.Z = 0.f;
		ForwardBoost = ForwardBoost.GetSafeNormal();
		ForwardBoost *= 1500;

		FVector UpwardBoost = FVector(0.f, 0.f, 500.0f);
		FVector TotalLaunchVelocity = ForwardBoost + UpwardBoost;

		LaunchCharacter(TotalLaunchVelocity, true, true);
	}
	else if (TimeSinceLeftGround <= 0.15f && GetCharacterMovement()->Velocity.Z <= 0.0f)
	{
		float StandardJumpForce = GetCharacterMovement()->JumpZVelocity;

		LaunchCharacter(FVector(0.f, 0.f, StandardJumpForce), false, true);
		TimeSinceLeftGround = 999.0f;
	}
	else
	{
		Super::Jump();
	}
}


#pragma region Grappling Hook System

bool AMyCharacter::PerformGrappleSweep(FHitResult& OutHitResult, FVector& OutStartLocation, FVector& OutEndLocation)
{
	OutStartLocation = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardDirection = FirstPersonCameraComponent->GetForwardVector();

	float MaxGrappleDistance = 1000.0f;
	OutEndLocation = OutStartLocation + (ForwardDirection * MaxGrappleDistance);

	float GrappleRadius = 300.0f;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(GrappleRadius);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	return GetWorld()->SweepSingleByChannel(
		OutHitResult,
		OutStartLocation,
		OutEndLocation,
		FQuat::Identity,
		ECC_GameTraceChannel2,
		SphereShape,
		QueryParams
	);
}

void AMyCharacter::ShootGrapplingHook()
{
	FHitResult HitResult;
	FVector StartLocation;
	FVector EndLocation;

	if (PerformGrappleSweep(HitResult, StartLocation, EndLocation))
	{
		bIsGrappling = true;
		GrappleHookLocation = HitResult.ImpactPoint;
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;

		// Draw Debug Stuff
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 300, 12, FColor::Green, false, 2.0f);
		DrawDebugLine(GetWorld(), StartLocation, HitResult.ImpactPoint, FColor::Green, false, 2.0f, 0, 2.0f);
	}
	else
	{
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 2.0f);
	}
}

void AMyCharacter::StopGrappling()
{
	if (bIsGrappling)
	{
		bIsGrappling = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
}

void AMyCharacter::HandleGrapplingMovement(float DeltaTime)
{
	if (bIsGrappling)
	{
		FVector HookDirection = (GrappleHookLocation - GetActorLocation()).GetSafeNormal();

		FVector CurrentVelocity = GetCharacterMovement()->Velocity;

		float CustomGravity = 1500.0f; // Default is 980, but 1500 is better for a heavier and snapier feeling
		CurrentVelocity += FVector(0.f, 0.f, -CustomGravity * DeltaTime);

		// Puts the player in a 2D plane for a pendulum effect
		FVector TangentVelocity = FVector::VectorPlaneProject(CurrentVelocity, HookDirection);

		// To slowly pull the player toward the hook as he swings
		float ReelInSpeed = 300.0f;
		TangentVelocity += HookDirection * ReelInSpeed;


		GetCharacterMovement()->Velocity = TangentVelocity;

		// detache if too close
		float DistanceToHook = FVector::Dist(GetActorLocation(), GrappleHookLocation);
		if (DistanceToHook < 150.0f)
		{
			StopGrappling();
		}
	}
}

void AMyCharacter::UpdateGrappleTarget()
{
	FHitResult HitResult;
	FVector StartLocation;
	FVector EndLocation;

	if (PerformGrappleSweep(HitResult, StartLocation, EndLocation))
	{
		CurrentGrappleTarget = HitResult.GetActor();
	}
	else
	{
		CurrentGrappleTarget = nullptr;
	}
}

#pragma endregion

#pragma region Dash System
void AMyCharacter::Dash()
{
	if (bIsDashing || !bCanDash) return;

	if (bIsGrappling) StopGrappling();

	bCanDash = false;
	bIsDashing = true;

	DashStartLocation = GetActorLocation();
	PreviousDashLocation = DashStartLocation - 2.1f;
	DashDirection = FirstPersonCameraComponent->GetForwardVector();

	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void AMyCharacter::StopDash()
{
	if (bIsDashing)
	{
		bIsDashing = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);

		float ExitDashSpeed = 800.0f;
		GetCharacterMovement()->Velocity = DashDirection * ExitDashSpeed;

		GetWorldTimerManager().SetTimer(DashCooldownTimerHandle, this, &AMyCharacter::ResetDashCooldown, DashCooldownDuration, false);
	}
}

void AMyCharacter::ResetDashCooldown()
{
	bCanDash = true;
}

void AMyCharacter::HandleDash(float DeltaTime)
{
	if (bIsDashing)
	{
		GetCharacterMovement()->Velocity = DashDirection * DashSpeed;

		float TotalDistanceTraveled = FVector::Dist(DashStartLocation, GetActorLocation());
		if (TotalDistanceTraveled >= TargetDashDistance)
		{
			StopDash();
			return;
		}

		FVector CurrentLocation = GetActorLocation();

		float DistanceMovedThisFrame = FVector::Dist(CurrentLocation, PreviousDashLocation);

		if (DistanceMovedThisFrame < 2.0f)
		{
			StopDash();
			return;
		}

		PreviousDashLocation = CurrentLocation;
	}
}
#pragma endregion

// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Test Message!"));