// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/MCAbilitySystemInterface.h"
#include "GameFramework/GameModeBase.h"
#include "MCGameModeBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnReleased, AController*, PlayerController);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFindStartTriggered);

/**
 *
 */
UCLASS()
class AMCGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

public:
	FOnSpawnReleased OnSpawnReleasedEvent;
	FOnFindStartTriggered OnFindStartTriggeredEvent;

private:
	UPROPERTY()
	TMap<AController*, AActor*> PlayerSpawnMap;
};
