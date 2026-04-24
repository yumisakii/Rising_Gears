#include "MyCharacter.h"

#include "Camera/CameraComponent.h"

#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "DrawDebugHelpers.h"
#include "CableComponent.h"
#include <Kismet/GameplayStatics.h>

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

	UpdateCameraFOV(DeltaTime);
	UpdateJumpValues(DeltaTime);

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

void AMyCharacter::UpdateCameraFOV(float DeltaTime)
{
	float CurrentSpeed = GetCharacterMovement()->Velocity.Size();

	FVector2D SpeedRange(600.0f, 3000.0f);

	FVector2D FOVRange(90.0f, 160.0f);

	float TargetFOV = FMath::GetMappedRangeValueClamped(SpeedRange, FOVRange, CurrentSpeed);
	float CurrentFOV = FirstPersonCameraComponent->FieldOfView;
	float InterpSpeed = 10.0f;

	float SmoothedFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, InterpSpeed);
	FirstPersonCameraComponent->SetFieldOfView(SmoothedFOV);
}

#pragma region Jump Override System
void AMyCharacter::Jump()
{
	if (bIsGrappling || bIsDashing)
	{
		StopGrappling();
		StopDash();

		FVector ForwardBoost = FirstPersonCameraComponent->GetForwardVector();
		ForwardBoost.Z = 0.f;
		ForwardBoost = ForwardBoost.GetSafeNormal();
		ForwardBoost *= 750;

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
		if (GetCharacterMovement()->IsFalling())
		{
			JumpBufferTimeLeft = 0.15f;
		}
		Super::Jump();
	}
}

void AMyCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (JumpBufferTimeLeft > 0.0f)
	{
		JumpBufferTimeLeft = 0.0f;

		float StandardJumpForce = GetCharacterMovement()->JumpZVelocity;
		LaunchCharacter(FVector(0.f, 0.f, StandardJumpForce), false, true);
	}
}

void AMyCharacter::UpdateJumpValues(float DeltaTime)
{
	if (GetCharacterMovement()->IsFalling()) TimeSinceLeftGround += DeltaTime;
	else TimeSinceLeftGround = 0.f;

	if (JumpBufferTimeLeft > 0.0f)
	{
		JumpBufferTimeLeft -= DeltaTime;
	}
}
#pragma endregion

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
	
	if (bIsGrappling) return;

	FHitResult HitResult;
	FVector StartLocation;
	FVector EndLocation;

	if (PerformGrappleSweep(HitResult, StartLocation, EndLocation))
	{
		
		UGameplayStatics::PlaySound2D(GetWorld(), GrappleShootSound);

		bIsGrappling = true;
		GrappleHookLocation = HitResult.ImpactPoint;
		CurrentRopeLength = FVector::Distance(GetActorLocation(), GrappleHookLocation);

		// visuals
		if (GrappleRopeClass)
		{
			CurrentGrappleRope = GetWorld()->SpawnActor<AActor>(GrappleRopeClass, GetActorLocation(), FRotator::ZeroRotator);
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		FVector ForwardDirection = FirstPersonCameraComponent->GetForwardVector();
		
		float InitialGrappleBoost = 1200.0f; 
		
		FVector VerticalPop = FVector(0.f, 0.f, 400.0f);
		GetCharacterMovement()->Velocity += (ForwardDirection * InitialGrappleBoost) + VerticalPop;

		CurrentForwardMomentumForce = BaseForwardMomentumForce;

		if (GrappleRopeClass)
		{
			CurrentGrappleRope->Destroy();
			CurrentGrappleRope = nullptr;

			CurrentGrappleRope = GetWorld()->SpawnActor<AActor>(GrappleRopeClass, GetActorLocation(), FRotator::ZeroRotator);
		}
	}
}

void AMyCharacter::StopGrappling()
{
	if (bIsGrappling)
	{
		bIsGrappling = false;
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);

		//visuals
		if (CurrentGrappleRope)
		{
			CurrentGrappleRope->Destroy();
			CurrentGrappleRope = nullptr;
		}
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

		// Add forward momentum
		if (CurrentForwardMomentumForce > 0)
		{
			FVector LookDirection = FirstPersonCameraComponent->GetForwardVector();
			FVector SwingForwardDirection = FVector::VectorPlaneProject(LookDirection, HookDirection).GetSafeNormal();
			TangentVelocity += SwingForwardDirection * (CurrentForwardMomentumForce * DeltaTime);

			CurrentForwardMomentumForce -= BaseForwardMomentumForce * DeltaTime; // Decay over 1 second
		}


		// To slowly pull the player toward the hook as he swings
		float ReelInSpeed = 600.0f;
		float DistanceToHook = FVector::Dist(GetActorLocation(), GrappleHookLocation);
		if (DistanceToHook < (CurrentRopeLength - 200.f)) ReelInSpeed = 0.f;

		TangentVelocity += HookDirection * ReelInSpeed;

		GetCharacterMovement()->Velocity = TangentVelocity;
		OnUpdateGrappleRope(GetActorLocation(), GrappleHookLocation);
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

	UGameplayStatics::PlaySound2D(GetWorld(), DashSound);

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