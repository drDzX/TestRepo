// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCPlayerController.h"

#include "MCGameInstance.h"
#include "MCPlayerState.h"
#include "Abilities/MCAttributeSet.h"
#include "GameFramework/GameModeBase.h"
#include "Inventory/InventorySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "SaveGame/MCSaveGameSubsystem.h"
#include "Vehicle/MCVehiclePawnBase.h"

void AMCPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCPlayerController, MainCar);
	DOREPLIFETIME(AMCPlayerController, MCTags);
	DOREPLIFETIME(AMCPlayerController, RespawnCarClass);
}

FGameplayTag AMCPlayerController::GetPlayerTeam_Implementation()
{
	const auto& playerState = GetPlayerState<AMCPlayerState>();
	if (!playerState)
	{
		return TAG_Team_Neutral;
	}

	return playerState->PlayerTeam;
}

void AMCPlayerController::ServerChangeTeam_Implementation(int TeamIndex)
{
	const auto& playerState = GetPlayerState<AMCPlayerState>();
	if (!playerState)
	{
		return;
	}

	playerState->ChangePlayerTeam(TeamIndex == 1 ? TAG_Team_B : TAG_Team_A);
}

void AMCPlayerController::ChangeRespawnCarClass_Implementation(TSubclassOf<AMCVehiclePawnBase> CarClass)
{
	RespawnCarClass = CarClass;
	if (bIsMarkedForRespawn)
	{
		bIsMarkedForRespawn = false;
		Respawn();
	}
}

void AMCPlayerController::ChangeTeam(int TeamIndex)
{
	ServerChangeTeam(TeamIndex);
	Respawn();
}

void AMCPlayerController::BeginPlay()
{
	if (HasAuthority())
	{
		bIsMarkedForRespawn = true;
	}

	if (IsLocalController())
	{
		SetupClient();

		if (const auto& saveSystem = GetGameInstance()->GetSubsystem<UMCSaveGameSubsystem>())
		{
			saveSystem->LoadSaveGame();
		}

		if (const auto& inventory = GetGameInstance()->GetSubsystem<UInventorySubsystem>(); inventory && PlayerState)
		{
			ChangeRespawnCarClass(inventory->CarData.CarClass);
		}
	}

	Super::BeginPlay();
}

void AMCPlayerController::BeginPlayingState()
{
	Super::BeginPlayingState();
	if (!MainCar && HasAuthority())
	{
		bIsMarkedForRespawn = true;
	}
}

void AMCPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AMCPlayerController::OnRep_MainCar(AMCVehiclePawnBase* oldValue)
{
	if (!MainCar)
	{
		ChangeState(NAME_Spectating);
	}
}

void AMCPlayerController::OnCarDataChanged_Implementation(FCarDataInventory NewCarData)
{
	ChangeRespawnCarClass(NewCarData.CarClass);
}

void AMCPlayerController::OnRespawnCarEvent_Implementation()
{
	bIsMarkedForRespawn = true;
}

void AMCPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	if (const auto& saveSystem = GetGameInstance()->GetSubsystem<UMCSaveGameSubsystem>())
	{
		saveSystem->LoadSaveGame();
	}

	if (const auto& inventory = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		ChangeRespawnCarClass(inventory->CarData.CarClass);
	}
}

void AMCPlayerController::TransferAttributes_Implementation(UMCAttributeSet* oldAttributes, UMCAttributeSet* newAttributes, FGameplayTagContainer transferAttributes)
{
	if (!ensure(oldAttributes) || !ensure(newAttributes))
	{
		return;
	}

	for (const auto& tag : transferAttributes)
	{
		if (tag == TAG_Attribute_Health)
		{
			newAttributes->SetHealth(oldAttributes->GetHealth());
			continue;;
		}

		FMessageLog(FName("Attribute System")).Error(FText::FromString("TransferAttributes function is called with the FGameplayTag that is not referenced to the value. Check code!"));
	}
}

void AMCPlayerController::Respawn_Implementation(FGameplayTagContainer transferAttributes, TSubclassOf<AMCVehiclePawnBase> respawnClass)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ensure(GetWorld()))
	{
		return;
	}

	TSubclassOf<AMCVehiclePawnBase> classToSpawn;
	if (!respawnClass->IsValidLowLevel())
	{
		classToSpawn = RespawnCarClass;
	}
	else
	{
		classToSpawn = respawnClass;
	}

	if (!classToSpawn->IsValidLowLevel())
	{
		return;
	}

	if (GetStateName() == NAME_Spectating)
	{
		ChangeState(NAME_Playing);
	}

	TObjectPtr<UMCAttributeSet> oldAttributes = nullptr;
	if (!transferAttributes.IsEmpty() && MainCar)
	{
		oldAttributes = MainCar->Attributes;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const auto& playerStart = GetWorld()->GetAuthGameMode()->FindPlayerStart(this);
	const FRotator Rotation = playerStart ? playerStart->GetActorRotation() : FRotator::ZeroRotator;
	const auto& Location = playerStart ? playerStart->GetActorLocation() : GetPawn()->GetActorLocation();
	const auto& spawnedActor = GetWorld()->SpawnActor<AMCVehiclePawnBase>(classToSpawn, Location, Rotation, SpawnInfo);

	if (!spawnedActor)
	{
		//Fail safe
		return;
	}

	MainCar = spawnedActor;
	UnPossess();
	verify(MainCar);
	Possess(MainCar);

	if (!transferAttributes.IsEmpty() && oldAttributes)
	{
		TransferAttributes(oldAttributes, MainCar->Attributes, transferAttributes);
	}

	const auto& playerState = GetPlayerState<APlayerState>();
	if (playerState && playerState->Implements<UMCAbilitySystemInterface>())
	{
		IMCAbilitySystemInterface::Execute_OnReportPlayerRespawned(playerState);
	}
}

void AMCPlayerController::SetMainCar_Implementation(AMCVehiclePawnBase* newCar)
{
	MainCar = newCar;
}

void AMCPlayerController::MoveToStart_Implementation()
{
	if (!MainCar)
	{
		Respawn();
		return;
	}

	if (GetStateName() == NAME_Spectating)
	{
		ChangeState(NAME_Playing);
	}

	if (!MainCar)
	{
		return;
	}

	const auto& playerStart = GetWorld()->GetAuthGameMode()->FindPlayerStart(this);
	const auto& Location = playerStart ? playerStart->GetActorLocation() : GetPawn()->GetActorLocation();
	const auto& Rotation = playerStart ? playerStart->GetActorRotation() : FRotator::ZeroRotator;
	MainCar->TeleportTo(Location, Rotation);
	// TODO FIX:
	// MainCar->GetVehicleMovement()->SetParked(true);
}

void AMCPlayerController::ChangeToSpectator_Implementation()
{
	SetMainCar(nullptr);
	ChangeState(NAME_Spectating);
}

void AMCPlayerController::SetupClient_Implementation()
{
	if (const auto& gameInstance = GetGameInstance<UMCGameInstance>())
	{
		gameInstance->RespawnCarEvent.AddDynamic(this, &AMCPlayerController::OnRespawnCarEvent);
	}
	else
	{
		return;
	}

	if (const auto& inventory = GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		inventory->OnCarDataChangedEvent.AddDynamic(this, &AMCPlayerController::OnCarDataChanged);
	}
}

void AMCPlayerController::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = MCTags;
}
