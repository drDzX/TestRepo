// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/SuspensionComponent.h"
#include "MCSuspensionComponent.generated.h"

USTRUCT(BlueprintType)
struct FSpeedToParams
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Custom Data")
	float Speed = 0;
	UPROPERTY(EditAnywhere, Category = "Custom Data")
	float MaxLateralForce = 0;
};

/**
 *
 */
UCLASS(ClassGroup = (RTune), meta = (BlueprintSpawnableComponent))
class UMCSuspensionComponent : public USuspensionComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMCSuspensionComponent();

private:
	UFUNCTION()
	float CalculateMaxLateralForce(float CurrentSpeed);
	UFUNCTION()
	void GetSideSlipAngle();
	UFUNCTION()
	void CalculateBurnoutSpeedCoef();
	UFUNCTION()
	void GetLongitudinalSlip(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void Explode();

public:
	virtual void UpdateTick(float DeltaTime) override;
	virtual void PostLoad() override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MC Params")
	class ARTuneVehicle* OwningCar;

	UPROPERTY(EditDefaultsOnly, Category = "MC Params")
	TArray<FSpeedToParams> MaxLateralForceSegments;
	UPROPERTY(EditDefaultsOnly, Category = "MC Params")
	float SpeedToSuspendBurnout = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float LongitudinalSlip = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SideSlipAngle = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float BurnoutSpeedCoef = 1.f;
	float BurnoutRotationDefault = 0.f;
};
