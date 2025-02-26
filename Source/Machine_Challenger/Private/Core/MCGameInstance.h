// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MCServerInterface.h"
#include "Engine/GameInstance.h"
#include "MCGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FResetGame);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRespawnCar);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCarIdChanged, FName, CarId);

/**
 *
 */
UCLASS()
class UMCGameInstance : public UGameInstance, public IMCServerInterface
{
	GENERATED_BODY()
	UMCGameInstance();

public:
	UFUNCTION(BlueprintCallable)
	void LoadingScreenShow();
	UFUNCTION(BlueprintCallable)
	void LoadingScreenHide();

	UFUNCTION(Exec)
	void ResetGame();

	UFUNCTION(Exec, BlueprintCallable)
	void ChangeCar(const FName& CarId);

protected:
	virtual void OnStart() override;

public:
	UPROPERTY(EditDefaultsOnly)
	FString StartingMapURL;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> LoadingScreenClass;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	TObjectPtr<UUserWidget> LoadingScreenWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	class UDataTable* CarInventory;

	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	class UDataTable* SlotInventory;

	UPROPERTY(EditDefaultsOnly, Category = "Color")
	class UDataTable* ColorMaterials;

	UPROPERTY(BlueprintAssignable)
	FResetGame ResetGameEvent;
	UPROPERTY(BlueprintAssignable)
	FRespawnCar RespawnCarEvent;
	UPROPERTY(BlueprintAssignable)
	FCarIdChanged OnCarIdChanged;;
};
