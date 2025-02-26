// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/InventorySubsystem.h"

#include "Core/MCGameInstance.h"
#include "Core/SaveGame/MCSaveGameSubsystem.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UInventorySubsystem::UpdateCarData(const FName& CarId, FString ColorId)
{
	const auto& mcGameInstance = Cast<UMCGameInstance>(GetGameInstance());
	if (!mcGameInstance)
	{
		return;
	}

	const auto& CarInventory = mcGameInstance->CarInventory;
	if (!CarInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("CarInventory DataTable is null!"));
		return;
	}

	static const FString ContextString(TEXT("Car Inventory Lookup"));
	const auto& CarDataTable = CarInventory->FindRow<FCarInventoryData>(CarId, ContextString);
	if (!CarDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Row with ID %s not found in CarInventory"), *CarId.ToString());
		return;
	}

	FCarDataInventory CarDataInventory;
	CarDataInventory.CarClass = CarDataTable->CarClass;
	CarDataInventory.CarTier = CarDataTable->CarTier;
	CarDataInventory.CarType = CarDataTable->CarType;
	CarDataInventory.AttachmentSkeleton = CarDataTable->AttachmentSkeleton;
	CarDataInventory.AttachmentSkeletonOffset = CarDataTable->AttachmentSkeletonOffset;
	CarDataInventory.CarBody = CarDataTable->CarBody;
	CarDataInventory.SlotItems = CarDataTable->SlotItems;
	CarDataInventory.SlotMapping = CarDataTable->SlotMapping;
	if (ColorId.IsEmpty())
	{
		CarDataInventory.CarMaterial = CarData.CarMaterial.IsEmpty() ? CarDataTable->CarMaterial : CarData.CarMaterial;
	}
	else
	{
		CarDataInventory.CarMaterial = ColorId;
	}

	CarDataInventory.BodyMaterialIndex = CarDataTable->BodyMaterialIndex;
	CarDataInventory.ForceMaterial = CarDataTable->ForceMaterial;
	CarDataInventory.UniqueCarName = CarId;
	SelectedCarId = CarId;
	CarData = CarDataInventory;
	if (const auto& saveSystem = GetGameInstance()->GetSubsystem<UMCSaveGameSubsystem>())
	{
		saveSystem->WriteSaveGame();
	}

	OnCarDataChangedEvent.Broadcast(CarDataInventory);
}

void UInventorySubsystem::UpdateCarColor(FString ColorId)
{
	const auto& mcGameInstance = Cast<UMCGameInstance>(GetGameInstance());
	if (!mcGameInstance)
	{
		return;
	}

	const auto& CarInventory = mcGameInstance->CarInventory;
	if (!CarInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("CarInventory DataTable is null!"));
		return;
	}

	if (CarData.CarMaterial == ColorId)
	{
		return;
	}

	CarData.CarMaterial = ColorId;
	OnCarColorChangedEvent.Broadcast(CarData);

	if (const auto& saveSystem = GetGameInstance()->GetSubsystem<UMCSaveGameSubsystem>())
	{
		saveSystem->WriteSaveGame();
	}
}

