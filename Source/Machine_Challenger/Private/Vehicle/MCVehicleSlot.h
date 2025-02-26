// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MCVehicleSlot.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Root);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Roof);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Door);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_SideSkirt);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Exhaust);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Mirror);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Fender);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Bumper);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Wheel);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Diffuser);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Hood);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Trunk);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_VehicleSlotId_Grill);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_SideTag_Right);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_SideTag_Left);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_SideTag_Front);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_SideTag_Rear);

USTRUCT()
struct FVehicleSlotServerState
{
	GENERATED_USTRUCT_BODY()

public:
	FVehicleSlotServerState() = default;

	UPROPERTY()
	FName SocketName = NAME_None;
	UPROPERTY()
	bool bIsFlippedRightLeft = false;
};

USTRUCT(BlueprintType)
struct FVehicleSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	FGameplayTag SlotId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	FGameplayTagContainer PositionId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	FName SocketName;

	bool operator==(const FVehicleSlotData& Other) const
	{
		return SlotId == Other.SlotId && PositionId == Other.PositionId;
	}

	bool operator!=(const FVehicleSlotData& Other) const
	{
		return !(*this == Other);
	}
};

USTRUCT(BlueprintType)
struct FVehicleSlotItem
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FName UniqueSlotID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	FGameplayTag SlotId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	TSubclassOf<AMCVehicleSlot> Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	bool IsFlippedLeftRight = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot data")
	bool bIsGarageViewOnly = false;
};

UCLASS()
class AMCVehicleSlot : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMCVehicleSlot();
	UFUNCTION(BlueprintCallable)
	void UpdateColor(UMaterialInterface* Material);

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void Explode(FVector CenterOfExplosion, float Force = 1000000.f);

	UFUNCTION(BlueprintCallable)
	void CreateMeshes();

	UFUNCTION(BlueprintCallable)
	void UpdateDynamicMaterialParameter(FName Name, float Value);

	UFUNCTION(Server, Reliable)
	void SetBaseMaterial(UMaterialInterface* NewMaterial);

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void SetDamage(float NewDamage);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UFUNCTION()
	void Attach();
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot data")
	FVehicleSlotData SlotData;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot data")
	bool bIsForceStaticMesh = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot data")
	UStaticMesh* StaticMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot data")
	USkeletalMesh* SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> StaticMeshComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USkeletalMeshComponent> SkeletalMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot data")
	int CarMaterialIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsStaticMesh = false;
	UPROPERTY(ReplicatedUsing = OnRep_OwningCarCombat, VisibleAnywhere, BlueprintReadOnly)
	class AMCVehicleCombat* OwningCarCombat;

	UPROPERTY(ReplicatedUsing = OnRep_BaseMaterial)
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UFUNCTION(NetMulticast, Reliable)
	void OnRep_BaseMaterial();
	UFUNCTION(NetMulticast, Reliable)
	void OnRep_OwningCarCombat();
	UFUNCTION()
	void OnRep_SlotState();
	UFUNCTION()
	void OnRep_CurrentDamage();
	virtual void PostNetReceive() override;
	UPROPERTY(Transient, BlueprintReadOnly, VisibleAnywhere)
	UMaterialInstanceDynamic* DynamicCarMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float MaxDamage = 1.f;
	UPROPERTY(ReplicatedUsing = OnRep_CurrentDamage, VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	float CurrentDamage = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	bool bIsIgnoringDamageVFX = false;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool bIsUsingSkeletal = false;

	UPROPERTY(ReplicatedUsing = OnRep_SlotState)
	FVehicleSlotServerState SlotState;
};

// Declare the GetTypeHash specialization in the global namespace
FORCEINLINE uint32 GetTypeHash(const FVehicleSlotData& Data)
{
	uint32 Hash = 0;
	Hash = HashCombine(Hash, GetTypeHash(Data.SlotId));

	// Iterate over each tag in the PositionId container and combine its hash
	for (const FGameplayTag& Tag : Data.PositionId)
	{
		Hash = HashCombine(Hash, GetTypeHash(Tag));
	}

	return Hash;
}
