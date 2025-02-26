// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MCVehicleMovementComponent.h"
#include "GameplayTagContainer.h"
#include "MCVehicleInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMCVehicleInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class IMCVehicleInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Camera")
	void StartCameraShake(TSubclassOf<UCameraShakeBase> InCameraShake, float Scale = 1.f, ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	UWeaponSpawnerComponent* GetWeaponSpawner();

	UFUNCTION(BlueprintNativeEvent, Category = "Car")
	TArray<UMCSuspensionComponent*> GetSuspensions(FGameplayTagContainer SideTags);
	UFUNCTION(BlueprintNativeEvent, Category = "Car")
	TArray<UStaticMeshComponent*> GetWheelComponents(FGameplayTagContainer SideTags);
	UFUNCTION(BlueprintNativeEvent, Category = "Car")
	UMeshComponent* GetAttachmentMesh();
	UFUNCTION
	(BlueprintNativeEvent, BlueprintCallable)
	void DriftEvent(FDriftInfo driftData);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool IsMovementAllowed();
};
