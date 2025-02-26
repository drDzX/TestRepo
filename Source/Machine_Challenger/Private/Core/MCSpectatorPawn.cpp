// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCSpectatorPawn.h"

#include "Inputs/MCEnhInputComponent.h"

void AMCSpectatorPawn::SetupPlayerInputComponent(UInputComponent* InInputComponent)
{
	Super::SetupPlayerInputComponent(InInputComponent);
	UMCEnhInputComponent* mcEnhancedInputComponent = Cast<UMCEnhInputComponent>(InInputComponent);
	if (!mcEnhancedInputComponent)
	{
		return;
	}
}
