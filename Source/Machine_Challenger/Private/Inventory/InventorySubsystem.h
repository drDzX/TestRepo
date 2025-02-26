// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Vehicle/MCVehicleSlot.h"
#include "InventorySubsystem.generated.h"

class AMCVehiclePawnBase;
struct FCarDataInventory;

UENUM(BlueprintType)
enum class ECarTier : uint8
{
	Tier1,
	Tier2,
	Tier3
};

UENUM(BlueprintType)
enum class ECarType : uint8
{
	Light,
	Medium,
	Heavy
};

USTRUCT(BlueprintType)
struct FCarInventorySlotData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	FGameplayTag SlotPosition;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vehicle slot")
	TSubclassOf<AMCVehicleSlot> Slot;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	TSet<ECarTier> SupportedTiers;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	TSet<ECarType> SupportedCarType;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	bool bIsUnlocked = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle slot")
	bool IsFlippedLeftRight = false;
};

USTRUCT(BlueprintType)
struct FCarPaintData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FString ColorId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	UMaterialInterface* Material;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FGameplayTag TeamTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	FColor WidgetColor;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	bool bShowInGarage = true;
};

USTRUCT(BlueprintType)
struct FCarInventoryData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
	TSubclassOf<AMCVehiclePawnBase> CarClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	ECarTier CarTier = ECarTier::Tier1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	ECarType CarType = ECarType::Medium;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Attachment")
	class USkeletalMesh* AttachmentSkeleton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Attachment")
	FVector AttachmentSkeletonOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	FString CarMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	int BodyMaterialIndex;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	FString ForceMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Parts")
	UStaticMesh* CarBody;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Parts")
	TArray<FVehicleSlotData> SlotMapping;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Parts")
	TArray<FVehicleSlotItem> SlotItems;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Data")
	UTexture2D* Thumbnail;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Data")
	FName CodeName;
};

USTRUCT(BlueprintType)
struct FCarDataInventory
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
	TSubclassOf<AMCVehiclePawnBase> CarClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	ECarTier CarTier = ECarTier::Tier1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car")
	ECarType CarType = ECarType::Medium;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Attachment")
	class USkeletalMesh* AttachmentSkeleton;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car Attachment")
	FVector AttachmentSkeletonOffset;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	FString CarMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	int BodyMaterialIndex;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Material")
	FString ForceMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Parts")
	TSoftObjectPtr<UStaticMesh> CarBody;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Parts")
	TArray<FVehicleSlotData> SlotMapping;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Parts")
	TArray<FVehicleSlotItem> SlotItems;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Data")
	TSoftObjectPtr<UTexture2D> Thumbnail;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Data")
	FName CodeName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Car Data")
	FName UniqueCarName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCarDataChanged, FCarDataInventory, CarData);

/**
 *
 */
UCLASS()
class UInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UFUNCTION(BlueprintCallable)
	void UpdateCarData(const FName& CarId, FString ColorId = "");
	UFUNCTION(BlueprintCallable)
	void UpdateCarColor(FString ColorId = "");
	UFUNCTION(BlueprintCallable)
	TArray<FVehicleSlotItem> GetAvailableSlots(FGameplayTag SlotPosition, TSet<ECarTier> SupportedTiers, TSet<ECarType> SupportedCarType);
	UFUNCTION(BlueprintCallable)
	TArray<FCarDataInventory> GetAvailableCars();
	UFUNCTION(BlueprintCallable)
	FVehicleSlotItem GetSlotById(FName SlotId);

	UFUNCTION()
	bool IsCarDataInited();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FName SelectedCarId;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	FCarDataInventory CarData;

	UPROPERTY(BlueprintAssignable)
	FCarDataChanged OnCarDataChangedEvent;
	UPROPERTY(BlueprintAssignable)
	FCarDataChanged OnCarColorChangedEvent;
};
