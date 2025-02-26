// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MCWeaponInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMCWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class IMCWeaponInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	FRotator GetWeaponRotation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	FVector GetMuzzleLocation(int MuzzleId = 0);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	int GetMuzzleId();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	FVector GetWeaponAimTraceLocation();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	FVector GetWeaponAimLocation();

	//One or more fire executions, each time gun triggers
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnAttackExecuted(int MuzzleId);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnAttackBegin();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnAttackEnd();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnArmingStarted();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnWeaponSpawned(AMCWeaponBase* weapon);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnArmingFinished();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnOpenCloseTrunk(bool IsOpened, float Delay);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	AMCWeaponBase* GetMountedWeapon();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	float GetWeaponSpread();

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon")
	void IsWeaponBehindCamera(bool IsBehindCamera);
};
