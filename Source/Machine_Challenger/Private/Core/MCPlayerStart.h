// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/PlayerStart.h"
#include "MCPlayerStart.generated.h"

/**
 *
 */
UCLASS()
class AMCPlayerStart : public APlayerStart, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AMCPlayerStart(const FObjectInitializer& ObjectInitializer);
	UFUNCTION(BlueprintCallable, Category = "GameplayTags")
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	UFUNCTION(Server, Reliable)
	void ChangeOccupiedActor(AController* newController);
	UFUNCTION(Server, Reliable)
	void CheckOccupancy();
	UFUNCTION(Server, Reliable)
	void SpawnReleased(AController* ReleasedController);

private:
	UFUNCTION()
	void EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	class UBoxComponent* SpawnZone;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer MCTags;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	AController* OccupiedActor;
};
