// Fill out your copyright notice in the Description page of Project Settings.

#include "Utils/MCActorSpawner.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Utils/MCBlueprintLib.h"

// Sets default values
AActorSpawner::AActorSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	if (!bIsSpawningOnSpot)
	{
		SpawnRegion = CreateDefaultSubobject<UBoxComponent>(TEXT("Spawn Area"));
		SpawnRegion->SetupAttachment(RootComponent);
		SpawnRegion->SetGenerateOverlapEvents(false);
		SpawnRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SpawnRegion->SetBoxExtent(FVector(2000.f, 2000.f, 500.f));
	}
}

void AActorSpawner::Spawn_Implementation()
{
	if (!bIsSpawnerActive)
	{
		return;
	}

	ClearSpawnedActorsArray();
	if (SpawnLimit > 0 && SpawnCount >= SpawnLimit)
	{
		return;
	}

	const auto& ClassToSpawn = GetClassToSpawn();
	if (!ClassToSpawn)
	{
		return;
	}
	FActorSpawnParameters spawnParams;
	spawnParams.Owner = GetOwner();
	const auto& spawnHitResult = GetSpawnHit();
	//Skip spawn if no floor and it is required.
	if (bIsSpawningAtFloor && !spawnHitResult.GetActor())
	{
		return;
	}
	const auto& spawnLoc = spawnHitResult.Location;
	const auto& spawnRot = bIsOrientingToFloor ? GetRotationFromImpactNormal(spawnHitResult) : GetActorRotation();
	const auto& spawnedActor = GetWorld()->SpawnActor<AActor>(ClassToSpawn, spawnLoc, spawnRot, spawnParams);

	if (!spawnedActor)
	{
		return;
	}

	SpawnedActors.Add(spawnedActor);
	if (bResetTimerAfterPickup)
	{
		spawnedActor->OnDestroyed.AddDynamic(this, &AActorSpawner::OnDestroyed_Actor);
	}

	SpawnCount++;
	if (bIsSpawningToLimitOnce && SpawnCount >= SpawnLimit)
	{
		SetActive(false);
	}
}

void AActorSpawner::ResetSpawner_Implementation()
{
	for (const auto& actor : SpawnedActors)
	{
		actor->Destroy();
	}

	SpawnedActors.Empty();
	SpawnCount = 0;
	ElapsedTime = 0.f;
}

void AActorSpawner::ResetTimer_Implementation()
{
	ElapsedTime = SpawnRate;
}

int AActorSpawner::GetNumSpawnedActors()
{
	ClearSpawnedActorsArray();
	return SpawnedActors.Num();
}

void AActorSpawner::SetActive(bool bIsActive)
{
	if (bIsSpawnerActive == bIsActive)
	{
		return;
	}

	bIsSpawnerActive = bIsActive;
	OnActivationChanged(bIsActive);
}

TSubclassOf<AActor> AActorSpawner::GetClassToSpawn() const
{
	if (ActorsToSpawn.IsEmpty())
	{
		return nullptr;
	}

	const FRandomStream RandomStream(FDateTime::Now().GetTicks()); // Seed the random stream with the current time

	float TotalChance = 0.0f;
	for (const auto& [Class, Chance] : ActorsToSpawn)
	{
		TotalChance += Chance;
	}

	const auto& RandomValue = RandomStream.GetFraction() * TotalChance;

	// Find the object corresponding to the random value
	float CurrentChanceSum = 0.0f;
	for (int32 Index = 0; Index < ActorsToSpawn.Num(); ++Index)
	{
		CurrentChanceSum += ActorsToSpawn[Index].Chance;
		if (RandomValue <= CurrentChanceSum)
		{
			return ActorsToSpawn[Index].SpawnClass;
		}
	}

	// Return nullptr if no valid object is found (this should not happen if chances are properly defined)
	return nullptr;
}

