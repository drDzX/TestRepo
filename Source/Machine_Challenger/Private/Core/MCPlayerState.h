// Fill out your copyright notice in the Description page of Project Settings.
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "NativeGameplayTags.h"
#include "Abilities/MCAbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "SaveGame/MCSaveGame.h"
#include "MCPlayerState.generated.h"

class AMCVehiclePawnBase;
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Team_A);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Team_B);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Team_Neutral);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PlayerState_Alive);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_PlayerState_Dead)

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPlayerInfoChanged);

class AMCPlayerState;

USTRUCT()
struct FDamageRecord
{
	GENERATED_BODY()

	UPROPERTY()
	class AMCPlayerState* PlayerState;
	FTimerHandle TimerHandle;

	FDamageRecord() :
		PlayerState(nullptr) {}

	FDamageRecord(AMCPlayerState* InPlayerState) :
		PlayerState(InPlayerState) {}
};

/**
 *
 */
UCLASS()
class AMCPlayerState : public APlayerState, public IMCAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(Server, Reliable)
	void AddDamageScore(const float AddDamage);
	UFUNCTION(Server, Reliable)
	void ResetDamageScore();

	UFUNCTION(Server, Reliable)
	void AddKillScore(const int AddKill);
	UFUNCTION(Server, Reliable)
	void ResetKillScore();
	UFUNCTION(Server, Reliable)
	void AddDeathScore(const int Value);
	UFUNCTION(Server, Reliable)
	void ResetDeathScore();
	UFUNCTION(Server, Reliable)
	void AddAssistScore(const int Value);
	UFUNCTION(Server, Reliable)
	void ResetAssistScore();
	UFUNCTION(Server, Reliable)
	void ChangePlayerTeam(const FGameplayTag& newTeam);
	UFUNCTION(Server, Reliable)
	void ChangePlayerState(const FGameplayTag& newState);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void RenamePlayer(const FString& NewName);

	UFUNCTION()
	void RemoveAssistInput(AMCPlayerState* playerState);

	void AssistDamageDone(AMCPlayerState* DamageMaker);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//Report damage player has inflicted
	virtual void OnReportDamageDone_Implementation(float Damage, bool isCritical, FVector damageLocation, AActor* damageMaker, AActor* damageReciever) override;
	virtual void OnReportPlayerRespawned_Implementation() override;
	virtual void OnReportKillDone_Implementation(APlayerState* damageMaker, APlayerState* killedActor) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

private:
	UFUNCTION()
	virtual void OnRep_PlayerTeam(const FGameplayTag& oldValue);
	UFUNCTION()
	virtual void OnRep_PlayerState(const FGameplayTag& oldValue);

public:
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int KillCollected = 0;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int Deaths = 0;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int Assists = 0;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float DamageDone = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	float AssistTimer = 5.f;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerTeam, VisibleAnywhere, BlueprintReadOnly, Category = "Teams")
	FGameplayTag PlayerTeam = TAG_Team_A;
	UPROPERTY(ReplicatedUsing = OnRep_PlayerState, VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	FGameplayTag PlayerState = TAG_PlayerState_Alive;
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FPlayerInfoChanged OnPlayerInfoChanged;
	UPROPERTY()
	TArray<FDamageRecord> AssistActors;
};
