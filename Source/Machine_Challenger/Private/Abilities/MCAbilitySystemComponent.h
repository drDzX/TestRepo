// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "MCAbilitySystemInterface.h"
#include "MCAbilitySystemComponent.generated.h"

class UMCAbilitySystemComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRecieveDamageDelegate, float, DamageTaken, bool, isCritical, FVector, damageLocation, AActor*, damageMakerActor);

/**
 * 
 */
UCLASS()
class UMCAbilitySystemComponent : public UAbilitySystemComponent, public IMCAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Server, Reliable, Category= "Tags")
	void AddGameplayTag(FGameplayTag newTag);
	UFUNCTION(BlueprintCallable, Server, Reliable, Category= "Tags")
	void RemoveGameplayTag(FGameplayTag removeTag);

	UFUNCTION(BlueprintCallable, Category= "Abilities")
	void AbilityInputTagPressed(const FGameplayTag InputTag);
	UFUNCTION(BlueprintCallable, Category= "Abilities")
	void AbilityInputTagReleased(const FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable, Category= "Cooldown")
	float GetRemainingCooldownByTag(FGameplayTag CooldownTag);
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

protected:
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec& GameplayEffect, FPredictionKey PredictionKey) override;
	virtual void BeginPlay() override;
	virtual void OnTagUpdated(const FGameplayTag& Tag, bool TagExists) override;
	virtual void OnDamageRecieved_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker) override;

	UFUNCTION(NetMulticast, Unreliable)
	void OnDamageRecievedMulticast(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker = nullptr);

public:
	UPROPERTY(BlueprintAssignable, Category = "Damage")
	FRecieveDamageDelegate OnDamageRecieved;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category= "Tags")
	FGameplayTagContainer MCTags;
};
