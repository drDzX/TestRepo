// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MCActorSpawner.generated.h"

USTRUCT(BlueprintType)
struct FSpawnActorSetup
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> SpawnClass;
	//Chance in percent to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
	int Chance = 100;
};

UCLASS()
class AActorSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AActorSpawner();

	UFUNCTION(Server, Reliable)
	void Spawn();

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ResetSpawner();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void ResetTimer();
	UFUNCTION(BlueprintPure)
	int GetNumSpawnedActors();

	UFUNCTION(BlueprintSetter)
	void SetActive(bool bIsActive);

	UFUNCTION(BlueprintImplementableEvent)
	void OnActivationChanged(bool& bIsActive);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void ClearSpawnedActorsArray();
	TSubclassOf<AActor> GetClassToSpawn() const;

	FHitResult GetSpawnHit() const;
	static FRotator GetRotationFromImpactNormal(const FHitResult& HitResult);

	UFUNCTION()
	void OnDestroyed_Actor(AActor* DestroyedActor);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter = SetActive, Category = "Spawner")
	bool bIsSpawnerActive = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	TArray<FSpawnActorSetup> ActorsToSpawn;
	//Spawn object by tracing the static mesh below
	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bIsSpawningAtFloor = true;
	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bIsOrientingToFloor = true;
	// How often the object is spawned
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	float SpawnRate = 5.f;
	//If set to false spawner will always spawn object until limit is reached, if some object get destroyed new one will re spawn. If  set to false, only set limit of objects will be spawned after which spawner is disabled.
	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bIsSpawningToLimitOnce = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	int SpawnLimit = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TArray<TObjectPtr<AActor>> SpawnedActors;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawner")
	bool bIsSpawningOnSpot = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner")
	class UBoxComponent* SpawnRegion;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Spawner")
	bool bResetTimerAfterPickup = false;

	int SpawnCount = 0;
	float ElapsedTime = 0.f;
};
