// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Character.h"
#include "ParkourCharacter.generated.h"

UCLASS()
class PARKOURMOVEMENTSYSTEM_API AParkourCharacter : public ACharacter
{
	GENERATED_BODY()

  /** Camera boom positioning the camera behind the character */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
  class USpringArmComponent* CameraBoom;

  /** Follow camera */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
  class UCameraComponent* FollowCamera;

public:
  AParkourCharacter();

  /** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
  float BaseTurnRate;

  /** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
  float BaseLookUpRate;

  UFUNCTION(BlueprintCallable, Category = "Movement Functions")
  void ParkourJump();
  UFUNCTION(BlueprintCallable, Category = "Movement Functions")
  void ParkourJumpStop();
  /* Returns the default gravity scale */
  UFUNCTION(BlueprintPure, Category = "Movement Functions")
  const float GetDefaultGravityScale();
  /* Reads the value of bJumpingOverLedge */
  UFUNCTION(BlueprintPure, Category = "Movement Functions")
  const bool GetJumpingOverLedge();
  /* Called when player climbs over a ledge */
  UFUNCTION(BlueprintImplementableEvent, Category = "Movement Events")
  void OnLedgeClimb();
  /* Called when player is done climbing ledge */
  UFUNCTION(BlueprintImplementableEvent, Category = "Movement Events")
  void OnLedgeClimbStop();
  /* Checks to see if player is on top of a ledge the top of a ledge */
  UFUNCTION(BlueprintCallable, Category = "Movement Events")
  bool IsPlayerOnTopOfLedge();

  virtual void Tick(float DeltaSeconds) override;

protected:

  /** Called for forwards/backward input */
  void MoveForward(float Value);

  /** Called for side to side input */
  void MoveRight(float Value);

  /**
   * Called via input to turn at a given rate.
   * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
   */
  void TurnAtRate(float Rate);

  /**
   * Called via input to turn look up/down at a given rate.
   * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
   */
  void LookUpAtRate(float Rate);

  virtual void BeginPlay() override;

  FTimeline LedgeTimeline;

  UPROPERTY()
  UCurveFloat* FloatCurve;

  UFUNCTION()
  void LedgeTimelineCallback(float val);

  UFUNCTION()
  void LedgeTimelineFinishedCallback();

protected:
  // APawn interface
  virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
  // End of APawn interface

public:
  /** Returns CameraBoom subobject **/
  FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
  /** Returns FollowCamera subobject **/
  FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

private:

  float DefaultGravityScale;

  bool bJumpingOverLedge;

  bool bReachedTopOfLedge;
};
