// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "Abilities/MCAbilitySystemInterface.h"
#include "GameFramework/GameState.h"
#include "MCGameState.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GamePhase_Neutral);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GamePhase_Freeze);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GamePhase_Round);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GamePhase_Finish);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GamePhase_RestartGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FKillReported, APlayerState*, KilledActor, APlayerState*, DamageMaker);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameStateChangedDelegate, FGameplayTag, newPhase);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameInfoChanged);

/**
 *
 */
UCLASS()
class AMCGameState : public AGameStateBase, public IMCAbilitySystemInterface
{
	GENERATED_BODY()

public:
	virtual void OnReportKillDone_Implementation(APlayerState* damageMaker, APlayerState* killedActor) override;

	UFUNCTION(NetMulticast, Reliable)
	void ReportKillDoneMulticast(APlayerState* damageMaker, APlayerState* killedActor);

	UFUNCTION(Exec)
	void RestartGame();
	UFUNCTION(Server, Reliable)
	void ChangeGamePhase(FGameplayTag newState);

	UFUNCTION(NetMulticast, Reliable)
	void BroadcastInfoChanged();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(NetMulticast, Reliable)
	void BroadcastGameChanged(FGameplayTag newPhase);

	virtual FGameplayTagContainer GetDeadTeams();

public:
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	UPROPERTY(BlueprintAssignable)
	FKillReported OnKillReportedEvent;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Game phases")
	FGameplayTag GamePhase = TAG_GamePhase_Neutral;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGameStateChangedDelegate OnStateChangedDelegate;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FGameInfoChanged OnGameInfoChanged;
};
