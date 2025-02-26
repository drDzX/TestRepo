// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MCVehiclePawnBase.h"
#include "MCVehicleSlot.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventorySubsystem.h"
#include "MCVehicleAssemblyComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UMCVehicleAssemblyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMCVehicleAssemblyComponent();

	UFUNCTION(Client, Reliable, BlueprintCallable)
	void AssembleCar();
	UFUNCTION(Server, Reliable)
	void AssembleCar_Server(FCarDataInventory CarDataValue);
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void AssembleCarClient(FCarDataInventory CarDataValue);
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void UpdateCarColor(FCarDataInventory CarDataValue);
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void AdjustAttachment(FCarDataInventory CarDataValue);

	UFUNCTION()
	void MakeMeshReferences();
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void AttachSlots();
	UFUNCTION(BlueprintCallable)
	void DestroyComponents();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Explode();

	UFUNCTION(BlueprintCallable)
	void UpdateDynamicMaterialParameter(FName Name, float Value);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetDamageAllComponents(float NewDamage);

	void Init();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION(Client, Reliable)
	void SetupClient();
	UFUNCTION(Client, Reliable)
	void OnCarDataChanged(FCarDataInventory NewCarData);
	UFUNCTION(Server, Reliable)
	void OnCarColorChanged(FCarDataInventory NewCarData);

	UFUNCTION()
	void OnRep_AssembleComponents(TArray<AMCVehicleSlot*> oldValue);
		UFUNCTION()
	void OnRep_CarData();

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void DestroyComponent(bool bPromoteChildren) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	TMap<FVehicleSlotData, FName> SlotMapping;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	TArray<FVehicleSlotItem> SlotItems;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USkeletalMesh* AttachmentSkeleton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector AttachmentSkeletonOffset;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class UMeshComponent* MeshComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class AMCVehiclePawnBase* OwningCar;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class AVehicleMainMenu* MainMenuCar;
	UPROPERTY(ReplicatedUsing = OnRep_AssembleComponents, VisibleAnywhere, BlueprintReadOnly)
	TArray<AMCVehicleSlot*> AssembleComponents;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	float ExplosionForce = 100000.f;

	UPROPERTY(ReplicatedUsing = OnRep_CarData)
	FCarDataInventory CarData;

	bool bIsEventSubscribed = false;
};
