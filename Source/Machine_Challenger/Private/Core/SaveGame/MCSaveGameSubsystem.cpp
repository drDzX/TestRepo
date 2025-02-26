// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/SaveGame/MCSaveGameSubsystem.h"

#include "Core/MCPlayerState.h"
#include "Core/SaveGame/MCSaveGame.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "SaveGameInterface.h"
#include "Inventory/InventorySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

void UMCSaveGameSubsystem::SetSlotName(FString NewSlotName)
{
	if (NewSlotName.IsEmpty())
	{
		return;
	}

	CurrentSlotName = NewSlotName;
}

void UMCSaveGameSubsystem::WriteSaveGame(FString SlotName)
{
	// Get the local player controller
	APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();
	if (!LocalPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("WriteSaveGame failed: Local PlayerController is null."));
		return;
	}

	// Get the local player's PlayerState
	APlayerState* LocalPlayerState = LocalPlayerController->PlayerState;
	if (!LocalPlayerState || !LocalPlayerState->GetUniqueId().IsValid()) // Ensure UniqueId is valid
	{
		UE_LOG(LogTemp, Warning, TEXT("WriteSaveGame failed: Local PlayerState is invalid or has no UniqueId."));
		return;
	}

	if (!LocalPlayerController->IsLocalController())
	{
		return;
	}

	// Check if running in the editor
	if (GEngine && GEngine->IsEditor())
	{
		SlotName = FPlatformProcess::ComputerName();
	}

	// Set the slot name based on the local player's UniqueId
	SlotName = SlotName.IsEmpty() ? LocalPlayerState->GetUniqueId().ToString() : SlotName; // Replace with your UniqueId retrieval method
	SetSlotName(SlotName);

	// Clear or initialize the save game object
	if (CurrentSaveGame)
	{
		CurrentSaveGame->SavedPlayer.bIsInitialized = false; // Optional: Clear previous save data if relevant
	}
	else
	{
		CurrentSaveGame = Cast<UMCSaveGame>(UGameplayStatics::CreateSaveGameObject(UMCSaveGame::StaticClass()));
		CurrentSaveGame->SavedPlayer.bIsInitialized = true;
	}

	CurrentSaveGame->SavedPlayer.PlayerID = SlotName;
	if (const auto& inventory = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		CurrentSaveGame->SavedPlayer.CarDataID = inventory->SelectedCarId;
		CurrentSaveGame->SavedPlayer.ColorID = inventory->CarData.CarMaterial;
	}

	// Save the game to the specified slot
	if (UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSlotName, 0))
	{
		UE_LOG(LogTemp, Log, TEXT("Game saved successfully to slot: %s"), *CurrentSlotName);
		OnSaveGameWritten.Broadcast(CurrentSaveGame);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("WriteSaveGame failed: Unable to save game to slot: %s"), *CurrentSlotName);
	}
}

void UMCSaveGameSubsystem::LoadSaveGame(FString SlotName)
{
	// Force garbage collection to clear unused objects and free memory
	//GEngine->ForceGarbageCollection(true);

	// Get the local player controller and its PlayerState
	APlayerController* LocalPlayerController = GetWorld()->GetFirstPlayerController();
	if (!LocalPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadSaveGame failed: Local PlayerController is null."));
		return;
	}

	if (!LocalPlayerController->IsLocalController())
	{
		return;
	}

	APlayerState* LocalPlayerState = LocalPlayerController->PlayerState;
	if (!LocalPlayerState || !LocalPlayerState->GetUniqueId().IsValid()) // Ensure UniqueId is valid
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadSaveGame failed: Local PlayerState is invalid or has no UniqueId."));
		return;
	}

	// Check if running in the editor
	if (GEngine && GEngine->IsEditor())
	{
		SlotName = FPlatformProcess::ComputerName();
	}

	// Set the slot name based on the local player's UniqueId
	SlotName = SlotName.IsEmpty() ? LocalPlayerState->GetUniqueId().ToString() : SlotName;
	SetSlotName(SlotName);

	// Check if a save game exists for the current slot name
	const auto& inventory = GetGameInstance()->GetSubsystem<UInventorySubsystem>();
	if (UGameplayStatics::DoesSaveGameExist(CurrentSlotName, 0))
	{
		// Load the save game from the slot
		CurrentSaveGame = Cast<UMCSaveGame>(UGameplayStatics::LoadGameFromSlot(CurrentSlotName, 0));
		if (!CurrentSaveGame)
		{
			if (inventory)
			{
				inventory->SelectedCarId = "MED1";
				inventory->UpdateCarData("MED1");
			}

			UE_LOG(LogTemp, Warning, TEXT("LoadSaveGame failed: Unable to cast to UMCSaveGame."));
			return;
		}

		if (inventory && CurrentSaveGame.Get())
		{
			inventory->SelectedCarId = CurrentSaveGame.Get()->SavedPlayer.CarDataID;
			inventory->UpdateCarData(inventory->SelectedCarId, CurrentSaveGame.Get()->SavedPlayer.ColorID);
		}

		// Trigger the load event
		OnSaveGameLoaded.Broadcast(CurrentSaveGame);
		bIsSaveLoaded = true;
	}
	else
	{
		CurrentSaveGame = Cast<UMCSaveGame>(UGameplayStatics::CreateSaveGameObject(UMCSaveGame::StaticClass()));
		if (inventory)
		{
			inventory->SelectedCarId = "MED1";
			inventory->UpdateCarData("MED1");
		} // If no save game exists, create a new save game instance
		UE_LOG(LogTemp, Log, TEXT("No save game found, a new save game object created."));
	}
}

void UMCSaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}
