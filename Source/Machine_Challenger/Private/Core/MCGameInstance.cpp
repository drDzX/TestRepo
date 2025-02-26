// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Inventory/InventorySubsystem.h"
#include "SaveGame/MCSaveGameSubsystem.h"

UMCGameInstance::UMCGameInstance() {}

void UMCGameInstance::LoadingScreenShow()
{
	if (LoadingScreenClass)
	{
		LoadingScreenWidget = CreateWidget<UUserWidget>(this, LoadingScreenClass);
		if (LoadingScreenWidget)
		{
			LoadingScreenWidget->AddToViewport();
		}
	}
}

void UMCGameInstance::LoadingScreenHide()
{
	if (!LoadingScreenWidget)
	{
		return;
	}

	LoadingScreenWidget->RemoveFromParent();
}

void UMCGameInstance::ResetGame()
{
	ResetGameEvent.Broadcast();
}

void UMCGameInstance::ChangeCar(const FName& CarId)
{
	RespawnCarEvent.Broadcast();
	if (const auto& inventory = GetSubsystem<UInventorySubsystem>())
	{
		inventory->UpdateCarData(CarId);
	}

	OnCarIdChanged.Broadcast(CarId);
}

void UMCGameInstance::OnStart()
{
	Super::OnStart();
}
