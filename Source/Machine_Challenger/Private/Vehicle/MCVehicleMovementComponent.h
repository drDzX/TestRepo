// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "MCVehicleMovementComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
struct FWheelsOutput;

USTRUCT(BlueprintType)
struct FDriftInfo
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int WheelIndexId = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SkidMagnitude = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SlipMagnitude = 0.f;
};

/**
 *
 */
UCLASS()
class UMCVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	void ApplyAntiRollForce();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ApplyAntiStuckForce(float ForceMultiplier = 1.f, float SteeringValue = 0.f);

	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable)
	void SpawnDriftVFX(bool isEnabled, int WheelIdx, USceneComponent* Attachment);

	UFUNCTION(BlueprintCallable)
	FString GetWheelContactMaterials(int WheelIndex);

protected:
	virtual void UpdateState(float DeltaTime) override;
	void CheckAntiStuck(float dt);

private:
	void DetectDrift();

public:
	virtual bool IsMovingOnGround() const override;
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti roll")
	bool bIsAntiRollEnabled = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti roll")
	float RollForceMagnitude = 500.0f; // Adjust this value based on your needs

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti roll")
	float PushDownForceMagnitude = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti roll")
	float TraceCenterOfMassNormalDistance = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti roll")
	float AntiRollAfterTimeInAir = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float AngularDampingInAir = 3.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	bool bIsAngularDampingAllowedInAir = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float AngularDampingInAirResetDelay = 1.f;
	float DelayToResetDamping = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti stuck")
	bool bIsAntiStuckEnabled = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti stuck")
	float AntiStuckForce = 15000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti stuck")
	float SteeringMaxAngle = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement - Anti stuck")
	float AntistuckDelayTimeSec = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Movement")
	float DefaultTorque = 750.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bAreAllWheelesOnGround = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "VFX")
	TObjectPtr<UNiagaraSystem> DriftVFXSystem;

	bool bIsForwardInputTriggered = false;
	float AntiStuckTime = 0.f;
	bool bCanExecuteAntiStuck = false;
	float InAirTime = 0.f;

	TMap<int, bool> WheelDrifting;
	TMap<int, UNiagaraComponent*> DriftVfx;

	/** */
	virtual TUniquePtr<Chaos::FSimpleWheeledVehicle> CreatePhysicsVehicle() override
	{
		// Make the Vehicle Simulation class that will be updated from the physics thread async callback
		VehicleSimulationPT = MakeUnique<UChaosWheeledVehicleSimulation>();

		return UChaosVehicleMovementComponent::CreatePhysicsVehicle();
	}
};