FHitResult AActorSpawner::GetSpawnHit() const
{
	FHitResult hitResult;
	FVector spawnLocation;
	if (!bIsSpawningOnSpot)
	{
		spawnLocation = UKismetMathLibrary::RandomPointInBoundingBox(SpawnRegion->GetRelativeTransform().GetLocation(), SpawnRegion->GetScaledBoxExtent());
	}
	else
	{
		spawnLocation = GetActorLocation();
	}

	if (!bIsSpawningAtFloor)
	{
		hitResult.Location = spawnLocation;
		hitResult.ImpactNormal = GetActorUpVector();
		return hitResult;
	}
	FVector TraceStart;
	FVector TraceEnd;
	if (!bIsSpawningOnSpot)
	{
		const FVector& StartLocation = SpawnRegion->GetComponentLocation();
		const auto& BoxExtents = SpawnRegion->GetScaledBoxExtent();
		const auto& ActorRot = GetActorRotation();
		TraceStart = ActorRot.RotateVector(FVector(spawnLocation.X, spawnLocation.Y, StartLocation.Z + BoxExtents.Z) - GetActorLocation()) + GetActorLocation();
		TraceEnd = ActorRot.RotateVector(FVector(spawnLocation.X, spawnLocation.Y, StartLocation.Z - BoxExtents.Z) - GetActorLocation()) + GetActorLocation();
	}
	else
	{
		const auto& ActorRot = GetActorRotation();
		TraceStart = ActorRot.RotateVector(FVector(spawnLocation.X, spawnLocation.Y, spawnLocation.Z + 100.f) - GetActorLocation()) + GetActorLocation();
		TraceEnd = ActorRot.RotateVector(FVector(spawnLocation.X, spawnLocation.Y, spawnLocation.Z - 100000.f) - GetActorLocation()) + GetActorLocation();
	}

	if (UMCBlueprintLib::Trace(GetWorld(), this, TraceStart, TraceEnd, hitResult))
	{
		return hitResult;
	}

	hitResult.Location = TraceEnd;
	hitResult.ImpactNormal = FVector_NetQuantize::UpVector;
	return hitResult;
}

FRotator AActorSpawner::GetRotationFromImpactNormal(const FHitResult& HitResult)
{
	const auto& ImpactNormal = HitResult.ImpactNormal;

	// Ensure the impact normal is valid
	if (ImpactNormal.IsNearlyZero())
	{
		// Handle the case where the impact normal is zero or nearly zero
		return FRotator::ZeroRotator;
	}

	// Create a rotation matrix from the impact normal
	const auto& RotationMatrix = FRotationMatrix::MakeFromZ(ImpactNormal);

	// Extract the rotation from the matrix
	const auto& ImpactRotation = RotationMatrix.Rotator();

	return ImpactRotation;
}

void AActorSpawner::OnDestroyed_Actor(AActor* DestroyedActor)
{
	ResetTimer();
	SpawnedActors.Remove(DestroyedActor);
}

// Called when the game starts or when spawned
void AActorSpawner::BeginPlay()
{
	Super::BeginPlay();
	ClearSpawnedActorsArray();
}

// Called every frame
void AActorSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority())
	{
		return;
	}

	if (!bIsSpawnerActive)
	{
		if (ElapsedTime != SpawnRate)
		{
			ElapsedTime = SpawnRate;
		}

		return;
	}

	ElapsedTime -= DeltaTime;

	if (ElapsedTime <= 0)
	{
		ElapsedTime = SpawnRate;
		Spawn();
	}
}

void AActorSpawner::ClearSpawnedActorsArray()
{
	TArray<AActor*> NewArray;
	for (const auto& actor : SpawnedActors)
	{
		if (actor && !actor->IsActorBeingDestroyed())
		{
			NewArray.Add(actor);
		}
	}

	SpawnedActors = NewArray;
	if (!bIsSpawningToLimitOnce)
	{
		SpawnCount = NewArray.Num();
	}
}
