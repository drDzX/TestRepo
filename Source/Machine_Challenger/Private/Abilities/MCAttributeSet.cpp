// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/MCAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "MCAbilitySystemInterface.h"

#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Attribute_Health, "GameEffect.Health");

void UMCAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMCAttributeSet, Health);
	DOREPLIFETIME(UMCAttributeSet, MaxHealth);
	DOREPLIFETIME(UMCAttributeSet, Energy);
	DOREPLIFETIME(UMCAttributeSet, MaxEnergy);
	DOREPLIFETIME(UMCAttributeSet, MinEnergy);
	DOREPLIFETIME(UMCAttributeSet, MoveSpeed);
	DOREPLIFETIME(UMCAttributeSet, FireRate);
	DOREPLIFETIME(UMCAttributeSet, WeaponTurnRate);
	DOREPLIFETIME(UMCAttributeSet, PickupEnergy1);
	DOREPLIFETIME(UMCAttributeSet, PickupEnergy2);
	DOREPLIFETIME(UMCAttributeSet, PickupEnergy3);
	DOREPLIFETIME(UMCAttributeSet, AmmoCount);
	DOREPLIFETIME(UMCAttributeSet, WeaponSpread);
	DOREPLIFETIME(UMCAttributeSet, WeaponMaxSpread);
	DOREPLIFETIME(UMCAttributeSet, Armor);
	DOREPLIFETIME(UMCAttributeSet, AttackDamage);
	DOREPLIFETIME(UMCAttributeSet, CritChance);
	DOREPLIFETIME(UMCAttributeSet, WeaponEnergy);
}

void UMCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	if (Attribute == GetMaxHealthAttribute())
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}

	if (Attribute == GetMaxEnergyAttribute())
	{
		AdjustAttributeForMaxChange(Energy, MaxEnergy, NewValue, GetEnergyAttribute());
	}
}

void UMCAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.ModifierOp != EGameplayModOp::Type::Additive)
	{
		return;
	}

	const auto& deltaValue = Data.EvaluatedData.Magnitude;

	const auto& abilityActorInfo = Data.Target.AbilityActorInfo;
	if (!abilityActorInfo.IsValid() && !abilityActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	auto* actor = abilityActorInfo->AvatarActor.Get();
	const auto& attribute = Data.EvaluatedData.Attribute;
	if (attribute == GetHealthAttribute())
	{
		UpdateHealth(deltaValue, actor);
		if (actor->Implements<UMCAbilitySystemInterface>())
		{
			IMCAbilitySystemInterface::Execute_OnHealthUpdated(actor, GetHealth());
		}
	}

	if (attribute == GetEnergyAttribute())
	{
		UpdateEnergy(deltaValue, actor);
		OnEnergyChangedEvent.Broadcast(GetEnergy());
	}
	if (attribute == GetWeaponEnergyAttribute())
	{
		OnWeaponEnergyChangedEvent.Broadcast(GetWeaponEnergy());
	}

	if (attribute == GetAmmoCountAttribute())
	{
		OnAmmoChangedEvent.Broadcast(GetAmmoCount());
	}

	if (attribute == GetWeaponSpreadAttribute())
	{
		SetWeaponSpread(FMath::Clamp(GetWeaponSpread(), 0.f, GetWeaponMaxSpread()));
	}
}

void UMCAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (OldValue == NewValue)
	{
		return;
	}

	if (Attribute == GetEnergyAttribute())
	{
		//Report if needed.
	}
}

void UMCAttributeSet::AdjustAttributeForMaxChange(const FGameplayAttributeData& affectedAttribute, const FGameplayAttributeData& maxAttribute, float newMaxValue, const FGameplayAttribute& AffectedAttributeProperty) const
{
	auto* AbilitySystemComponent = GetOwningAbilitySystemComponent();
	const auto& CurrentMaxValue = maxAttribute.GetBaseValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, newMaxValue) && AbilitySystemComponent)
	{
		const auto& currentValue = affectedAttribute.GetCurrentValue();
		const auto& newDelta = CurrentMaxValue > 0.f ? (currentValue * newMaxValue / CurrentMaxValue) - currentValue : newMaxValue;

		AbilitySystemComponent->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, newDelta);
	}
}

void UMCAttributeSet::OnRep_Health(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, Health, oldValue);
}

void UMCAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, MaxHealth, oldValue);
}

void UMCAttributeSet::OnRep_Energy(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, Energy, oldValue);
}

void UMCAttributeSet::OnRep_WeaponEnergy(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, WeaponEnergy, oldValue);
}

void UMCAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, MaxEnergy, oldValue);
}

void UMCAttributeSet::OnRep_MinEnergy(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, MinEnergy, oldValue);
}

void UMCAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, MoveSpeed, OldMoveSpeed);
}

void UMCAttributeSet::OnRep_Armor(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, Armor, oldValue);
}

void UMCAttributeSet::OnRep_AttackDamage(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, AttackDamage, oldValue);
}

void UMCAttributeSet::OnRep_CritChance(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, CritChance, oldValue);
}

void UMCAttributeSet::OnRep_FireRate(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, FireRate, oldValue);
}

void UMCAttributeSet::OnRep_WeaponTurnRate(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, WeaponTurnRate, oldValue);
}

void UMCAttributeSet::OnRep_PickupEnergy1(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, PickupEnergy1, oldValue);
}

void UMCAttributeSet::OnRep_PickupEnergy2(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, PickupEnergy2, oldValue);
}

void UMCAttributeSet::OnRep_PickupEnergy3(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, PickupEnergy3, oldValue);
}

void UMCAttributeSet::OnRep_AmmoCount(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, AmmoCount, oldValue);
	OnAmmoChangedEvent.Broadcast(GetAmmoCount());
}

void UMCAttributeSet::OnRep_WeaponSpread(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, WeaponSpread, oldValue);
}

void UMCAttributeSet::OnRep_WeaponMaxSpread(const FGameplayAttributeData& oldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UMCAttributeSet, WeaponMaxSpread, oldValue);
}

void UMCAttributeSet::UpdateHealth(float deltaValue, AActor* targetActor)
{
	SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
}

void UMCAttributeSet::UpdateEnergy(float deltaValue, AActor* targetActor)
{
	SetEnergy(FMath::Clamp(GetEnergy(), GetMinEnergy(), GetMaxEnergy()));
}
