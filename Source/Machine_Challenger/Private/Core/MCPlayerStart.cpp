// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCPlayerStart.h"

#include "MCGameModeBase.h"
#include "MCPlayerController.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Vehicle/MCVehiclePawnBase.h"

AMCPlayerStart::AMCPlayerStart(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	SpawnZone = CreateDefaultSubobject<UBoxComponent>(TEXT("Area of effect"));
	SpawnZone->SetupAttachment(RootComponent);
	SpawnZone->SetBoxExtent(FVector(500.f, 500.f, 500.f));
	SpawnZone->SetGenerateOverlapEvents(true);
	SpawnZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SpawnZone->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SpawnZone->SetCollisionResponseToChannel(ECC_Vehicle, ECollisionResponse::ECR_Overlap);
	SpawnZone->OnComponentEndOverlap.AddDynamic(this, &AMCPlayerStart::EndOverlap);
}

void AMCPlayerStart::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer = MCTags;
}

void AMCPlayerStart::ChangeOccupiedActor_Implementation(AController* newController)
{
	OccupiedActor = newController;
	if (!newController)
	{
		ForceNetUpdate();
	}
}

void AMCPlayerStart::SpawnReleased_Implementation(AController* ReleasedController)
{
	if (OccupiedActor && ReleasedController == OccupiedActor)
	{
		ChangeOccupiedActor(nullptr);
	}
}

void AMCPlayerStart::EndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OccupiedActor || !HasAuthority())
	{
		return;
	}

	CheckOccupancy();
}

void AMCPlayerStart::CheckOccupancy_Implementation()
{
	TArray<AActor*> FoundActors;
	SpawnZone->GetOverlappingActors(FoundActors, AMCVehiclePawnBase::StaticClass());
	bool containsOccupiedCar = false;
	for (const auto& actor : FoundActors)
	{
		const auto& MCVehicle = Cast<AMCVehiclePawnBase>(actor);
		if (!MCVehicle)
		{
			continue;
		}

		if (MCVehicle->owningController && MCVehicle->owningController == OccupiedActor)
		{
			containsOccupiedCar = true;
			break;
		}
	}

	if (!containsOccupiedCar)
	{
		ChangeOccupiedActor(nullptr);
	}
}

void AMCPlayerStart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCPlayerStart, OccupiedActor);
}

void AMCPlayerStart::BeginPlay()
{
	Super::BeginPlay();
	const auto& World = GetWorld();
	if (!World)
	{
		return;
	}

	if (const auto& gameMode = Cast<AMCGameModeBase>(World->GetAuthGameMode()))
	{
		gameMode->OnSpawnReleasedEvent.AddDynamic(this, &AMCPlayerStart::SpawnReleased);
	}
}
