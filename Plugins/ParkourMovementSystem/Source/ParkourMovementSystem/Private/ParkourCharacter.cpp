// Fill out your copyright notice in the Description page of Project Settings.


#include "ParkourCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Components/ArrowComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
AParkourCharacter::AParkourCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

  // Set size for collision capsule
  GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

  // set our turn rates for input
  BaseTurnRate = 45.f;
  BaseLookUpRate = 45.f;

  // Don't rotate when the controller rotates. Let that just affect the camera.
  bUseControllerRotationPitch = false;
  bUseControllerRotationYaw = false;
  bUseControllerRotationRoll = false;

  // Configure character movement
  GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
  GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
  GetCharacterMovement()->JumpZVelocity = 600.f;
  GetCharacterMovement()->AirControl = 0.2f;

  // Create a camera boom (pulls in towards the player if there is a collision)
  CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(RootComponent);
  CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
  CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

  // Create a follow camera
  FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
  FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
  FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

  FollowCamera->bUsePawnControlRotation = false;

  // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
  // are set in the derived blueprint asset named ParkourCharacterBP (to avoid direct content references in C++)

  // Get Jump Timeline curve
  static ConstructorHelpers::FObjectFinder<UCurveFloat> Curve(TEXT("/ParkourMovementSystem/Character/JumpCurve"));
  check(Curve.Succeeded());

  FloatCurve = Curve.Object;
}

void AParkourCharacter::BeginPlay()
{
  Super::BeginPlay();

  GetCharacterMovement()->SetPlaneConstraintEnabled(true);

  DefaultGravityScale = GetCharacterMovement()->GravityScale;
}


// Called to bind functionality to input
void AParkourCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

  // Set up gameplay key bindings
  check(PlayerInputComponent);
  PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AParkourCharacter::ParkourJump);
  PlayerInputComponent->BindAction("Jump", IE_Released, this, &AParkourCharacter::ParkourJumpStop);

  PlayerInputComponent->BindAxis("MoveForward", this, &AParkourCharacter::MoveForward);
  PlayerInputComponent->BindAxis("MoveRight", this, &AParkourCharacter::MoveRight);

  // We have 2 versions of the rotation bindings to handle different kinds of devices differently
  // "turn" handles devices that provide an absolute delta, such as a mouse.
  // "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
  PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
  PlayerInputComponent->BindAxis("TurnRate", this, &AParkourCharacter::TurnAtRate);
  PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
  PlayerInputComponent->BindAxis("LookUpRate", this, &AParkourCharacter::LookUpAtRate);
}

void AParkourCharacter::TurnAtRate(float Rate)
{
  // calculate delta for this frame from the rate information
  AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AParkourCharacter::LookUpAtRate(float Rate)
{
  // calculate delta for this frame from the rate information
  AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AParkourCharacter::MoveForward(float Value)
{
  if ((Controller != NULL) && (Value != 0.0f))
  {
    // find out which way is forward
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    // get forward vector
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    AddMovementInput(Direction, Value);
  }
}

void AParkourCharacter::MoveRight(float Value)
{
  if ((Controller != NULL) && (Value != 0.0f))
  {
    // find out which way is right
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    // get right vector 
    const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
    // add movement in that direction
    AddMovementInput(Direction, Value);
  }
}

void AParkourCharacter::ParkourJump()
{
  Jump();

  FVector TraceStart = GetArrowComponent()->GetComponentLocation();

  FVector TraceEnd = GetArrowComponent()->GetForwardVector() * 50 + TraceStart;

  FHitResult OutHit;
  FCollisionQueryParams CollisionParams;

  DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 1, 0, 1);

  if (GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_Visibility, CollisionParams))
  {
    if (OutHit.bBlockingHit)
    {
      OnLedgeClimb();
      bJumpingOverLedge = true;
      GetCharacterMovement()->SetPlaneConstraintNormal(FVector(1, 1, 0));
      GetCharacterMovement()->GravityScale = 0;

      FOnTimelineFloat LedgeTimelineCallback;
      FOnTimelineEventStatic LedgeTimelineFinishedCallback;

      if (FloatCurve)
      {
        LedgeTimelineCallback.BindUFunction(this, FName("LedgeTimelineCallback"));
        LedgeTimelineFinishedCallback.BindUFunction(this, FName("LedgeTimelineFinishedCallback"));
        LedgeTimeline.SetTimelineFinishedFunc(LedgeTimelineFinishedCallback);
        LedgeTimeline.AddInterpFloat(FloatCurve, LedgeTimelineCallback);
        LedgeTimeline.SetLooping(false);
        LedgeTimeline.SetPlayRate(1.0f);
        LedgeTimeline.PlayFromStart();
      }
    }
  }
}

void AParkourCharacter::Tick(float DeltaSeconds)
{
  Super::Tick(DeltaSeconds);

  LedgeTimeline.TickTimeline(DeltaSeconds);
}

void AParkourCharacter::ParkourJumpStop()
{
  LedgeTimeline.Stop();
  GetCharacterMovement()->SetPlaneConstraintNormal(FVector(0, 0, 0));
  GetCharacterMovement()->GravityScale = GetDefaultGravityScale();
  bJumpingOverLedge = false;
  OnLedgeClimbStop();
}

// Ledge Climbing functions
bool AParkourCharacter::IsPlayerOnTopOfLedge()
{
  FVector TraceStart = GetMesh()->GetSocketLocation(FName("ball_r"));

  FVector TraceEnd = GetMesh()->GetSocketTransform(FName("ball_r")).GetRotation().GetForwardVector() * 50 + TraceStart;

  FHitResult OutHit;
  FCollisionQueryParams CollisionParams;

  if (GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECC_Visibility, CollisionParams))
  {
    if (!OutHit.bBlockingHit)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

void AParkourCharacter::LedgeTimelineCallback(float val)
{
  LaunchCharacter(FVector(0, 0, 250), false, true);
  
  if (IsPlayerOnTopOfLedge())
  {
    LedgeTimeline.Stop();
    ParkourJumpStop();
  }
}

void AParkourCharacter::LedgeTimelineFinishedCallback()
{
  ParkourJumpStop();
}

const float AParkourCharacter::GetDefaultGravityScale()
{
  return DefaultGravityScale;
}

const bool AParkourCharacter::GetJumpingOverLedge()
{
  return bJumpingOverLedge;
}
