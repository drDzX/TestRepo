// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "MCGameInterface.h"
#include "GameFramework/PlayerController.h"
#include "MCPlayerController.generated.h"

class UMCAttributeSet;

/**
 *
 */
UCLASS()
class AMCPlayerController : public APlayerController, public IGameplayTagAssetInterface, public IMCGameInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(Client, Reliable)
	void SetupClient();
	UFUNCTION(Server, Reliable)
	void SetMainCar(AMCVehiclePawnBase* newCar);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Respawn(FGameplayTagContainer transferAttributes = FGameplayTagContainer(), TSubclassOf<AMCVehiclePawnBase> respawnClass = nullptr);
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void MoveToStart();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ChangeToSpectator();
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void TransferAttributes(UMCAttributeSet* oldAttributes, UMCAttributeSet* newAttributes, FGameplayTagContainer transferAttributes);
	UFUNCTION(BlueprintCallable, Category = "Team")
	virtual FGameplayTag GetPlayerTeam_Implementation() override;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerChangeTeam(int TeamIndex);
	UFUNCTION(Server, Reliable)
	void ChangeRespawnCarClass(TSubclassOf<AMCVehiclePawnBase> CarClass);

	UFUNCTION(Exec)
	void ChangeTeam(int TeamIndex);

protected:
	virtual void BeginPlay() override;
	virtual void BeginPlayingState() override;
	virtual void OnUnPossess() override;

private:
	UFUNCTION()
	virtual void OnRep_MainCar(AMCVehiclePawnBase* oldValue);
	UFUNCTION(Client, Reliable)
	void OnCarDataChanged(FCarDataInventory NewCarData);
	UFUNCTION(Server, Reliable)
	void OnRespawnCarEvent();

public:
	virtual void OnRep_PlayerState() override;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer MCTags;
	UPROPERTY(ReplicatedUsing = OnRep_MainCar, BlueprintReadOnly)
	AMCVehiclePawnBase* MainCar;
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite, Category = "Team")
	TSubclassOf<AMCVehiclePawnBase> RespawnCarClass;
	bool bIsMarkedForRespawn = false;
};
