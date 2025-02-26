// Fill out your copyright notice in the Description page of Project Settings.

#include "Abilities/MCDamageExecCalculation.h"

#include "MCAttributeSet.h"
#include "Vehicle/MCVehiclePawnBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "MCAbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "Physics/MCPhysicalMaterial.h"
#include "Weapons/MCWeaponInterface.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Damage, "GameEffect.Health");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CritChance, "GameEffect.Stats.CritChance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Multiplier, "GameEffect.Stats.DamageMultiplier");

struct FDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;
	FGameplayEffectAttributeCaptureDefinition BaseArmorDef;
	FGameplayEffectAttributeCaptureDefinition CritChanceDef;

	FDamageStatics()
	{
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(UMCAttributeSet::GetAttackDamageAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		BaseArmorDef = FGameplayEffectAttributeCaptureDefinition(UMCAttributeSet::GetArmorAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
		CritChanceDef = FGameplayEffectAttributeCaptureDefinition(UMCAttributeSet::GetCritChanceAttribute(), EGameplayEffectAttributeCaptureSource::Source, true);
	}
};

static FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UMCDamageExecCalculation::UMCDamageExecCalculation()
{
	RelevantAttributesToCapture.Add(DamageStatics().BaseDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritChanceDef);
}

void UMCDamageExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FGameplayEffectContext* TypedContext = Spec.GetContext().Get();
	check(TypedContext);

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().BaseDamageDef, EvaluateParameters, BaseDamage);
	const float AddedDamage = Spec.GetSetByCallerMagnitude(TAG_Damage);
	const float DamageMultiplier = Spec.GetSetByCallerMagnitude(TAG_Multiplier, false, 1);

	const float AddedCritChance = Spec.GetSetByCallerMagnitude(TAG_CritChance);
	float EffectCauserChances = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritChanceDef, EvaluateParameters, EffectCauserChances);

	AMCVehiclePawnBase* EffectCauser = Cast<AMCVehiclePawnBase>(TypedContext->GetEffectCauser());
	const float CritChance = EffectCauserChances + AddedCritChance;

	const FHitResult* HitActorResult = TypedContext->GetHitResult();

	AMCVehiclePawnBase* HitActor = nullptr;
	FVector ImpactLocation = FVector::ZeroVector;
	float EnemyArmor = 0.f;
	FVector ImpactNormal = FVector::ZeroVector;
	FVector StartTrace = FVector::ZeroVector;
	FVector EndTrace = FVector::ZeroVector;

	if (HitActorResult)
	{
		const FHitResult& CurHitResult = *HitActorResult;
		HitActor = Cast<AMCVehiclePawnBase>(CurHitResult.HitObjectHandle.FetchActor());
		if (HitActor)
		{
			ImpactLocation = CurHitResult.ImpactPoint;
			if (const auto& attrSet = HitActor->Attributes)
			{
				EnemyArmor = attrSet->GetArmor();
			}
			ImpactNormal = CurHitResult.ImpactNormal;
			StartTrace = CurHitResult.TraceStart;
			EndTrace = CurHitResult.TraceEnd;
		}
	}

	// Handle case of no hit result or hit result not actually returning an actor
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!HitActor)
	{
		HitActor = TargetAbilitySystemComponent ? Cast<AMCVehiclePawnBase>(TargetAbilitySystemComponent->GetAvatarActor_Direct()) : nullptr;
		if (HitActor)
		{
			if (const auto& attrSet = HitActor->Attributes)
			{
				EnemyArmor = attrSet->GetArmor();
			}

			ImpactLocation = HitActor->GetActorLocation();
		}
	}

	if (ImpactLocation.IsNearlyZero() && HitActor)
	{
		ImpactLocation = HitActor->GetActorLocation();
	}

	// Apply rules for team damage/self damage/etc...
	float DamageInteractionAllowedMultiplier = 0.0f;
	if (HitActor)
	{
		//const auto& otherActorTeam = IMCInteractionsInterface::Execute_GetPlayerTeam(HitActor);
		//const auto& causerActorTeam = IMCInteractionsInterface::Execute_GetPlayerTeam(EffectCauser);	
		DamageInteractionAllowedMultiplier = 1.f;
	}

	// Determine distance
	double Distance = WORLD_MAX;

	if (TypedContext->HasOrigin())
	{
		Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
	}
	else if (EffectCauser)
	{
		Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
	}
	else
	{
		ensureMsgf(false, TEXT("Damage Calculation cannot deduce a source location for damage coming from %s; Falling back to WORLD_MAX dist!"), *GetPathNameSafe(Spec.Def));
	}

	float MaterialMultiplier = 1.f;

	if (HitActorResult && HitActorResult->PhysMaterial.Get())
	{
		if (const auto& pMaterial = Cast<UMCPhysicalMaterial>(HitActorResult->PhysMaterial.Get()))
		{
			MaterialMultiplier = pMaterial->DamageMultiplier;
		}
	}

	const float CritMultiplier = UKismetMathLibrary::RandomBoolWithWeight(CritChance / 100.f) ? 2 : 1;

	// Damage calculation
	const float DamageDone = FMath::Min((BaseDamage + AddedDamage - EnemyArmor) * MaterialMultiplier * CritMultiplier * DamageInteractionAllowedMultiplier * DamageMultiplier * -1, -1.0f);

	if (DamageDone < 0.0f)
	{
		// Apply a damage modifier, this gets turned into - health on the target
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UMCAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, DamageDone));
		if (TargetAbilitySystemComponent->Implements<UMCAbilitySystemInterface>())
		{
			IMCAbilitySystemInterface::Execute_OnDamageRecieved(TargetAbilitySystemComponent, DamageDone, CritMultiplier == 2, ImpactLocation, EffectCauser);
		}

		if (EffectCauser->GetPlayerState() && EffectCauser->GetPlayerState()->Implements<UMCAbilitySystemInterface>())
		{
			IMCAbilitySystemInterface::Execute_OnReportDamageDone(EffectCauser->GetPlayerState(), std::abs(DamageDone), CritMultiplier == 2, ImpactLocation, EffectCauser, HitActor);
		}

		if (EffectCauser && EffectCauser->Implements<UMCAbilitySystemInterface>())
		{
			IMCAbilitySystemInterface::Execute_OnReportDamageDone(EffectCauser, std::abs(DamageDone), CritMultiplier == 2, ImpactLocation, EffectCauser, HitActor);
		}
	}
#endif
}
