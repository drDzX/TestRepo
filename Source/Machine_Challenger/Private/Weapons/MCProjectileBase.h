// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCProjectileBase.generated.h"

USTRUCT(BlueprintType)
struct FTraceActorResult
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	AActor* Actor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Distance = 0.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;
};

UCLASS()
class AMCProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMCProjectileBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintPure)
	float GetTravelDistanceDamageMultiplier();
	UFUNCTION(BlueprintPure)
	float GetDistanceDamageMultiplier(float ForDistance);

	UFUNCTION(BlueprintPure)
	TArray<FTraceActorResult> TraceActorsAround(const TArray<AActor*>& ActorsToCheck, AActor* ignoreActor, FVector offset = FVector::ZeroVector, bool SkipDownOffset = true);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	FRuntimeFloatCurve DamageToDistanceCurve;

private:
	FVector SpawnLocation;
};
