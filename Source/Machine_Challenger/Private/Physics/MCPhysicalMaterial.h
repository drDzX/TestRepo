// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "MCPhysicalMaterial.generated.h"

/**
 * 
 */
UCLASS()
class UMCPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	UMCPhysicalMaterial(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// A container of gameplay tags that game code can use to reason about this physical material
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PhysicalProperties)
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DamageProperties)
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=DamageProperties)
	bool IsTargetSpot = false;
};
