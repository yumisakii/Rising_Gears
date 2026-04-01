#include "MyCharacter.h"

#include "Camera/CameraComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "DrawDebugHelpers.h"

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Construct the camera component
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(RootComponent);
	FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 64.f));

}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Cast the default input component to the Enhanced Input Component
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Bind the action. ETriggerEvent::Started means it fires exactly when the button is pressed down.
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

	float GrappleRadius = 50.0f;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(GrappleRadius);
	
	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		FQuat::Identity,
		ECC_Visibility,
		SphereShape
	);

	if (bHit)
	{
		// Draw a green sphere exactly where the grapple struck the surface
		DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, GrappleRadius, 12, FColor::Green, false, 2.0f);
		// Draw a line to show the path
		DrawDebugLine(GetWorld(), StartLocation, HitResult.ImpactPoint, FColor::Green, false, 2.0f, 0, 2.0f);
	}
	else
	{
		// Draw a red line showing a total miss
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Red, false, 2.0f, 0, 2.0f);
	}
}

