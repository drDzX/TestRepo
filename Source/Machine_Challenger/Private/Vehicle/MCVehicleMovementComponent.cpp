// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCVehicleMovementComponent.h"

#include "MCVehicleInterface.h"
#include "MCVehiclePawnBase.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Core/MCGameState.h"
#include "Utils/MCBlueprintLib.h"

void UMCVehicleMovementComponent::ApplyAntiRollForce()
{
	const auto& VehicleMesh = GetMesh();

	if (!VehicleMesh)
	{
		// Ensure you have a valid vehicle mesh component
		return;
	}

	// Calculate the anti-roll force
	FVector UpVector = VehicleMesh->GetUpVector();
	FVector FwdVector = VehicleMesh->GetForwardVector();
	FVector ImpactNormal = UpVector;
	FHitResult hit;
	if (UMCBlueprintLib::TraceByType(GetWorld(), GetOwner(), VehicleMesh->GetCenterOfMass(), FVector(0.f, 0.f, -TraceCenterOfMassNormalDistance) + VehicleMesh->GetComponentLocation(), hit))
	{
		ImpactNormal = hit.ImpactNormal;
	}

	// Calculate the projection of InputVector onto the ForwardVector
	FVector ProjectionOntoForward = FVector::DotProduct(ImpactNormal, FwdVector.GetSafeNormal()) * FwdVector.GetSafeNormal();

	// Subtract the projection from the original vector to get the projection onto the plane perpendicular to the forward vector
	FVector ProjectionOntoPlane = ImpactNormal - ProjectionOntoForward;
	// Calculate the torque force needed to align the Up vector with the world Up vector
	FVector TorqueForce = FVector::CrossProduct(UpVector, ProjectionOntoPlane) * RollForceMagnitude;

	// Calculate the dot product of the two vectors
	float DotProduct = FVector::DotProduct(UpVector.GetSafeNormal(), ProjectionOntoPlane.GetSafeNormal());

	// Calculate the angle in radians using the arccosine (Acos) function
	float AngleInRadians = FMath::Acos(FMath::Clamp(DotProduct, -1.0f, 1.0f));

	// Convert the angle to degrees
	float AngleInDegrees = FMath::RadiansToDegrees(AngleInRadians);
	if (AngleInDegrees <= 5.f)
	{
		return;
	}

	// Apply the anti-roll force to the vehicle's root component
	VehicleMesh->AddTorqueInDegrees(TorqueForce, NAME_None, true);
	if (AngleInDegrees > 45 || !(!bAreAllWheelesOnGround && IsMovingOnGround()))
	{
		return;
	}

	VehicleMesh->AddForce(FVector::UpVector * PushDownForceMagnitude * -1.f, NAME_None, true);
}

void UMCVehicleMovementComponent::ApplyAntiStuckForce_Implementation(float ForceMultiplier, float SteeringValue)
{
	if (!bIsAntiStuckEnabled)
	{
		return;
	}

	const auto& VehicleMesh = GetMesh();

	if (!VehicleMesh)
	{
		// Ensure you have a valid vehicle mesh component
		return;
	}

	auto MoveForce = VehicleMesh->GetForwardVector() * AntiStuckForce * ForceMultiplier;
	MoveForce = MoveForce.RotateAngleAxis(SteeringValue * -SteeringMaxAngle, FVector::UpVector);

	VehicleMesh->AddForce(MoveForce, NAME_None, true);
}

FString UMCVehicleMovementComponent::GetWheelContactMaterials(int WheelIndex)
{
	if (!PVehicleOutput->Wheels[WheelIndex].PhysMaterial.IsValid())
	{
		return "";
	}
	return PVehicleOutput->Wheels[WheelIndex].PhysMaterial->GetName();
}

void UMCVehicleMovementComponent::UpdateState(float DeltaTime)
{
	Super::UpdateState(DeltaTime);
	if (GetOwner()->HasAuthority() && UMCBlueprintLib::IsGameplayBlocked(GetWorld()))
	{
		if (IsMovingOnGround() && !IsParked())
		{
			SetParked(true);
		}

		return;
	}

	VehicleState.bVehicleInAir = true;

	if (PVehicleOutput)
	{
		bAreAllWheelesOnGround = true;
		for (int WheelIdx = 0; WheelIdx < PVehicleOutput->Wheels.Num(); WheelIdx++)
		{
			if (PVehicleOutput->Wheels[WheelIdx].InContact)
			{
				VehicleState.bVehicleInAir = false;
			}
			else
			{
				bAreAllWheelesOnGround = false;
			}
		}
	}

	if (bIsAntiRollEnabled && !bAreAllWheelesOnGround && AntiRollAfterTimeInAir >= InAirTime)
	{
		ApplyAntiRollForce();
	}
	else
	{
		if (InAirTime > 0.f)
		{
			InAirTime = 0.f;
		}
	}

	if (!bIsAngularDampingAllowedInAir)
	{
		return;
	}

	if (VehicleState.bVehicleInAir)
	{
		GetMesh()->SetAngularDamping(AngularDampingInAir);
		DelayToResetDamping = AngularDampingInAirResetDelay;
		InAirTime += DeltaTime;
	}
	else
	{
		if (DelayToResetDamping <= 0.f)
		{
			GetMesh()->SetAngularDamping(0.f);
		}
		else
		{
			DelayToResetDamping -= DeltaTime;
		}
	}

	CheckAntiStuck(DeltaTime);

	DetectDrift();
}