TArray<FVehicleSlotItem> UInventorySubsystem::GetAvailableSlots(FGameplayTag SlotPosition, TSet<ECarTier> SupportedTiers, TSet<ECarType> SupportedCarType)
{
	TArray<FVehicleSlotItem> Result;

	// Ensure we have a valid GameInstance
	const auto* mcGameInstance = Cast<UMCGameInstance>(GetGameInstance());
	if (!mcGameInstance)
	{
		return Result;
	}

	// Ensure SlotInventory is valid
	const auto* SlotInventory = mcGameInstance->SlotInventory;
	if (!SlotInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("SlotInventory DataTable is null!"));
		return Result;
	}

	const FString Context;
	TArray<FName> RowNames = SlotInventory->GetRowNames(); // Get row names from the DataTable

	for (const FName& RowName : RowNames)
	{
		const FCarInventorySlotData* Slot = SlotInventory->FindRow<FCarInventorySlotData>(RowName, Context);
		if (!Slot)
		{
			continue;
		}

		// Skip locked slots or those that don't match the position
		if (!Slot->bIsUnlocked || Slot->SlotPosition != SlotPosition)
		{
			continue;
		}

		// Check for tier and type matches
		const bool bHasMatchingTier = Slot->SupportedTiers.Intersect(SupportedTiers).Num() > 0;
		const bool bHasMatchingType = Slot->SupportedCarType.Intersect(SupportedCarType).Num() > 0;

		if (bHasMatchingTier && bHasMatchingType)
		{
			FVehicleSlotItem SlotData;
			SlotData.Slot = Slot->Slot;
			SlotData.SlotId = SlotPosition;
			SlotData.IsFlippedLeftRight = Slot->IsFlippedLeftRight;

			// Assign the UniqueSlotID from the row name
			SlotData.UniqueSlotID = RowName;

			Result.Add(SlotData);
		}
	}

	return Result;
}

TArray<FCarDataInventory> UInventorySubsystem::GetAvailableCars()
{
	TArray<FCarDataInventory> AvailableCars;

	// Ensure we have a valid GameInstance
	const auto* mcGameInstance = Cast<UMCGameInstance>(GetGameInstance());
	if (!mcGameInstance)
	{
		return AvailableCars;
	}

	const auto* CarInventory = mcGameInstance->CarInventory;
	if (!CarInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("SlotInventory DataTable is null!"));
		return AvailableCars;
	}

	const FString Context;
	TArray<FName> RowNames = CarInventory->GetRowNames(); // Get row names from the DataTable

	for (const FName& RowName : RowNames)
	{
		const FCarInventoryData* Car = CarInventory->FindRow<FCarInventoryData>(RowName, Context);
		if (!Car)
		{
			continue;
		}

		FCarDataInventory CarDataInventory;
		CarDataInventory.CarClass = Car->CarClass;
		CarDataInventory.CarTier = Car->CarTier;
		CarDataInventory.CarType = Car->CarType;
		CarDataInventory.AttachmentSkeleton = Car->AttachmentSkeleton;
		CarDataInventory.AttachmentSkeletonOffset = Car->AttachmentSkeletonOffset;
		CarDataInventory.CarBody = Car->CarBody;
		CarDataInventory.SlotItems = Car->SlotItems;
		CarDataInventory.SlotMapping = Car->SlotMapping;
		CarDataInventory.CarMaterial = Car->CarMaterial;
		CarDataInventory.BodyMaterialIndex = Car->BodyMaterialIndex;
		CarDataInventory.ForceMaterial = Car->ForceMaterial;
		CarDataInventory.Thumbnail = Car->Thumbnail;
		CarDataInventory.CodeName = Car->CodeName;
		CarDataInventory.UniqueCarName = RowName;

		AvailableCars.Add(CarDataInventory);
	}

	return AvailableCars;
}

FVehicleSlotItem UInventorySubsystem::GetSlotById(FName SlotId)
{
	FVehicleSlotItem Result;
	// Ensure we have a valid GameInstance
	const auto* mcGameInstance = Cast<UMCGameInstance>(GetGameInstance());
	if (!mcGameInstance)
	{
		return Result;
	}

	// Ensure SlotInventory is valid
	const auto* SlotInventory = mcGameInstance->SlotInventory;
	if (!SlotInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("SlotInventory DataTable is null!"));
		return Result;
	}

	const FString Context;
	const FCarInventorySlotData* SlotRow = SlotInventory->FindRow<FCarInventorySlotData>(SlotId, Context);
	if (SlotRow)
	{
		Result.Slot = SlotRow->Slot;
		Result.SlotId = SlotRow->SlotPosition;
		Result.IsFlippedLeftRight = SlotRow->IsFlippedLeftRight;
		Result.UniqueSlotID = SlotId;
	}

	return Result;
}

bool UInventorySubsystem::IsCarDataInited()
{
	return !CarData.SlotItems.IsEmpty();
}
