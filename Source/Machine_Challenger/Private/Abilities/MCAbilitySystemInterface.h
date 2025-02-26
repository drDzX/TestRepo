// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MCAbilitySystemInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMCAbilitySystemInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 *
 */
class IMCAbilitySystemInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnDamageRecieved(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnReportDamageDone(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker, AActor* damageReciever);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnReportKillDone(APlayerState* damageMaker, APlayerState* killedActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnReportPlayerRespawned();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Events")
	void OnBoostTriggered(bool isActivated);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attributes")
	void OnHealthUpdated(float deltaValue);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attributes")
	void OnEnergyUpdated(float deltaValue);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Attributes")
	void OnAmmoChanged(float deltaValue);
};
