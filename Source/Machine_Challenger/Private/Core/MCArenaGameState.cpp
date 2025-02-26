// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCArenaGameState.h"

#include "MCPlayerState.h"
#include "Abilities/MCAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "Vehicle/MCVehiclePawnBase.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_GamePhase_Neutral, "GamePhase.Neutral");         // "Testing phase"
UE_DEFINE_GAMEPLAY_TAG(TAG_GamePhase_Freeze, "GamePhase.Freeze");           // "Pre round"
UE_DEFINE_GAMEPLAY_TAG(TAG_GamePhase_Round, "GamePhase.Round");             // "Gameplay"
UE_DEFINE_GAMEPLAY_TAG(TAG_GamePhase_Finish, "GamePhase.Finish");           // "After round"
UE_DEFINE_GAMEPLAY_TAG(TAG_GamePhase_RestartGame, "GamePhase.RestartGame"); // "Restart queued"

void AMCArenaGameState::StartRound_Implementation()
{
	ClearTimers();
	GetWorldTimerManager().SetTimer(RoundTimer.DurationTimer, this, &AMCArenaGameState::FinishRound, RoundTimer.Duration, false);
	if (RoundTimer.Duration > 0.f)
	{
		ChangeGamePhase(TAG_GamePhase_Round);
	}
}

void AMCArenaGameState::FinishRound_Implementation()
{
	if (GamePhase != TAG_GamePhase_Round)
	{
		return;
	}

	ChangeGamePhase(TAG_GamePhase_Finish);
	WinRound();

	if (EndRoundTimer.Delay > 0.f)
	{
		ClearTimers();
		const auto& elapsedTime = GetWorldTimerManager().GetTimerElapsed(EndRoundTimer.DelayTimer);
		if (elapsedTime <= 0.f)
		{
			GetWorldTimerManager().SetTimer(EndRoundTimer.DelayTimer, this, &AMCArenaGameState::FinishRoundExecute, EndRoundTimer.Delay, false);
		}
		return;
	}

	FinishRoundExecute();
}

void AMCArenaGameState::StartFreezeTime_Implementation()
{
	if (RoundTimer.Delay <= 0.f)
	{
		StartRound();
		return;
	}

	ChangeGamePhase(TAG_GamePhase_Freeze);
	ClearTimers();
	GetWorldTimerManager().SetTimer(RoundTimer.DelayTimer, this, &AMCArenaGameState::StartRound, RoundTimer.Delay, false);
}

void AMCArenaGameState::StartNeutralPhase_Implementation()
{
	Reset();
	ChangeGamePhase(TAG_GamePhase_Neutral);
	if (!GetInstigatorController() || !GetInstigatorController()->IsLocalController())
	{
		return;
	}

	UpdateRemainingTimeVar();
}

void AMCArenaGameState::StartArenaPhase_Implementation()
{
	ChangeGamePhase(TAG_GamePhase_RestartGame);
	Reset();
	if (!GetInstigatorController() || !GetInstigatorController()->IsLocalController())
	{
		return;
	}
	UpdateRemainingTimeVar();
	StartFreezeTime();
}

AMCArenaGameState::AMCArenaGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMCArenaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCArenaGameState, RemainingTime);
	DOREPLIFETIME(AMCArenaGameState, ScoreTeamA);
	DOREPLIFETIME(AMCArenaGameState, ScoreTeamB);
}

void AMCArenaGameState::WinRound_Implementation()
{
	const auto& deadTeams = GetDeadTeams();

	if (deadTeams.HasTag(TAG_Team_B) && !deadTeams.HasTag(TAG_Team_A))
	{
		ScoreTeamA++;
		return;
	}

	if (!deadTeams.HasTag(TAG_Team_B) && deadTeams.HasTag(TAG_Team_A))
	{
		ScoreTeamB++;
		return;
	}

	ScoreTeamA++;
}

