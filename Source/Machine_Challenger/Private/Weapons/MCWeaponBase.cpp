// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/MCWeaponBase.h"

#include "MCWeaponInterface.h"
#include "Net/UnrealNetwork.h"
#include "Abilities/MCGameplayAbility.h"

#include "Abilities/MCAbilitySystemComponent.h"
#include "Abilities/MCAttributeSet.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/MCBlueprintLib.h"
#include "Vehicle/MCVehicleCombat.h"
#include "Vehicle/MCVehiclePawnBase.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Weapon_Activated, "GameEffect.Weapon.Activated");

// Sets default values
AMCWeaponBase::AMCWeaponBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMCWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	ConstructDynMaterials();
}

// Called every frame
void AMCWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAutoAimLocation();
	if (HasAuthority())
	{
		UpdateWeaponRotation(DeltaTime);
	}
	if (OwningCar && OwningCar->IsLocallyControlled())
	{
		UpdateWeaponAimTarget();
	}
}

void AMCWeaponBase::Equip_Implementation(AMCVehiclePawnBase* ownerCar)
{
	if (!ownerCar)
	{
		return;
	}

	OwningCar = ownerCar;
	if (const auto& combatCar = Cast<AMCVehicleCombat>(OwningCar))
	{
		combatCar->MountedWeapon = this;
	}

	AttachToOwner();
	AssignAbilities();
	AssignWeaponAttributes();

	OnEquipped();
	OnEquipEvent.Broadcast();
}

void AMCWeaponBase::UnEquip_Implementation()
{
	if (!OwningCar)
	{
		return;
	}

	RemoveAbilities();
	if (const auto& combatCar = Cast<AMCVehicleCombat>(OwningCar))
	{
		combatCar->MountedWeapon = nullptr;
	}

	if (OwningCar->AbilitySystemComponent)
	{
		OwningCar->AbilitySystemComponent->RemoveGameplayTag(TAG_Weapon_Activated);
	}

	OnUnEquipped();
	OwningCar = nullptr;
}

void AMCWeaponBase::AimAt_Implementation(FVector AimLocation)
{
	if (!OwningCar)
	{
		return;
	}

	const auto& TargetVector = AimLocation - GetActorLocation();
	FVector LocalToTarget = ActorToWorld().InverseTransformVector(TargetVector);
	LocalToTarget -= AimLocationOffset;
	const auto& Yaw = FMath::RadiansToDegrees(FMath::Atan2(LocalToTarget.Y, LocalToTarget.X));
	const auto& Pitch = FMath::RadiansToDegrees(FMath::Atan2(LocalToTarget.Z, FMath::Sqrt(LocalToTarget.X * LocalToTarget.X + LocalToTarget.Y * LocalToTarget.Y)));

	FRotator RelativeGunRotation = FRotator(Pitch, Yaw, 0.0f);
	{
		if (RotationLowerLimit.Yaw != 0.f || RotationUpperLimit.Yaw != 0.f)
		{
			if (RelativeGunRotation.Yaw < RotationLowerLimit.Yaw || RelativeGunRotation.Yaw > RotationUpperLimit.Yaw)
			{
				RelativeGunRotation.Yaw = FMath::Clamp(RelativeGunRotation.Yaw, RotationLowerLimit.Yaw, RotationUpperLimit.Yaw);
			}
		}

		if (RotationLowerLimit.Pitch != 0.f || RotationUpperLimit.Pitch != 0.f)
		{
			if (RelativeGunRotation.Pitch < RotationLowerLimit.Pitch || RelativeGunRotation.Pitch > RotationUpperLimit.Pitch)
			{
				RelativeGunRotation.Pitch = FMath::Clamp(RelativeGunRotation.Pitch, RotationLowerLimit.Pitch, RotationUpperLimit.Pitch);
			}
		}
	}

	// Rotate the AimLocationOffset to match the car's rotation
	const FVector RotatedOffset = GetActorRotation().RotateVector(AimLocationOffset);

	// Compute the start point for the trace
	const FVector StartLocation = GetActorLocation() + RotatedOffset;
	const auto& trace = UMCBlueprintLib::Trace(GetWorld(), nullptr, StartLocation, AimLocation, selfHitTraceData, AutoAimTraceChannel);
	//DrawDebugLine(GetWorld(), StartLocation, AimLocation, FColor::Black);
	if (trace && selfHitTraceData.GetActor() == OwningCar)
	{
		RelativeGunRotation.Pitch = AimAtCarSafePitch;
	}

	//Trace if behind camera
	const auto& traceCamera = UMCBlueprintLib::Trace(GetWorld(), nullptr, UMCBlueprintLib::GetCameraPosition(OwningCar), AimLocation, selfHitTraceData, ECC_Visibility);
	const bool isBehindLocal = traceCamera && selfHitTraceData.GetActor() == this;
	if (bIsBehindCamera != isBehindLocal)
	{
		bIsBehindCamera = isBehindLocal;
		IMCWeaponInterface::Execute_IsWeaponBehindCamera(this, isBehindLocal);
		UpdateDynMaterialOpacity(bIsBehindCamera ? BehindCameraOpacity : 1.f);
	}

	AimTargetRotation = RelativeGunRotation;
	AimTargetLocation = AimLocation;
}

