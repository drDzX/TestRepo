// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCSuspensionComponent.h"

#include "MCVehicleSlot.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/KismetMathLibrary.h"
#include "Vehicle/RTuneVehicle.h"

UMCSuspensionComponent::UMCSuspensionComponent()
{
	OwningCar = Cast<ARTuneVehicle>(GetOwner());
}

float UMCSuspensionComponent::CalculateMaxLateralForce(float CurrentSpeed)
{
	// Ensure we have at least two segments to interpolate between
	if (MaxLateralForceSegments.Num() < 2)
		return 0.0f;

	// Get the MinForce and MaxForce from the first and last segments
	float MinForce = MaxLateralForceSegments[0].MaxLateralForce;
	float MaxForce = MaxLateralForceSegments.Last().MaxLateralForce;

	// Initialize the MaxLateralForce to zero
	float LocMaxLateralForce = 0.0f;

	// Check if the current speed is below or equal to the minimum speed
	if (CurrentSpeed <= MaxLateralForceSegments[0].Speed)
	{
		return MinForce; // Clamp to MinForce
	}

	// Loop through segments to find the correct interval
	for (int32 i = 0; i < MaxLateralForceSegments.Num() - 1; ++i)
	{
		float SpeedStart = MaxLateralForceSegments[i].Speed;
		float SpeedEnd = MaxLateralForceSegments[i + 1].Speed;
		float ForceStart = MaxLateralForceSegments[i].MaxLateralForce;
		float ForceEnd = MaxLateralForceSegments[i + 1].MaxLateralForce;

		// Check if current speed falls within this segment
		if (CurrentSpeed >= SpeedStart && CurrentSpeed <= SpeedEnd)
		{
			// Calculate the proportion of CurrentSpeed within the segment range
			float Alpha = (CurrentSpeed - SpeedStart) / (SpeedEnd - SpeedStart);

			// Linearly interpolate between ForceStart and ForceEnd
			LocMaxLateralForce = FMath::Lerp(ForceStart, ForceEnd, Alpha);
			return FMath::Clamp(LocMaxLateralForce, MinForce, MaxForce); // Return clamped value
		}
	}

	// If the speed is above the last segment, return MaxForce
	if (CurrentSpeed > MaxLateralForceSegments.Last().Speed)
	{
		return MaxForce; // Clamp to MaxForce
	}

	// If no conditions are met, return the last segment's MaxLateralForce
	return MaxLateralForceSegments.Last().MaxLateralForce;
}

void UMCSuspensionComponent::GetSideSlipAngle()
{
	SideSlipAngle = FMath::RadiansToDegrees(FMath::Atan(UKismetMathLibrary::SafeDivide(LinearVelocity.Y, LinearVelocity.X)));
}

void UMCSuspensionComponent::CalculateBurnoutSpeedCoef()
{
	if (SpeedToSuspendBurnout <= 0.f || !OwningCar)
	{
		if (BurnoutSpeedCoef != 1.f)
		{
			BurnoutSpeedCoef = 1.f;
			BurnoutRotation = BurnoutRotationDefault;
		}
		return;
	}

	const auto& currentSpeed = OwningCar->GetSpeedKPH();
	if (currentSpeed > SpeedToSuspendBurnout)
	{
		BurnoutSpeedCoef = 0; // No burnout if speed is too high
	}
	else
	{
		// Linearly interpolate from 1 to 0 based on the speed
		BurnoutSpeedCoef = 1 - (currentSpeed / SpeedToSuspendBurnout);
		const auto& inputValue = OwningCar->GetThrottleInput() > OwningCar->GetBrakesInput() ? OwningCar->GetThrottleInput() : OwningCar->GetBrakesInput();
		BurnoutSpeedCoef *= inputValue;
		// Ensure the coefficient stays between 0 and 1
		BurnoutSpeedCoef = FMath::Clamp(BurnoutSpeedCoef, 0.0f, 1.0f);
	}

	BurnoutRotation = BurnoutSpeedCoef * BurnoutRotationDefault;
}

void UMCSuspensionComponent::GetLongitudinalSlip(float DeltaTime)
{
	//If the frame rate is higher than 60, scale the rotation accordingly to compensate these extra rotations added every frame
	float timeScalar = 60.f / FMath::Pow(DeltaTime, -1.f);
	int d = 1;
	if (bRotateWheel)
	{
		d = -1;
	}

	// Calculate the actual angular velocity (how fast the wheel is actually rotating)
	float actualAngularVelocity = (LinearVelocity.X * 100.f) / WheelRadius * -1.f;
	actualAngularVelocity *= timeScalar * WheelAngularVelocityMultiplier * HandbrakeVelocityMultiplier * d;
	const auto& animAngularVelocity = WheelLocalRotation.Pitch;

	// Check for zero to avoid division by zero
	if (FMath::IsNearlyZero(actualAngularVelocity, 0.01f))
	{
		LongitudinalSlip = 0.0f; // No movement means no slip
		return;
	}

	const auto& value = (actualAngularVelocity - animAngularVelocity) / actualAngularVelocity * -1.f;
	if (FMath::IsNearlyZero(value, 0.10f))
	{
		LongitudinalSlip = 0.f;
		return;
	}
	// Calculate longitudinal slip as the difference between actual and animated angular velocities
	LongitudinalSlip = FMath::Clamp(value, -1.f, 1.f);
}

void UMCSuspensionComponent::Explode()
{
	if (!WheelMeshComponent || !GetWorld())
	{
		return;
	}
	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (const auto& Wheel = GetWorld()->SpawnActor<AMCVehicleSlot>(AMCVehicleSlot::StaticClass(), GetComponentLocation(), WheelMeshComponent->GetComponentRotation(), SpawnParams))
	{
		Wheel->StaticMesh = WheelMesh;
		Wheel->CreateMeshes();
		Wheel->Explode(GetOwner()->GetActorLocation());

		Wheel->SetLifeSpan(10.f);
	}

	WheelMeshComponent->DestroyComponent();
	DestroyComponent();
}

void UMCSuspensionComponent::UpdateTick(float DeltaTime)
{
	if (OwningCar && !MaxLateralForceSegments.IsEmpty())
	{
		const auto& CurrentSpeed = OwningCar->GetSpeedKPH();
		const auto& LocMaxLateralForce = CalculateMaxLateralForce(CurrentSpeed);

		// Set the calculated MaxLateralForce
		// Replace SetMaxLateralForce with the actual method in your class
		MaxLateralForce = LocMaxLateralForce;
	}

	GetLongitudinalSlip(DeltaTime);
	Super::UpdateTick(DeltaTime);
	GetSideSlipAngle();
	CalculateBurnoutSpeedCoef();
}

void UMCSuspensionComponent::PostLoad()
{
	Super::PostLoad();
	OwningCar = Cast<ARTuneVehicle>(GetOwner());
}

void UMCSuspensionComponent::BeginPlay()
{
	Super::BeginPlay();
	BurnoutRotationDefault = BurnoutRotation;
}
