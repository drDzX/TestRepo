// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCVehicleCombat.h"

#include "Abilities/MCAbilitySystemComponent.h"
#include "Core/MCGameInterface.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Utils/MCBlueprintLib.h"
#include "Weapons/MCWeaponBase.h"
#include "Vehicle/MCVehicleAssemblyComponent.h"

AMCVehicleCombat::AMCVehicleCombat(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	CarAssemblyComponent = CreateDefaultSubobject<UMCVehicleAssemblyComponent>(TEXT("Vehicle Assembly"));
}

void AMCVehicleCombat::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCVehicleCombat, MountedWeapon);
	DOREPLIFETIME(AMCVehicleCombat, LastDamageMaker);
	DOREPLIFETIME(AMCVehicleCombat, bIsKilled);
	DOREPLIFETIME(AMCVehicleCombat, CarColor);
}

FRotator AMCVehicleCombat::GetWeaponRotation_Implementation()
{
	if (!MountedWeapon)
	{
		return FRotator::ZeroRotator;
	}

	return MountedWeapon.Get()->WeaponRotation;
}

FVector AMCVehicleCombat::GetMuzzleLocation_Implementation(int MuzzleId)
{
	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return FVector::ZeroVector;
	}

	return IMCWeaponInterface::Execute_GetMuzzleLocation(MountedWeapon, MuzzleId);
}

FVector AMCVehicleCombat::GetWeaponAimTraceLocation_Implementation()
{
	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return FVector::ZeroVector;
	}

	return MountedWeapon.Get()->WeaponTraceAimPoint;
}

void AMCVehicleCombat::OnAttackExecuted_Implementation(int MuzzleId)
{
	IMCWeaponInterface::OnAttackExecuted_Implementation(MuzzleId);

	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return;
	}

	IMCWeaponInterface::Execute_OnAttackExecuted(MountedWeapon, MuzzleId);
}

FVector AMCVehicleCombat::GetWeaponAimLocation_Implementation()
{
	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return FVector::ZeroVector;
	}

	return MountedWeapon.Get()->WeaponAimLocation;
}

int AMCVehicleCombat::GetMuzzleId_Implementation()
{
	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return 0;
	}

	return IMCWeaponInterface::Execute_GetMuzzleId(MountedWeapon);
}

void AMCVehicleCombat::OnAttackBegin_Implementation()
{
	IMCWeaponInterface::OnAttackBegin_Implementation();

	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return;
	}

	IMCWeaponInterface::Execute_OnAttackBegin(MountedWeapon);
}

void AMCVehicleCombat::OnAttackEnd_Implementation()
{
	IMCWeaponInterface::OnAttackEnd_Implementation();

	if (!MountedWeapon || !MountedWeapon->Implements<UMCWeaponInterface>())
	{
		return;
	}

	IMCWeaponInterface::Execute_OnAttackEnd(MountedWeapon);
}

void AMCVehicleCombat::OnHealthUpdated_Implementation(float deltaValue)
{
	if (HasAuthority() && !bIsKilled && deltaValue <= 0.f && LastDamageMaker && LastDamageMaker->GetPlayerState() && LastDamageMaker->GetPlayerState()->Implements<UMCAbilitySystemInterface>())
	{
		if (GetPlayerState()->Implements<UMCAbilitySystemInterface>())
		{
			IMCAbilitySystemInterface::Execute_OnReportKillDone(GetPlayerState(), LastDamageMaker->GetPlayerState(), GetPlayerState());
		}

		IMCAbilitySystemInterface::Execute_OnReportKillDone(LastDamageMaker->GetPlayerState(), LastDamageMaker->GetPlayerState(), GetPlayerState());

		bIsKilled = true;
		LastDamageMaker = nullptr;
	}
	IMCAbilitySystemInterface::OnHealthUpdated_Implementation(deltaValue);
}

