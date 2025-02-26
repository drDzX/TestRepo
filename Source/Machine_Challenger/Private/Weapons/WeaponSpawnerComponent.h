// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/MCWeaponBase.h"
#include "Vehicle/MCVehicleCombat.h"
#include "WeaponSpawnerComponent.generated.h"

UENUM(BlueprintType)
enum class EWeaponPickupType : uint8
{
	Unknown,
	Pickup1,
	Pickup2,
	Pickup3
};

USTRUCT(BlueprintType)
struct FPickupCombination
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Pickup1 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Pickup2 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Pickup3 = 0;

	static TOptional<FPickupCombination> GetCombination(const TArray<EWeaponPickupType>& Combination)
	{
		TOptional<FPickupCombination> SetupCombination;
		if (Combination.IsEmpty())
		{
			return SetupCombination;
		}

		SetupCombination = FPickupCombination();
		for (const auto& element : Combination)
		{
			switch (element)
			{
				case EWeaponPickupType::Unknown:
					break;
				case EWeaponPickupType::Pickup1:
					SetupCombination->Pickup1++;
					break;
				case EWeaponPickupType::Pickup2:
					SetupCombination->Pickup2++;
					break;
				case EWeaponPickupType::Pickup3:
					SetupCombination->Pickup3++;
					break;
				default: ;
			}
		}

		return SetupCombination;
	}

	friend bool operator==(const FPickupCombination& Lhs, const FPickupCombination& RHS)
	{
		return Lhs.Pickup1 == RHS.Pickup1
				&& Lhs.Pickup2 == RHS.Pickup2
				&& Lhs.Pickup3 == RHS.Pickup3;
	}

	friend bool operator!=(const FPickupCombination& Lhs, const FPickupCombination& RHS) { return !(Lhs == RHS); }

	// Implement the GetTypeHash function to make the struct hashable.
	friend uint32 GetTypeHash(const FPickupCombination& Key)
	{
		// Combine the hash values of the three integer properties.
		return HashCombine(HashCombine(GetTypeHash(Key.Pickup1), GetTypeHash(Key.Pickup2)), GetTypeHash(Key.Pickup3));
	}
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UWeaponSpawnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UWeaponSpawnerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void UseEnergy1();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void UseEnergy2();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void UseEnergy3();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ConfirmSpawn();

	UFUNCTION(Server, Reliable)
	void SpawnWeapon();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void TrySpawnWeapon(TSubclassOf<AMCWeaponBase> weaponClass, bool CheckCost = true);

	UFUNCTION(BlueprintCallable)
	bool IsAvailableToBuy(TSubclassOf<AMCWeaponBase> weaponClass);
	UFUNCTION(BlueprintCallable)
	FPickupCombination GetWeaponCost(TSubclassOf<AMCWeaponBase> weaponClass);
	UFUNCTION(BlueprintCallable)
	bool IsCombinationAvailableToBuy(const FPickupCombination& PickupCombinationToCheck) const;
	UFUNCTION(BlueprintCallable)
	bool IsCurrentCombinationAvailableToBuy() const;

	UFUNCTION(Server, Reliable)
	void ConsumeEnergy(TSubclassOf<AMCWeaponBase> weaponClass);

private:
	UFUNCTION(NetMulticast, Reliable)
	void OnWeaponSpawnedEvent(AMCWeaponBase* weapon);

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FPickupCombination, TSubclassOf<AMCWeaponBase>> WeaponSetup;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	TArray<EWeaponPickupType> CurrentPickupSetup;

	UPROPERTY()
	TSubclassOf<AMCWeaponBase> SpawnWeaponQueue;
	UPROPERTY()
	TObjectPtr<AMCVehicleCombat> CarOwner;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EnergyGameEffectClass;
};
