// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/WeaponSpawnerComponent.h"

#include "MCWeaponBase.h"
#include "Abilities/MCAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Utils/MCBlueprintLib.h"
#include "Vehicle/MCVehiclePawnBase.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_Pickup1, "GameEffect.PickupEnergy.1");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_Pickup2, "GameEffect.PickupEnergy.2");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_Pickup3, "GameEffect.PickupEnergy.3");
// Sets default values for this component's properties
UWeaponSpawnerComponent::UWeaponSpawnerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	CurrentPickupSetup.Reserve(3);
	CurrentPickupSetup.Add(EWeaponPickupType::Unknown);
	CurrentPickupSetup.Add(EWeaponPickupType::Unknown);
	CurrentPickupSetup.Add(EWeaponPickupType::Unknown);
	// ...
}

// Called when the game starts
void UWeaponSpawnerComponent::BeginPlay()
{
	Super::BeginPlay();
	CarOwner = Cast<AMCVehicleCombat>(GetOwner());
	// ...
}

// Called every frame
void UWeaponSpawnerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UWeaponSpawnerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponSpawnerComponent, CurrentPickupSetup);
}

bool UWeaponSpawnerComponent::IsCombinationAvailableToBuy(const FPickupCombination& PickupCombinationToCheck) const
{
	if (!CarOwner)
	{
		return false;
	}

	const auto& attributes = CarOwner->Attributes.Get();
	if (attributes->PickupEnergy1.GetCurrentValue() < PickupCombinationToCheck.Pickup1)
	{
		return false;
	}
	if (attributes->PickupEnergy2.GetCurrentValue() < PickupCombinationToCheck.Pickup2)
	{
		return false;
	}
	if (attributes->PickupEnergy3.GetCurrentValue() < PickupCombinationToCheck.Pickup3)
	{
		return false;
	}

	return true;
}

bool UWeaponSpawnerComponent::IsCurrentCombinationAvailableToBuy() const
{
	const auto& combination = FPickupCombination::GetCombination(CurrentPickupSetup);
	if (!combination.IsSet())
	{
		return false;
	}

	return IsCombinationAvailableToBuy(combination.GetValue());
}

void UWeaponSpawnerComponent::OnWeaponSpawnedEvent_Implementation(AMCWeaponBase* weapon)
{
	if (!weapon || !GetOwner()->Implements<UMCWeaponInterface>())
	{
		return;
	}

	IMCWeaponInterface::Execute_OnWeaponSpawned(GetOwner(), weapon);
}

void UWeaponSpawnerComponent::ConsumeEnergy_Implementation(TSubclassOf<AMCWeaponBase> weaponClass)
{
	if (!CarOwner || !EnergyGameEffectClass->IsValidLowLevel())
	{
		return;
	}

	const auto& weaponCost = GetWeaponCost(weaponClass);

	UMCBlueprintLib::UpdateGameEffect(CarOwner, -weaponCost.Pickup1, EnergyGameEffectClass, TAG_GameEffect_Pickup1);
	UMCBlueprintLib::UpdateGameEffect(CarOwner, -weaponCost.Pickup2, EnergyGameEffectClass, TAG_GameEffect_Pickup2);
	UMCBlueprintLib::UpdateGameEffect(CarOwner, -weaponCost.Pickup3, EnergyGameEffectClass, TAG_GameEffect_Pickup3);
}

bool UWeaponSpawnerComponent::IsAvailableToBuy(TSubclassOf<AMCWeaponBase> weaponClass)
{
	return IsCombinationAvailableToBuy(GetWeaponCost(weaponClass));
}

FPickupCombination UWeaponSpawnerComponent::GetWeaponCost(TSubclassOf<AMCWeaponBase> weaponClass)
{
	if (WeaponSetup.IsEmpty())
	{
		return FPickupCombination();
	}

	for (const auto& element : WeaponSetup)
	{
		if (element.Value == weaponClass)
		{
			return element.Key;
		}
	}

	return FPickupCombination();
}

void UWeaponSpawnerComponent::TrySpawnWeapon_Implementation(TSubclassOf<AMCWeaponBase> weaponClass, bool CheckCost)
{
	if (UMCBlueprintLib::IsGameplayBlocked(GetWorld()))
	{
		return;
	}

	if (!weaponClass->IsValidLowLevel())
	{
		if (CarOwner->MountedWeapon)
		{
			CarOwner->MountedWeapon->UnEquip();
		}

		return;
	}

	if (CheckCost && !IsAvailableToBuy(weaponClass))
	{
		return;
	}

	SpawnWeaponQueue = weaponClass;
	ConsumeEnergy(weaponClass);
	if (CarOwner->MountedWeapon)
	{
		CarOwner->MountedWeapon.Get()->OnUnEquipEvent.AddDynamic(this, &UWeaponSpawnerComponent::SpawnWeapon);
		CarOwner->MountedWeapon->UnEquip();
	}
	else
	{
		SpawnWeapon();
	}
}

void UWeaponSpawnerComponent::SpawnWeapon_Implementation()
{
	if (UMCBlueprintLib::IsGameplayBlocked(GetWorld()))
	{
		return;
	}

	if (!SpawnWeaponQueue->IsValidLowLevel() || !CarOwner)
	{
		return;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Owner = CarOwner;
	auto* spawnedWeapon = GetWorld()->SpawnActor<AMCWeaponBase>(SpawnWeaponQueue.Get(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	if (ensure(spawnedWeapon))
	{
		spawnedWeapon->Equip(CarOwner);
		CarOwner->MountedWeapon = spawnedWeapon;
		OnWeaponSpawnedEvent(spawnedWeapon);
	}

	SpawnWeaponQueue = nullptr;
}

void UWeaponSpawnerComponent::ConfirmSpawn_Implementation()
{
	if (!CarOwner)
	{
		return;
	}

	const auto& combination = FPickupCombination::GetCombination(CurrentPickupSetup);
	if (!combination.IsSet())
	{
		return;
	}

	const auto& weaponClass = WeaponSetup.Find(combination.GetValue());
	if (!weaponClass)
	{
		return;
	}

	TrySpawnWeapon(weaponClass->Get());
}

void UWeaponSpawnerComponent::UseEnergy1_Implementation()
{
	if (CurrentPickupSetup.IsEmpty())
	{
		return;
	}

	CurrentPickupSetup.Add(EWeaponPickupType::Pickup1);
	CurrentPickupSetup.RemoveAt(0);
}

void UWeaponSpawnerComponent::UseEnergy2_Implementation()
{
	if (CurrentPickupSetup.IsEmpty())
	{
		return;
	}

	CurrentPickupSetup.Add(EWeaponPickupType::Pickup2);
	CurrentPickupSetup.RemoveAt(0);
}

void UWeaponSpawnerComponent::UseEnergy3_Implementation()
{
	if (CurrentPickupSetup.IsEmpty())
	{
		return;
	}

	CurrentPickupSetup.Add(EWeaponPickupType::Pickup3);
	CurrentPickupSetup.RemoveAt(0);
}
