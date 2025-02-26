// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MCVehicleInterface.h"
#include "Abilities/MCAbilitySystemInterface.h"
#include "Vehicle/MCVehiclePawnBase.h"
#include "Weapons/MCWeaponInterface.h"
#include "MCVehicleCombat.generated.h"

class AMCWeaponBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWeaponSpawnedEvent);

/**
 *
 */
UCLASS()
class AMCVehicleCombat : public AMCVehiclePawnBase, public IMCWeaponInterface, public IMCAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMCVehicleCombat(const FObjectInitializer& ObjectInitializer);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual FRotator GetWeaponRotation_Implementation() override;
	virtual FVector GetMuzzleLocation_Implementation(int MuzzleId = 0) override;

	virtual FVector GetWeaponAimTraceLocation_Implementation() override;
	virtual void OnAttackExecuted_Implementation(int MuzzleId) override;
	virtual FVector GetWeaponAimLocation_Implementation() override;
	virtual int GetMuzzleId_Implementation() override;
	virtual void OnAttackBegin_Implementation() override;
	virtual void OnAttackEnd_Implementation() override;
	virtual void OnHealthUpdated_Implementation(float deltaValue) override;

	virtual void OnDamageRecieved_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker) override;

	virtual void OnWeaponSpawned_Implementation(AMCWeaponBase* weapon) override;

	UFUNCTION(NetMulticast, Reliable)
	void OnWeaponSpawnedMulticast();

	UFUNCTION(BlueprintCallable)
	UMaterialInterface* GetMaterialByColorId(const FString& ColorId, bool ConsiderTeamTag = false);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void UpdateCarColor(const FString& ColorId, bool ConsiderTeamTag = false);

	UFUNCTION(NetMulticast, Reliable)
	void OnRep_ColorChanged(UMaterialInterface* newValue);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetCarColor(UMaterialInterface* NewCarColor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Color")
	void OnCarColorChanged();

	UFUNCTION(BlueprintCallable)
	void Explode();

protected:
	virtual AMCWeaponBase* GetMountedWeapon_Implementation() override;
	void ApplyDynamicMaterial();

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void Destroyed() override;
	virtual UMeshComponent* GetAttachmentMesh_Implementation() override;
	virtual void PostNetReceive() override;
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DamageMakerCooldown = 5.f;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Weapons")
	TObjectPtr<AMCWeaponBase> MountedWeapon = nullptr;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AMCVehiclePawnBase> LastDamageMaker = nullptr;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	bool bIsKilled = false;

	UPROPERTY(BlueprintAssignable)
	FWeaponSpawnedEvent WeaponSpawnedEvent;

	UPROPERTY(ReplicatedUsing = OnRep_ColorChanged, VisibleAnywhere, BlueprintReadOnly, Category = "Color")
	UMaterialInterface* CarColor;

	UPROPERTY(Transient, BlueprintReadOnly, VisibleAnywhere)
	UMaterialInstanceDynamic* DynamicCarMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UMCVehicleAssemblyComponent> CarAssemblyComponent;

private:
	float DamageMakerCooldownDuration = 0.f;
};
