// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MCDestructionActor.generated.h"

UCLASS()
class AMCDestructionActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMCDestructionActor();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components")
	class UGeometryCollectionComponent* GeometryCollection;

	UFUNCTION()
	void OnChaosBreakEvent(const FChaosBreakEvent& BreakEvent);

	FTimerHandle DestructionTimerHandle;

	void DestroyBrokenChunks(int32 index);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsDisabled = false;
};
