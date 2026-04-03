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
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	// Handle Grappling
	if (bIsGrappling)
	{
		FVector HookDirection = (GrappleHookLocation - GetActorLocation()).GetSafeNormal();
	
		FVector CurrentVelocity = GetCharacterMovement()->Velocity;

		float CustomGravity = 1500.0f; // Default is 980, but 1500 is better for a heavier and snapier feeling
		CurrentVelocity += FVector(0.f, 0.f, -CustomGravity * DeltaTime);

		// Puts the player in a 2D plane for a pendulum effect
		FVector TangentVelocity = FVector::VectorPlaneProject(CurrentVelocity, HookDirection);

		// To slowly pull the player toward the hook as he swings
		float ReelInSpeed = 400.0f;
		TangentVelocity += HookDirection * ReelInSpeed;

		
		GetCharacterMovement()->Velocity = TangentVelocity;

		// detache if too far away
		float DistanceToHook = FVector::Dist(GetActorLocation(), GrappleHookLocation);
		if (DistanceToHook < 150.0f)
		{
			StopGrappling();
		}
	}

}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(ShootGrapplingHookAction, ETriggerEvent::Started, this, &AMyCharacter::ShootGrapplingHook);
	}
}

void AMyCharacter::ShootGrapplingHook()
{
	


	FVector StartLocation = FirstPersonCameraComponent->GetComponentLocation();
	FVector ForwardDirection = FirstPersonCameraComponent->GetForwardVector();

	float MaxGrappleDistance = 5000.0f;
	FVector EndLocation = StartLocation + (ForwardDirection * MaxGrappleDistance);

	FHitResult HitResult;
	float GrappleRadius = 200.0f;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(GrappleRadius);
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_GameTraceChannel2,
		SphereShape,
		QueryParams
	);

	if (bHit)
	{
		bIsGrappling = true;
		GrappleHookLocation = HitResult.ImpactPoint;
		GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		GetCharacterMovement()->Velocity = FVector::ZeroVector;

		// Draw Debug Stuff
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, GrappleRadius, 12, FColor::Green, false, 2.0f);
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

void AMyCharacter::Jump()
{	
	if (bIsGrappling)
	{
		StopGrappling();

		FVector ForwardBoost = FirstPersonCameraComponent->GetForwardVector() * 1500.0f;
		FVector UpwardBoost = FVector(0.f, 0.f, 1000.0f);
		FVector TotalLaunchVelocity = ForwardBoost + UpwardBoost;

		LaunchCharacter(TotalLaunchVelocity, true, true);
	}
	else
	{
		Super::Jump();
	}
}

// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Test Message!"));