void AMCArenaGameState::WinRoundByTeam_Implementation(FGameplayTag winningTeam)
{
	if (!winningTeam.IsValid() || winningTeam == TAG_Team_Neutral)
	{
		return;
	}

	ChangeGamePhase(TAG_GamePhase_Finish);

	if (winningTeam == TAG_Team_A)
	{
		ScoreTeamA++;
	}

	if (winningTeam == TAG_Team_B)
	{
		ScoreTeamB++;
	}

	if (EndRoundTimer.Delay > 0.f)
	{
		const auto& elapsedTime = GetWorldTimerManager().GetTimerElapsed(EndRoundTimer.DelayTimer);
		if (elapsedTime <= 0.f)
		{
			GetWorldTimerManager().SetTimer(EndRoundTimer.DelayTimer, this, &AMCArenaGameState::FinishRoundExecute, EndRoundTimer.Delay, false);
		}
		return;
	}

	FinishRoundExecute();
}

void AMCArenaGameState::EndRound()
{
	FinishRound();
}

void AMCArenaGameState::BeginRound()
{
	StartRound();
}

void AMCArenaGameState::RestartArenaGame()
{
	StartArenaPhase();
}

void AMCArenaGameState::StartNeutralGameMode()
{
	StartNeutralPhase();
}

void AMCArenaGameState::SetRoundTime(float Time)
{
	RoundTimer.Duration = Time;
}

void AMCArenaGameState::BeginFreezeTime()
{
	StartFreezeTime();
}

void AMCArenaGameState::ChangeEnergy(int NewEnergy)
{
	for (const auto& player : PlayerArray)
	{
		const auto& mcCar = Cast<AMCVehiclePawnBase>(player->GetPawn());
		if (!mcCar)
		{
			continue;
		}

		mcCar->Attributes.Get()->SetPickupEnergy1(NewEnergy);
		mcCar->Attributes.Get()->SetPickupEnergy2(NewEnergy);
		mcCar->Attributes.Get()->SetPickupEnergy3(NewEnergy);
	}
}

void AMCArenaGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!GetInstigatorController() || !GetInstigatorController()->IsLocalController())
	{
		return;
	}

	UpdateRemainingTimeVar();
}

void AMCArenaGameState::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AddTag(GamePhase);
}

void AMCArenaGameState::Reset()
{
	Super::Reset();
	ScoreTeamA = 0;
	ScoreTeamB = 0;
	ClearTimers();
}

void AMCArenaGameState::OnReportKillDone_Implementation(APlayerState* damageMaker, APlayerState* killedActor)
{
	Super::OnReportKillDone_Implementation(damageMaker, killedActor);
	CheckFinishRoundCondition();
}

void AMCArenaGameState::ClearTimers()
{
	GetWorldTimerManager().ClearTimer(EndRoundTimer.DelayTimer);
	GetWorldTimerManager().ClearTimer(EndRoundTimer.DurationTimer);
	GetWorldTimerManager().ClearTimer(RoundTimer.DelayTimer);
	GetWorldTimerManager().ClearTimer(RoundTimer.DurationTimer);
}

void AMCArenaGameState::CheckFinishRoundCondition_Implementation()
{
	if (GamePhase != TAG_GamePhase_Round)
	{
		return;
	}
	const auto& deadTeams = GetDeadTeams();
	if (deadTeams.IsEmpty())
	{
		return;
	}

	FinishRound();
}

void AMCArenaGameState::DangerZoneActivationChange_Implementation(bool IsActivated)
{
	DangerZoneChangeEvent.Broadcast(IsActivated);
}

bool AMCArenaGameState::IsMovementAllowed_Implementation()
{
	return MovementAllowedPhase.HasTag(GamePhase);
}

void AMCArenaGameState::FinishRoundExecute_Implementation()
{
	ClearTimers();
	StartFreezeTime();
}

void AMCArenaGameState::UpdateRemainingTimeVar_Implementation()
{
	if (RoundTimer.DurationTimer.IsValid())
	{
		RemainingTime = GetWorldTimerManager().GetTimerRemaining(RoundTimer.DurationTimer);
		return;
	}

	if (RoundTimer.DelayTimer.IsValid())
	{
		RemainingTime = GetWorldTimerManager().GetTimerRemaining(RoundTimer.DelayTimer);
		return;
	}

	if (EndRoundTimer.DelayTimer.IsValid())
	{
		RemainingTime = GetWorldTimerManager().GetTimerRemaining(EndRoundTimer.DelayTimer);
		return;
	}

	if (RemainingTime <= 0.f)
	{
		return;
	}

	RemainingTime = 0.f;
}
