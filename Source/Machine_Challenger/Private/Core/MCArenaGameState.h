// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "MCGameInterface.h"
#include "NativeGameplayTags.h"
#include "Core/MCGameState.h"
#include "MCArenaGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDangerZoneActivationChange, bool, IsActive);

USTRUCT(BlueprintType)
struct FTimerParameters
{
	GENERATED_BODY()

	//Delay before effect take place
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float Delay;
	//Duration of main effect (round, effect etc.)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float Duration;
	//Timer handle for delay
	UPROPERTY()
	FTimerHandle DelayTimer;
	//Timer handle for duration
	UPROPERTY()
	FTimerHandle DurationTimer;

	//Constructor
	FTimerParameters()
	{
		Duration = 0.f;
		Delay = 0.f;
	}
};

/**
 *
 */
UCLASS()
class AMCArenaGameState : public AMCGameState, public IGameplayTagAssetInterface, public IMCGameInterface
{
	GENERATED_BODY()

public:
	AMCArenaGameState();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void StartRound();
	UFUNCTION(Server, Reliable)
	void FinishRound();
	UFUNCTION(Server, Reliable)
	void StartFreezeTime();
	UFUNCTION(Server, Reliable)
	void StartNeutralPhase();
	UFUNCTION(Server, Reliable)
	void StartArenaPhase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(Server, Reliable)
	void WinRound();
	UFUNCTION(Server, Reliable)
	void WinRoundByTeam(FGameplayTag winningTeam);

	UFUNCTION(Exec)
	void EndRound();
	UFUNCTION(Exec)
	void BeginRound();
	UFUNCTION(Exec)
	void RestartArenaGame();
	UFUNCTION(Exec)
	void StartNeutralGameMode();
	UFUNCTION(Exec)
	void SetRoundTime(float Time);
	UFUNCTION(Exec)
	void BeginFreezeTime();
	UFUNCTION(Exec)
	void ChangeEnergy(int NewEnergy);
	UFUNCTION(NetMulticast, Reliable)
	void DangerZoneActivationChange(bool IsActivated);

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual void Reset() override;
	virtual void OnReportKillDone_Implementation(APlayerState* damageMaker, APlayerState* killedActor) override;

private:
	void ClearTimers();
	UFUNCTION(Server, Reliable)
	void UpdateRemainingTimeVar();
	UFUNCTION(Server, Reliable)
	void FinishRoundExecute();

	UFUNCTION(Server, Reliable)
	void CheckFinishRoundCondition();

	virtual bool IsMovementAllowed_Implementation() override;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	FTimerParameters RoundTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	FTimerParameters EndRoundTimer;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Timer")
	float RemainingTime;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Teams")
	int ScoreTeamA = 0;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Teams")
	int ScoreTeamB = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Phase")
	FGameplayTagContainer MovementAllowedPhase;

	UPROPERTY(BlueprintAssignable)
	FDangerZoneActivationChange DangerZoneChangeEvent;
};
