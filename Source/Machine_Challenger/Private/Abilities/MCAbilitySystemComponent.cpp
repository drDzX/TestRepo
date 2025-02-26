// Fill out your copyright notice in the Description page of Project Settings.

#include "MCAbilitySystemComponent.h"

#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Movement_Stunned, "GameEffect.Stunned")

void UMCAbilitySystemComponent::AddGameplayTag_Implementation(FGameplayTag newTag)
{
	MCTags.AddTag(newTag);
	UpdateTagMap(newTag, 1);
}

void UMCAbilitySystemComponent::RemoveGameplayTag_Implementation(FGameplayTag removeTag)
{
	MCTags.RemoveTag(removeTag);
	UpdateTagMap(removeTag, -1);
}

void UMCAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag InputTag)
{
	if (InputTag.IsValid())
	{
		for (FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				AbilitySpec.InputPressed = true;
				AbilitySpecInputPressed(AbilitySpec);
				TryActivateAbility(AbilitySpec.Handle);
			}
		}
	}
}

void UMCAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag InputTag)
{
	if (InputTag.IsValid())
	{
		for (FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
		{
			if (AbilitySpec.Ability && (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag)))
			{
				AbilitySpec.InputPressed = false;
				AbilitySpecInputReleased(AbilitySpec);
			}
		}
	}
}

float UMCAbilitySystemComponent::GetRemainingCooldownByTag(FGameplayTag CooldownTag)
{
	FGameplayEffectQuery query;
	query.CustomMatchDelegate.BindLambda([&](const FActiveGameplayEffect& CurEffect)
	{
		FGameplayTagContainer foundTags;
		CurEffect.Spec.GetAllGrantedTags(foundTags);

		if (foundTags.IsEmpty())
		{
			return false;
		}

		if (foundTags.HasTag(CooldownTag))
		{
			return true;
		}

		return false;
	});

	auto durations = ActiveGameplayEffects.GetActiveEffectsTimeRemaining(query);
	if (durations.Num() > 0)
	{
		durations.Sort();
		return durations[durations.Num() - 1];
	}

	return 0.f;
}

void UMCAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputPress ability task works.
	if (Spec.IsActive())
	{
		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
	}
}

void UMCAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	// We don't support UGameplayAbility::bReplicateInputDirectly.
	// Use replicated events instead so that the WaitInputRelease ability task works.
	if (Spec.IsActive())
	{
		// Invoke the InputReleased event. This is not replicated here. If someone is listening, they may replicate the InputReleased event to the server.
		InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
	}
}

void UMCAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMCAbilitySystemComponent, MCTags);
}

void UMCAbilitySystemComponent::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	Super::GetOwnedGameplayTags(TagContainer);
	TagContainer.AppendTags(MCTags);
}

FActiveGameplayEffectHandle UMCAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec& GameplayEffect, FPredictionKey PredictionKey)
{
	return Super::ApplyGameplayEffectSpecToSelf(GameplayEffect, PredictionKey);
}

void UMCAbilitySystemComponent::BeginPlay()
{
	Super::BeginPlay();
	SetReplicationMode(EGameplayEffectReplicationMode::Full);
}

void UMCAbilitySystemComponent::OnTagUpdated(const FGameplayTag& Tag, bool TagExists)
{
	Super::OnTagUpdated(Tag, TagExists);
	if (Tag == TAG_Movement_Stunned)
	{
		CancelAllAbilities();
	}
}

void UMCAbilitySystemComponent::OnDamageRecieved_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker)
{
	IMCAbilitySystemInterface::OnDamageRecieved_Implementation(Damage, isCritical, damageLocation, damageMaker);
	if (GetOwner()->Implements<UMCAbilitySystemInterface>())
	{
		IMCAbilitySystemInterface::Execute_OnDamageRecieved(GetOwner(), Damage, isCritical, damageLocation, damageMaker);
	}

	OnDamageRecievedMulticast(Damage, isCritical, damageLocation, damageMaker);
}

void UMCAbilitySystemComponent::OnDamageRecievedMulticast_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker)
{
	OnDamageRecieved.Broadcast(Damage, isCritical, damageLocation, damageMaker);
}