void AMCVehicleCombat::OnDamageRecieved_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker)
{
	if (!HasAuthority())
	{
		return;
	}

	if (const auto& mcCar = Cast<AMCVehiclePawnBase>(damageMaker))
	{
		LastDamageMaker = mcCar;
		DamageMakerCooldownDuration = DamageMakerCooldown;
	}

	IMCAbilitySystemInterface::OnDamageRecieved_Implementation(Damage, isCritical, damageLocation, damageMaker);
}

void AMCVehicleCombat::OnWeaponSpawned_Implementation(AMCWeaponBase* weapon)
{
	OnWeaponSpawnedMulticast();
}

void AMCVehicleCombat::OnWeaponSpawnedMulticast_Implementation()
{
	WeaponSpawnedEvent.Broadcast();
}

UMaterialInterface* AMCVehicleCombat::GetMaterialByColorId(const FString& ColorId, bool ConsiderTeamTag)
{
	if (ConsiderTeamTag)
	{
		const auto& controller = GetController();
		if (controller && controller->Implements<UMCGameInterface>())
		{
			return UMCBlueprintLib::GetMaterialByColorId(GetWorld(), ColorId, IMCGameInterface::Execute_GetPlayerTeam(controller));
		}
	}

	return UMCBlueprintLib::GetMaterialByColorId(GetWorld(), ColorId, FGameplayTag());
}

void AMCVehicleCombat::UpdateCarColor_Implementation(const FString& ColorId, bool ConsiderTeamTag)
{
	SetCarColor(GetMaterialByColorId(ColorId, ConsiderTeamTag));
}

void AMCVehicleCombat::OnRep_ColorChanged_Implementation(UMaterialInterface* newValue)
{
	UE_LOG(LogTemp, Log, TEXT("CarColor has changed!"));
	ApplyDynamicMaterial();

	OnCarColorChanged();
}

void AMCVehicleCombat::SetCarColor_Implementation(UMaterialInterface* NewCarColor)
{
	CarColor = NewCarColor;

	// Apply replicated color on clients
	if (HasAuthority())
	{
		OnRep_ColorChanged(CarColor);
	}
}

void AMCVehicleCombat::Explode()
{
	if (!Body)
	{
		return;
	}

	PrimitiveComponent = nullptr;
	MCTags.AddTag(TAG_Movement_DisableAll);
	Body->AddForce(FVector(0, 0, 10000000));
}

AMCWeaponBase* AMCVehicleCombat::GetMountedWeapon_Implementation()
{
	return MountedWeapon;
}

void AMCVehicleCombat::ApplyDynamicMaterial()
{
	if (!CarColor)
	{
		return;
	}

	// Create the dynamic material instance if necessary
	if (!DynamicCarMaterial || DynamicCarMaterial->Parent != CarColor)
	{
		DynamicCarMaterial = UMaterialInstanceDynamic::Create(CarColor, this);
	}
}

void AMCVehicleCombat::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!LastDamageMaker)
	{
		return;
	}

	if (DamageMakerCooldownDuration > 0.f)
	{
		DamageMakerCooldownDuration -= DeltaSeconds;
		return;
	}

	LastDamageMaker = nullptr;
}

void AMCVehicleCombat::BeginPlay()
{
	Super::BeginPlay();
}

void AMCVehicleCombat::Destroyed()
{
	Super::Destroyed();
	if (CarAssemblyComponent)
	{
		CarAssemblyComponent->DestroyComponents();
	}
}

UMeshComponent* AMCVehicleCombat::GetAttachmentMesh_Implementation()
{
	if (!CarAssemblyComponent)
	{
		return nullptr;
	}

	if (!CarAssemblyComponent->MeshComponent)
	{
		CarAssemblyComponent->MakeMeshReferences();
	}

	return CarAssemblyComponent->MeshComponent;
}

void AMCVehicleCombat::PostNetReceive()
{
	Super::PostNetReceive();
	if (CarAssemblyComponent)
	{
		CarAssemblyComponent->MakeMeshReferences();
		CarAssemblyComponent->AttachSlots();
	}
}

void AMCVehicleCombat::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (CarAssemblyComponent)
	{
		CarAssemblyComponent->Init();
	}
}
