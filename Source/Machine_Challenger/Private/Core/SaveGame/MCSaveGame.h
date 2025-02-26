// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Vehicle/MCVehiclePawnBase.h"
#include "MCSaveGame.generated.h"

USTRUCT()
struct FPlayerSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsInitialized = false;

	UPROPERTY()
	FString PlayerID;

	UPROPERTY()
	FName CarDataID;

	UPROPERTY()
	FString ColorID;
};

/**
 *
 */
UCLASS()
class UMCSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPlayerSaveData SavedPlayer;
};
