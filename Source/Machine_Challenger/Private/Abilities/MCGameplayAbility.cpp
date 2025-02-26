// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/MCGameplayAbility.h"

#include "MCAbilitySystemComponent.h"
#include "Vehicle/MCVehiclePawnBase.h"

FGameplayEffectContextHandle UMCGameplayAbility::MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);
	FGameplayEffectContext* EffectContext = ContextHandle.Get();

	AActor* EffectCauser = nullptr;
	float SourceLevel = 0.0f;
	GetAbilitySource(Handle, ActorInfo, SourceLevel, EffectCauser);

	const auto* SourceObject = GetSourceObject(Handle, ActorInfo);

	AActor* Instigator = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;

	EffectContext->AddInstigator(Instigator, EffectCauser);
	EffectContext->AddSourceObject(SourceObject);

	return ContextHandle;
}

void UMCGameplayAbility::GetAbilitySource(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& OutSourceLevel, AActor*& OutEffectCauser) const
{
	OutSourceLevel = 0.0f;
	OutEffectCauser = nullptr;

	OutEffectCauser = ActorInfo->AvatarActor.Get();
}

void UMCGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UMCGameplayAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);
	if (const auto& carPawn = Cast<AMCVehiclePawnBase>(GetOwningActorFromActorInfo()))
	{
		for (const auto& tag : AbilityTags)
		{
			carPawn->AbilitySystemComponent.Get()->AddGameplayTag(tag);
		}
	}
}

void UMCGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	OnMCSGameplayAbilityEnded.Broadcast(this);
	if (const auto& carPawn = Cast<AMCVehiclePawnBase>(GetOwningActorFromActorInfo()))
	{
		for (const auto& tag : AbilityTags)
		{
			carPawn->AbilitySystemComponent.Get()->RemoveGameplayTag(tag);
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
