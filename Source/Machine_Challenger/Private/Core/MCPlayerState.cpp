// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCPlayerState.h"

#include "MCGameInstance.h"
#include "MCGameState.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/InventorySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "SaveGame/MCSaveGame.h"
#include "Vehicle/MCVehiclePawnBase.h"
#include "Online/CoreOnline.h"
#include "SaveGame/MCSaveGameSubsystem.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Team_A, "Team.A");
UE_DEFINE_GAMEPLAY_TAG(TAG_Team_B, "Team.B");
UE_DEFINE_GAMEPLAY_TAG(TAG_Team_Neutral, "Team.Neutral");

UE_DEFINE_GAMEPLAY_TAG(TAG_PlayerState_Alive, "PlayerState.Alive");
UE_DEFINE_GAMEPLAY_TAG(TAG_PlayerState_Dead, "PlayerState.Dead");

void AMCPlayerState::RenamePlayer_Implementation(const FString& NewName)
{
	SetPlayerName(NewName);
	OnPlayerInfoChanged.Broadcast();
}

void AMCPlayerState::ResetKillScore_Implementation()
{
	KillCollected = 0;
}

void AMCPlayerState::ChangePlayerTeam_Implementation(const FGameplayTag& newTeam)
{
	PlayerTeam = newTeam;
	if (HasAuthority())
	{
		OnPlayerInfoChanged.Broadcast();
	}
}

void AMCPlayerState::ChangePlayerState_Implementation(const FGameplayTag& newState)
{
	PlayerState = newState;
	if (HasAuthority())
	{
		OnPlayerInfoChanged.Broadcast();
	}
}

void AMCPlayerState::AddDeathScore_Implementation(const int Value)
{
	Deaths += Value;
}

void AMCPlayerState::ResetDeathScore_Implementation()
{
	Deaths = 0;
}

void AMCPlayerState::AddAssistScore_Implementation(const int Value)
{
	Assists += Value;
}

void AMCPlayerState::ResetAssistScore_Implementation()
{
	Assists = 0;
}

void AMCPlayerState::RemoveAssistInput(AMCPlayerState* playerState)
{
	AssistActors.RemoveAll([&](const FDamageRecord& Record)
	{
		return Record.PlayerState == playerState;
	});
}

void AMCPlayerState::AssistDamageDone(AMCPlayerState* DamageMaker)
{
	// Check if the player is already in the records
	FDamageRecord* ExistingRecord = AssistActors.FindByPredicate([&](const FDamageRecord& Record)
	{
		return Record.PlayerState == DamageMaker;
	});

	if (ExistingRecord)
	{
		// Reset the timer
		GetWorld()->GetTimerManager().ClearTimer(ExistingRecord->TimerHandle);
	}
	else
	{
		// Add new record
		AssistActors.Add(FDamageRecord(DamageMaker));
		ExistingRecord = &AssistActors.Last();
	}

	// Set a timer to remove the record after 5 seconds
	GetWorld()->GetTimerManager().SetTimer(
		ExistingRecord->TimerHandle,
		[this, DamageMaker]()
		{
			RemoveAssistInput(DamageMaker);
		},
		AssistTimer, false);
}

void AMCPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(AMCPlayerState, DamageDone, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AMCPlayerState, KillCollected, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AMCPlayerState, Assists, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AMCPlayerState, Deaths, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AMCPlayerState, PlayerTeam, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(AMCPlayerState, PlayerState, SharedParams);
}

void AMCPlayerState::AddKillScore_Implementation(const int AddKill)
{
	KillCollected += AddKill;
}

void AMCPlayerState::OnReportDamageDone_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker, AActor* damageReciever)
{
	if (const auto& dmgReciever = Cast<AMCVehiclePawnBase>(damageReciever))
	{
		if (const auto& mcPlayerState = Cast<AMCPlayerState>(dmgReciever->GetPlayerState()))
		{
			mcPlayerState->AssistDamageDone(this);
		}
	}

	IMCAbilitySystemInterface::OnReportDamageDone_Implementation(Damage, isCritical, damageLocation, damageMaker, damageReciever);
	AddDamageScore(Damage);
}

void AMCPlayerState::OnReportPlayerRespawned_Implementation()
{
	IMCAbilitySystemInterface::OnReportPlayerRespawned_Implementation();
	ChangePlayerState(TAG_PlayerState_Alive);
}

void AMCPlayerState::OnReportKillDone_Implementation(APlayerState* damageMaker, APlayerState* killedActor)
{
	IMCAbilitySystemInterface::OnReportKillDone_Implementation(damageMaker, killedActor);
	if (killedActor == this)
	{
		for (const auto& assistActor : AssistActors)
		{
			if (assistActor.PlayerState == damageMaker)
			{
				continue;
			}

			assistActor.PlayerState->AddAssistScore(1);
		}

		AssistActors.Empty();

		AddDeathScore(1);
		ChangePlayerState(TAG_PlayerState_Dead);
		return;
	}

	AddKillScore(1);
	if (const auto& world = GetWorld())
	{
		if (world->GetGameState()->Implements<UMCAbilitySystemInterface>())
		{
			IMCAbilitySystemInterface::Execute_OnReportKillDone(world->GetGameState(), damageMaker, killedActor);
		}
	}
}

void AMCPlayerState::BeginPlay()
{
	Super::BeginPlay();

	if (const auto& MCGameInstace = Cast<UMCGameInstance>(GetGameInstance()))
	{
		MCGameInstace->ResetGameEvent.AddDynamic(this, &AMCPlayerState::ResetDamageScore);
		MCGameInstace->ResetGameEvent.AddDynamic(this, &AMCPlayerState::ResetKillScore);
		MCGameInstace->ResetGameEvent.AddDynamic(this, &AMCPlayerState::ResetDeathScore);
		MCGameInstace->ResetGameEvent.AddDynamic(this, &AMCPlayerState::ResetAssistScore);
	}

	const auto& world = GetWorld();
	if (!world)
	{
		return;
	}

	const auto& gameState = Cast<AMCGameState>(world->GetGameState());
	if (!gameState)
	{
		return;
	}

	OnPlayerInfoChanged.AddDynamic(gameState, &AMCGameState::BroadcastInfoChanged);
}

void AMCPlayerState::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AddTag(PlayerTeam);
	TagContainer.AddTag(PlayerState);
}

void AMCPlayerState::OnRep_PlayerTeam(const FGameplayTag& oldValue)
{
	OnPlayerInfoChanged.Broadcast();
}

void AMCPlayerState::OnRep_PlayerState(const FGameplayTag& oldValue)
{
	OnPlayerInfoChanged.Broadcast();
}

void AMCPlayerState::ResetDamageScore_Implementation()
{
	DamageDone = 0.f;
}

void AMCPlayerState::AddDamageScore_Implementation(const float AddDamage)
{
	DamageDone += AddDamage;
}