void AMCWeaponBase::UpdateWeaponAimTarget_Implementation()
{
	UpdateWeaponAimTargetClient();
	const auto& Start = GetActorLocation() + GetActorRotation().RotateVector(AimLocationOffset);
	WeaponTraceAimPoint = GetActorLocation() + GetActorRotation().RotateVector(WeaponRotation.Vector() * AutoAimTraceDistance);
	//DrawDebugLine(GetWorld(), Start, WeaponTraceAimPoint, FColor::Black);
	FHitResult hitResult;
	UMCBlueprintLib::Trace(GetWorld(), OwningCar, Start, WeaponTraceAimPoint, hitResult, ECC_GameTraceChannel2);
	if (!hitResult.GetActor())
	{
		WeaponAimLocation = WeaponTraceAimPoint;
		return;
	}

	WeaponAimLocation = hitResult.Location;
}

void AMCWeaponBase::AimAtServer_Implementation(FVector AimLocation)
{
	AimAt(AimLocation);
}

void AMCWeaponBase::UpdateWeaponAimTargetClient_Implementation()
{
	const auto& Start = GetActorLocation() + GetActorRotation().RotateVector(AimLocationOffset);
	WeaponTraceAimPoint = GetActorLocation() + GetActorRotation().RotateVector(WeaponRotation.Vector() * AutoAimTraceDistance);
	//DrawDebugLine(GetWorld(), Start, WeaponTraceAimPoint, FColor::Black);
	FHitResult hitResult;
	UMCBlueprintLib::Trace(GetWorld(), OwningCar, Start, WeaponTraceAimPoint, hitResult, ECC_GameTraceChannel2);
	if (!hitResult.GetActor())
	{
		WeaponAimLocationClient = WeaponTraceAimPoint;
		return;
	}

	WeaponAimLocationClient = hitResult.Location;
}

void AMCWeaponBase::Destroyed()
{
	Super::Destroyed();
}

void AMCWeaponBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCWeaponBase, OwningCar);
	//DOREPLIFETIME(AMCWeaponBase, WeaponRotation);
	DOREPLIFETIME(AMCWeaponBase, WeaponAimLocation);
	DOREPLIFETIME(AMCWeaponBase, bIsFiring);
}

void AMCWeaponBase::AssignWeaponAttributes_Implementation()
{
	if (!OwningCar)
	{
		return;
	}
	const auto& carAbilityComponent = OwningCar->AbilitySystemComponent;
	check(OwningCar->AbilitySystemComponent);
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	for (const auto& gameEffect : WeaponAttributes)
	{
		auto effectContext = carAbilityComponent->MakeEffectContext();
		effectContext.AddSourceObject(this);

		auto NewHandle = carAbilityComponent->MakeOutgoingSpec(gameEffect, 1, effectContext);

		if (!NewHandle.IsValid())
		{
			continue;
		}

		auto activeGameplayEffectHandle = carAbilityComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), carAbilityComponent);
	}
}