void UMCVehicleMovementComponent::CheckAntiStuck(float dt)
{
	if (GetThrottleInput() > 0 && GetBrakeInput() > 0 || GetHandbrakeInput() || GetMesh()->GetComponentVelocity().Length() >= 200.f)
	{
		bCanExecuteAntiStuck = false;
		AntiStuckTime = AntistuckDelayTimeSec;
		bIsForwardInputTriggered = false;
		return;
	}

	if (GetThrottleInput() > 0 || GetBrakeInput() > 0)
	{
		if (!bIsForwardInputTriggered)
		{
			bIsForwardInputTriggered = true;
			AntiStuckTime = AntistuckDelayTimeSec;
		}

		if (bCanExecuteAntiStuck)
		{
			const auto& Multiplier = GetThrottleInput() > 0.f ? GetThrottleInput() : GetBrakeInput() * -1.f;
			ApplyAntiStuckForce(Multiplier * dt * 100.f, GetSteeringInput());
			if (GetMesh()->GetComponentVelocity().Length() >= 500.f)
			{
				bCanExecuteAntiStuck = false;
				AntiStuckTime = AntistuckDelayTimeSec;
				bIsForwardInputTriggered = false;
			}

			return;
		}

		if (bIsForwardInputTriggered)
		{
			AntiStuckTime -= dt;
		}

		if (AntiStuckTime <= 0.f)
		{
			bCanExecuteAntiStuck = true;
		}
	}
	else
	{
		bCanExecuteAntiStuck = false;
		AntiStuckTime = AntistuckDelayTimeSec;
		bIsForwardInputTriggered = false;
	}
}

void UMCVehicleMovementComponent::DetectDrift()
{
	if (!GetOwner()->Implements<UMCVehicleInterface>())
	{
		return;
	}

	for (int WheelIdx = 0; WheelIdx < PVehicleOutput->Wheels.Num(); WheelIdx++)
	{
		const auto& wheel = PVehicleOutput->Wheels[WheelIdx];
		if (wheel.InContact && (wheel.bIsSkidding || wheel.bIsSlipping))
		{
			FDriftInfo drift;
			drift.WheelIndexId = WheelIdx;
			drift.SkidMagnitude = wheel.SkidMagnitude;
			drift.SlipMagnitude = wheel.SlipMagnitude;
			IMCVehicleInterface::Execute_DriftEvent(GetOwner(), drift);
			WheelDrifting.Emplace(WheelIdx, true);
		}
		else
		{
			if (WheelDrifting.Contains(WheelIdx) && WheelDrifting[WheelIdx])
			{
				FDriftInfo drift;
				drift.WheelIndexId = WheelIdx;
				IMCVehicleInterface::Execute_DriftEvent(GetOwner(), drift);
				WheelDrifting[WheelIdx] = false;
			}
		}
	}
}

void UMCVehicleMovementComponent::SpawnDriftVFX_Implementation(bool isEnabled, int WheelIdx, USceneComponent* Attachment)
{
	if (!isEnabled)
	{
		if (DriftVfx.Contains(WheelIdx) && DriftVfx[WheelIdx])
		{
			DriftVfx[WheelIdx]->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			DriftVfx[WheelIdx] = nullptr;
		}

		return;
	}

	if (DriftVfx.Contains(WheelIdx) && DriftVfx[WheelIdx])
	{
		return;
	}

	DriftVfx.Emplace(WheelIdx, UNiagaraFunctionLibrary::SpawnSystemAttached(DriftVFXSystem, Attachment, NAME_None, FVector::Zero(), FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true));
}

bool UMCVehicleMovementComponent::IsMovingOnGround() const
{
	return !VehicleState.bVehicleInAir;
}

void UMCVehicleMovementComponent::BeginPlay()
{
	Super::BeginPlay();
}
