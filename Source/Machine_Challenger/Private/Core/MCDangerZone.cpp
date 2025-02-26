// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCDangerZone.h"

#include "MCArenaGameState.h"

// Sets default values
AMCDangerZone::AMCDangerZone()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AMCDangerZone::DangerZoneActivationChange_Implementation(bool IsActivated)
{
	const auto& world = GetWorld();
	if (!world)
	{
		return;
	}

	const auto& gameState = Cast<AMCArenaGameState>(world->GetGameState());
	if (!gameState)
	{
		return;
	}

	gameState->DangerZoneActivationChange(IsActivated);
}

// Called when the game starts or when spawned
void AMCDangerZone::BeginPlay()
{
	Super::BeginPlay();
}

void AMCDangerZone::CreateEffectHandle()
{ }

// Called every frame
// Called every frame
void AMCDangerZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
