// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MCDamageExecCalculation.generated.h"

/**
 * 
 */
UCLASS()
class UMCDamageExecCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	UMCDamageExecCalculation();
public:
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
