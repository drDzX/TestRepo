// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCGameState.h"

#include "MCPlayerState.h"
#include "Net/UnrealNetwork.h"

void AMCGameState::OnReportKillDone_Implementation(APlayerState* damageMaker, APlayerState* killedActor)
{
	IMCAbilitySystemInterface::OnReportKillDone_Implementation(damageMaker, killedActor);
	if (!HasAuthority())
	{
		return;
	}

	ReportKillDoneMulticast(damageMaker, killedActor);
}

void AMCGameState::RestartGame()
{
	GetWorld()->ServerTravel("?Restart", true);
}

void AMCGameState::BroadcastInfoChanged_Implementation()
{
	OnGameInfoChanged.Broadcast();
}

void AMCGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCGameState, GamePhase);
}

void AMCGameState::ChangeGamePhase_Implementation(FGameplayTag newState)
{
	GamePhase = newState;
	BroadcastGameChanged(newState);
}

FGameplayTagContainer AMCGameState::GetDeadTeams()
{
	int teamA = 0;
	int teamB = 0;
	for (const auto& player : PlayerArray)
	{
		const auto& DWPlayer = Cast<AMCPlayerState>(player);
		if (!DWPlayer)
		{
			continue;
		}

		FGameplayTagContainer tagContainer;
		tagContainer.AddTagFast(TAG_Team_A);
		tagContainer.AddTagFast(TAG_PlayerState_Alive);
		if (DWPlayer->HasAllMatchingGameplayTags(tagContainer))
		{
			teamA++;
			continue;
		}

		tagContainer.Reset();
		tagContainer.AddTagFast(TAG_Team_B);
		tagContainer.AddTagFast(TAG_PlayerState_Alive);
		if (DWPlayer->HasAllMatchingGameplayTags(tagContainer))
		{
			teamB++;
		}
	}

	FGameplayTagContainer returnValue;
	if (teamA == 0)
	{
		returnValue.AddTagFast(TAG_Team_A);
	}

	if (teamB == 0)
	{
		returnValue.AddTagFast(TAG_Team_B);
	}

	return returnValue;
}

void AMCGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	OnGameInfoChanged.Broadcast();
}

void AMCGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	OnGameInfoChanged.Broadcast();
}

void AMCGameState::BroadcastGameChanged_Implementation(FGameplayTag newPhase)
{
	OnStateChangedDelegate.Broadcast(newPhase);
}

void AMCGameState::ReportKillDoneMulticast_Implementation(APlayerState* damageMaker, APlayerState* killedActor)
{
	OnKillReportedEvent.Broadcast(killedActor, damageMaker);
}
