// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "MCSpringArmComponent.generated.h"

/**
 *
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent))
class UMCSpringArmComponent : public USpringArmComponent
{
	GENERATED_BODY()

protected:
	virtual void UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void AsyncTick(float DeltaTime);

private:
	void CheckYawCameraLag();
	void ApplyCameraDistanceLag(float DeltaTime);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsIgnoringSelf = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0", UIMin = "-89.0"))
	float CameraLagDisableAngleYaw = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DefaultTargetArmLength = 600.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float AccelerationThreshold = 5.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float DecelerationThreshold = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float UpshiftMultiplier = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsAccelerationClamped = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bIsDecelerationClamped = false;

	float PreviousSpeedKPH;
};