void AMCWeaponBase::AttachToOwner()
{
	if (!OwningCar || AttachSocketName.IsNone())
	{
		return;
	}

	AttachToComponent(OwningCar->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
}

void AMCWeaponBase::OnAttackBegin_Implementation()
{
	IMCWeaponInterface::OnAttackBegin_Implementation();
	bIsFiring = true;
}

void AMCWeaponBase::OnAttackEnd_Implementation()
{
	IMCWeaponInterface::OnAttackEnd_Implementation();
	bIsFiring = false;
}

void AMCWeaponBase::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AddTag(WeaponIdTag);
}

float AMCWeaponBase::GetWeaponSpread_Implementation()
{
	if (!OwningCar)
	{
		return 0.f;
	}

	return OwningCar->Attributes->GetWeaponSpread();
}

void AMCWeaponBase::ConstructDynMaterials()
{
	if (!WeaponMesh)
	{
		return;
	}

	const auto& materials = WeaponMesh->GetMaterials();
	int i = 0;
	for (const auto& material : materials)
	{
		DynMaterials.Add(UMaterialInstanceDynamic::Create(material, this));
		WeaponMesh->SetMaterial(i, DynMaterials[i]);
		i++;
	}
}

void AMCWeaponBase::UpdateDynMaterialOpacity_Implementation(float NewOpacity)
{
	if (DynMaterials.IsEmpty())
	{
		return;
	}

	for (const auto& DynMat : DynMaterials)
	{
		DynMat->SetScalarParameterValue(OpacityParameterName, NewOpacity);
	}
}

void AMCWeaponBase::UpdateAutoAimLocation_Implementation()
{
	if (!bIsAutoAiming || !OwningCar)
	{
		return;
	}

	const auto& hitResult = UMCBlueprintLib::GetAimLocation(GetWorld(), OwningCar, AutoAimTraceDistance, AutoAimTraceChannel, true, CrosshairOffset);
	if (OwningCar && OwningCar->IsLocallyControlled())
	{
		AimAtServer(hitResult.Location);
	}
}

void AMCWeaponBase::UpdateWeaponRotation_Implementation(float dt)
{
	if (!OwningCar || !OwningCar->Attributes || UKismetMathLibrary::EqualEqual_RotatorRotator(WeaponRotation, AimTargetRotation, 0.05f))
	{
		return;
	}

	const auto& turnRate = OwningCar->GetAttributes()->GetWeaponTurnRate();

	// Update the current rotation
	//WeaponRotation = UKismetMathLibrary::RInterpTo(WeaponRotation, AimTargetRotation, dt, turnRate);

	// Calculate rotation step based on the speed and delta time
	const auto& MaxStep = turnRate * dt;

	// Calculate the rotation needed to reach the target
	FRotator DeltaRotation = (AimTargetRotation - WeaponRotation).GetNormalized();

	// Clamp each component of the DeltaRotation
	DeltaRotation.Yaw = FMath::Clamp(DeltaRotation.Yaw, -MaxStep, MaxStep);
	DeltaRotation.Pitch = FMath::Clamp(DeltaRotation.Pitch, -MaxStep, MaxStep);
	DeltaRotation.Roll = FMath::Clamp(DeltaRotation.Roll, -MaxStep, MaxStep);

	// Update the current rotation by the clamped delta rotation
	WeaponRotation += DeltaRotation;
}

void AMCWeaponBase::AssignAbilities_Implementation()
{
	if (!OwningCar || WeaponAbilities.IsEmpty())
	{
		return;
	}

	FGameplayAbilitySpecHandle abiltiyHandle;
	for (const auto& [abilityInput, abilityTemplate] : WeaponAbilities)
	{
		AbilitySpecHandles.Add(
			OwningCar->GiveGameplayAbility(abilityTemplate, 1, abilityInput)
		);
	}
}

void AMCWeaponBase::RemoveAbilities_Implementation()
{
	if (!OwningCar)
	{
		return;
	}

	for (const auto& handle : AbilitySpecHandles)
	{
		OwningCar->AbilitySystemComponent->CancelAbilities();
		OwningCar->AbilitySystemComponent->ClearAbility(handle);
	}

	AbilitySpecHandles.Reset();
}
