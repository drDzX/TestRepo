// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpectatorPawn.h"
#include "MCSpectatorPawn.generated.h"

/**
 * 
 */
UCLASS()
class AMCSpectatorPawn : public ASpectatorPawn
{
	GENERATED_BODY()

public:
	virtual void SetupPlayerInputComponent(UInputComponent* InInputComponent) override;
};
