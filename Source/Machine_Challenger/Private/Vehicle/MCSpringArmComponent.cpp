// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCSpringArmComponent.h"

#include "AsyncTickFunctions.h"
#include "MCVehiclePawnBase.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Math/UnrealMathUtility.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UMCSpringArmComponent::UpdateDesiredArmLocation(bool bDoTrace, bool bDoLocationLag, bool bDoRotationLag, float DeltaTime)
{
	FRotator DesiredRot = GetTargetRotation();

	// If our viewtarget is simulating using physics, we may need to clamp deltatime
	if (bClampToMaxPhysicsDeltaTime)
	{
		// Use the same max timestep cap as the physics system to avoid camera jitter when the viewtarget simulates less time than the camera
		DeltaTime = FMath::Min(DeltaTime, UPhysicsSettings::Get()->MaxPhysicsDeltaTime);
	}

	// Apply 'lag' to rotation if desired
	if (bDoRotationLag)
	{
		if (bUseCameraLagSubstepping && DeltaTime > CameraLagMaxTimeStep && CameraRotationLagSpeed > 0.f)
		{
			const FRotator ArmRotStep = (DesiredRot - PreviousDesiredRot).GetNormalized() * (1.f / DeltaTime);
			FRotator LerpTarget = PreviousDesiredRot;
			float RemainingTime = DeltaTime;
			while (RemainingTime > UE_KINDA_SMALL_NUMBER)
			{
				const float LerpAmount = FMath::Min(CameraLagMaxTimeStep, RemainingTime);
				LerpTarget += ArmRotStep * LerpAmount;
				RemainingTime -= LerpAmount;

				DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(LerpTarget), LerpAmount, CameraRotationLagSpeed));
				PreviousDesiredRot = DesiredRot;
			}
		}
		else
		{
			DesiredRot = FRotator(FMath::QInterpTo(FQuat(PreviousDesiredRot), FQuat(DesiredRot), DeltaTime, CameraRotationLagSpeed));
		}
	}
	PreviousDesiredRot = DesiredRot;

	// Get the spring arm 'origin', the target we want to look at
	FVector ArmOrigin = GetComponentLocation() + TargetOffset;
	// We lag the target, not the actual camera position, so rotating the camera around does not have lag
	FVector DesiredLoc = ArmOrigin;
	PreviousArmOrigin = ArmOrigin;
	PreviousDesiredLoc = DesiredLoc;

	// Now offset camera position back along our rotation
	DesiredLoc -= DesiredRot.Vector() * TargetArmLength;
	// Add socket offset in local space
	DesiredLoc += FRotationMatrix(DesiredRot).TransformVector(SocketOffset);

	// Do a sweep to ensure we are not penetrating the world
	FVector ResultLoc;
	if (bDoTrace && (TargetArmLength != 0.0f))
	{
		bIsCameraFixed = true;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, bIsIgnoringSelf ? GetOwner() : nullptr);

		FHitResult Result;
		GetWorld()->SweepSingleByChannel(Result, ArmOrigin, DesiredLoc, FQuat::Identity, ProbeChannel, FCollisionShape::MakeSphere(ProbeSize), QueryParams);

		UnfixedCameraPosition = DesiredLoc;

		ResultLoc = BlendLocations(DesiredLoc, Result.Location, Result.bBlockingHit, DeltaTime);

		if (ResultLoc == DesiredLoc)
		{
			bIsCameraFixed = false;
		}
	}
	else
	{
		ResultLoc = DesiredLoc;
		bIsCameraFixed = false;
		UnfixedCameraPosition = ResultLoc;
	}

	// Form a transform for new world transform for camera
	FTransform WorldCamTM(DesiredRot, ResultLoc);
	// Convert to relative to component
	FTransform RelCamTM = WorldCamTM.GetRelativeTransform(GetComponentTransform());

	// Update socket location/rotation
	RelativeSocketLocation = RelCamTM.GetLocation();
	RelativeSocketRotation = RelCamTM.GetRotation();

	UpdateChildTransforms();
}

void UMCSpringArmComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UMCSpringArmComponent::AsyncTick(float DeltaTime)
{
	CheckYawCameraLag();
	ApplyCameraDistanceLag(DeltaTime);
}

void UMCSpringArmComponent::CheckYawCameraLag()
{
	if (CameraLagDisableAngleYaw <= 0.f)
	{
		return;
	}

	const auto& mcCar = Cast<AMCVehiclePawnBase>(GetOwner());
	if (!mcCar)
	{
		return;
	}

	const auto& relativeRotation = mcCar->GetFollowCamera()->GetComponentRotation().Yaw - mcCar->GetActorRotation().Yaw;
	if (FMath::Abs(relativeRotation) >= CameraLagDisableAngleYaw)
	{
		if (bEnableCameraLag)
		{
			bEnableCameraLag = false;
		}

		return;
	}

	if (bEnableCameraLag)
	{
		return;
	}

	bEnableCameraLag = true;
}

void UMCSpringArmComponent::ApplyCameraDistanceLag(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	// Get the owner's current position and speed
	const auto& MCCar = Cast<AMCVehiclePawnBase>(GetOwner());
	if (!MCCar || !MCCar->IsLocallyControlled())
	{
		return;
	}

	// Get the current speed in KPH
	float CurrentSpeedKPH = MCCar->GetSpeedKPH();

	// Calculate acceleration (change in speed over time)
	float SpeedDeltaKPH = CurrentSpeedKPH - PreviousSpeedKPH;

	if (SpeedDeltaKPH == 0.f)
	{
		return;
	}

	// Calculate the target arm length
	float DesiredLength = DefaultTargetArmLength;
	if (CurrentSpeedKPH < 5 || !bEnableCameraLag)
	{
		TargetArmLength = FMath::FInterpTo(TargetArmLength, DesiredLength, DeltaTime, CameraLagSpeed);
		// Store the current speed for the next frame
		PreviousSpeedKPH = CurrentSpeedKPH;
		return;
	}

	float AccelerationRateKPH = SpeedDeltaKPH / DeltaTime; // Rate of change in KPH/s

	float CameraLagSpeedLocal = CameraLagSpeed;
	if (AccelerationRateKPH > 0)
	{
		DesiredLength = DefaultTargetArmLength + CameraLagMaxDistance;
		auto value = AccelerationRateKPH / AccelerationThreshold;
		CameraLagSpeedLocal *= bIsAccelerationClamped ? FMath::Clamp(value, 0.f, 1.f) : value;
	}
	else
	{
		DesiredLength = DefaultTargetArmLength;
		auto value = std::abs(AccelerationRateKPH / DecelerationThreshold);
		bool IsClamped = bIsDecelerationClamped;
		if (MCCar->IsGearboxUpshifting() && UpshiftMultiplier != 1.f)
		{
			value *= UpshiftMultiplier;
			IsClamped = false;
		}
		CameraLagSpeedLocal *= IsClamped ? FMath::Clamp(value, 0.f, 1.f) : value;
	}

	// Clamp the length between the default and max distance
	DesiredLength = FMath::Clamp(DesiredLength, DefaultTargetArmLength, DefaultTargetArmLength + CameraLagMaxDistance);

	// Smoothly interpolate the actual length to the desired length
	TargetArmLength = FMath::FInterpTo(TargetArmLength, DesiredLength, DeltaTime, CameraLagSpeedLocal);

	//if (GEngine)
	//{
	//	const FString DebugMessage = FString::Printf(TEXT("DesiredLength: %.2f, TargetArmLength: %.2f"), DesiredLength, TargetArmLength);
	//	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, DebugMessage);
	//}

	// Store the current speed for the next frame
	PreviousSpeedKPH = CurrentSpeedKPH;
}
