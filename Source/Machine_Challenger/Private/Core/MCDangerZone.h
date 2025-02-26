// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Actor.h"
#include "MCDangerZone.generated.h"

UCLASS()
class AMCDangerZone : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMCDangerZone();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void DangerZoneActivationChange(bool IsActivated);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void CreateEffectHandle();

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DamageAmount = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag DamageMagnitudeTag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<AActor*, FActiveGameplayEffectHandle> ActiveHandles;
};
