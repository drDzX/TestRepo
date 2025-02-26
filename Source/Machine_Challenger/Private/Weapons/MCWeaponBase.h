// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "MCWeaponInterface.h"
#include "NativeGameplayTags.h"
#include "GameFramework/Actor.h"
#include "MCWeaponBase.generated.h"

struct FAbilityWithTag;
struct FGameplayAbilitySpecHandle;
class UMCGameplayAbility;
class UGameplayEffect;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquip);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnEquip);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Weapon_Activated);

UCLASS()
class AMCWeaponBase : public APawn, public IMCWeaponInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMCWeaponBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Equip(AMCVehiclePawnBase* ownerCar);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void UnEquip();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void AssignAbilities();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void RemoveAbilities();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void AimAt(FVector AimLocation);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void AimAtServer(FVector AimLocation);

	UFUNCTION(BlueprintCallable, Server, Unreliable)
	void UpdateWeaponAimTarget();

	UFUNCTION(BlueprintCallable, NetMulticast, Unreliable)
	void UpdateWeaponAimTargetClient();

	UFUNCTION(BlueprintImplementableEvent)
	void OnEquipped();

	UFUNCTION(BlueprintImplementableEvent)
	void OnUnEquipped();

	virtual void Destroyed() override;

	virtual float GetWeaponSpread_Implementation() override;

	void ConstructDynMaterials();
	UFUNCTION(Client, Unreliable)
	void UpdateDynMaterialOpacity(float NewOpacity);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(Server, Reliable)
	void AssignWeaponAttributes();

private:
	UFUNCTION()
	void AttachToOwner();
	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void UpdateWeaponRotation(float dt);
	UFUNCTION(Client, Reliable)
	void UpdateAutoAimLocation();

public:
	virtual void OnAttackBegin_Implementation() override;
	virtual void OnAttackEnd_Implementation() override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponMesh")
	USkeletalMeshComponent* WeaponMesh;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Attachment")
	AMCVehiclePawnBase* OwningCar;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
	FName AttachSocketName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<FAbilityWithTag> WeaponAbilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayEffect>> WeaponAttributes;
	// Handles to the granted abilities.
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	bool bIsAutoAiming = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	float AutoAimTraceDistance = 100000.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	TEnumAsByte<ECollisionChannel> AutoAimTraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	FRotator RotationLowerLimit = FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	FRotator RotationUpperLimit = FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	float AimAtCarSafePitch = -20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Aiming")
	FVector2D CrosshairOffset = FVector2D(0.f, -150.f);

	//Offset from the root of the actor to place where we get aim.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	FVector AimLocationOffset = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FRotator WeaponRotation;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FRotator AimTargetRotation;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector AimTargetLocation;
	UPROPERTY(BlueprintReadOnly, Replicated, VisibleAnywhere)
	FVector WeaponAimLocation;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector WeaponAimLocationClient;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector WeaponTraceAimPoint;
	FHitResult selfHitTraceData;
	UPROPERTY(Replicated, BlueprintReadWrite, EditAnywhere)
	bool bIsFiring = false;
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FOnEquip OnEquipEvent;
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FOnUnEquip OnUnEquipEvent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bIsBehindCamera;
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	FName OpacityParameterName = "Opacity";
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float BehindCameraOpacity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTag WeaponIdTag;
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynMaterials;
};
