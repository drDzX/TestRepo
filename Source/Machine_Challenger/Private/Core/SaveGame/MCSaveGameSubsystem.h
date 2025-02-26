// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MCSaveGameSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameEvent, class UMCSaveGame*, SaveObject);

/**
 *
 */
UCLASS(meta = (DisplayName = "SaveGame System"))
class UMCSaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

protected:
	FString CurrentSlotName;

	UPROPERTY()
	TObjectPtr<UMCSaveGame> CurrentSaveGame;

public:
	/* Restore spawn transform using stored data per PlayerState after being fully initialized. */
	UFUNCTION(BlueprintCallable)
	void SetSlotName(FString NewSlotName);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void WriteSaveGame(FString SlotName = "");
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadSaveGame(FString SlotName = "");

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameEvent OnSaveGameLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameEvent OnSaveGameWritten;

	UPROPERTY()
	bool bIsSaveLoaded = false;

	/* Initialize Subsystem, good moment to load in SaveGameSettings variables */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
