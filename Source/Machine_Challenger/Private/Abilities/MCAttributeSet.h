// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NativeGameplayTags.h"
#include "MCAttributeSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnergyChanged, float, newValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponEnergyChanged, float, newValue);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAmmoChanged, float, newValue);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Attribute_Health);

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
/**
 *
 */
UCLASS()
class UMCAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	void AdjustAttributeForMaxChange(const FGameplayAttributeData& affectedAttribute, const FGameplayAttributeData& maxAttribute, float newMaxValue, const FGameplayAttribute& AffectedAttributeProperty) const;

private:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& oldValue);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& oldValue);

	UFUNCTION()
	virtual void OnRep_Energy(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_WeaponEnergy(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_MaxEnergy(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_MinEnergy(const FGameplayAttributeData& oldValue);

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);
	UFUNCTION()
	virtual void OnRep_Armor(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_AttackDamage(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_CritChance(const FGameplayAttributeData& oldValue);

	UFUNCTION()
	virtual void OnRep_FireRate(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_WeaponTurnRate(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_PickupEnergy1(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_PickupEnergy2(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_PickupEnergy3(const FGameplayAttributeData& oldValue);

	UFUNCTION()
	virtual void OnRep_AmmoCount(const FGameplayAttributeData& oldValue);

	UFUNCTION()
	virtual void OnRep_WeaponSpread(const FGameplayAttributeData& oldValue);
	UFUNCTION()
	virtual void OnRep_WeaponMaxSpread(const FGameplayAttributeData& oldValue);

private:
	void UpdateHealth(float deltaValue, AActor* targetActor);
	void UpdateEnergy(float deltaValue, AActor* targetActor);

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "Attributes")
	FGameplayAttributeData Energy;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, Energy);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Energy, Category = "Attributes")
	FGameplayAttributeData WeaponEnergy;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, WeaponEnergy);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxEnergy, Category = "Attributes")
	FGameplayAttributeData MaxEnergy;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, MaxEnergy);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MinEnergy, Category = "Attributes")
	FGameplayAttributeData MinEnergy;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, MinEnergy);

	// MoveSpeed affects how fast characters can move.
	UPROPERTY(BlueprintReadOnly, Category = "MoveSpeed", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Attributes")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, Armor);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackDamage, Category = "Attributes")
	FGameplayAttributeData AttackDamage;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, AttackDamage);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CritChance, Category = "Attributes")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, CritChance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireRate, Category = "Attributes")
	FGameplayAttributeData FireRate;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, FireRate);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponTurnRate, Category = "Attributes")
	FGameplayAttributeData WeaponTurnRate;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, WeaponTurnRate);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PickupEnergy1, Category = "Attributes")
	FGameplayAttributeData PickupEnergy1;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, PickupEnergy1);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PickupEnergy2, Category = "Attributes")
	FGameplayAttributeData PickupEnergy2;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, PickupEnergy2);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PickupEnergy3, Category = "Attributes")
	FGameplayAttributeData PickupEnergy3;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, PickupEnergy3);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AmmoCount, Category = "Attributes")
	FGameplayAttributeData AmmoCount;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, AmmoCount);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponSpread, Category = "Attributes")
	FGameplayAttributeData WeaponSpread;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, WeaponSpread);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponMaxSpread, Category = "Attributes")
	FGameplayAttributeData WeaponMaxSpread;
	ATTRIBUTE_ACCESSORS(UMCAttributeSet, WeaponMaxSpread);

	UPROPERTY(BlueprintAssignable)
	FOnAmmoChanged OnAmmoChangedEvent;
	UPROPERTY(BlueprintAssignable)
	FOnEnergyChanged OnEnergyChangedEvent;
	UPROPERTY(BlueprintAssignable)
	FOnWeaponEnergyChanged OnWeaponEnergyChangedEvent;
};
