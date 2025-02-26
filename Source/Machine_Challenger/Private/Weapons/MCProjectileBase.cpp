// Fill out your copyright notice in the Description page of Project Settings.

#include "Weapons/MCProjectileBase.h"

#include "Utils/MCBlueprintLib.h"

// Sets default values
AMCProjectileBase::AMCProjectileBase()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMCProjectileBase::BeginPlay()
{
	Super::BeginPlay();
	SpawnLocation = GetActorLocation();
}

// Called every frame
void AMCProjectileBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (MaxDistance > 0.f && FVector::Distance(GetActorLocation(), SpawnLocation) >= MaxDistance)
	{
		Destroy();
	}
}

float AMCProjectileBase::GetTravelDistanceDamageMultiplier()
{
	const auto& curve = DamageToDistanceCurve.GetRichCurveConst();
	if (curve->IsEmpty())
	{
		return 1.f;
	}

	return curve->Eval(FVector::Distance(GetActorLocation(), SpawnLocation));
}

float AMCProjectileBase::GetDistanceDamageMultiplier(float ForDistance)
{
	const auto& curve = DamageToDistanceCurve.GetRichCurveConst();
	if (curve->IsEmpty())
	{
		return 1.f;
	}

	return curve->Eval(ForDistance);
}

TArray<FTraceActorResult> AMCProjectileBase::TraceActorsAround(const TArray<AActor*>& ActorsToCheck, AActor* ignoreActor, FVector offset, bool SkipDownOffset)
{
	const auto& world = GetWorld();
	if (ActorsToCheck.IsEmpty() || !world)
	{
		return TArray<FTraceActorResult>();
	}

	TArray<FTraceActorResult> outResult;
	for (const auto& actor : ActorsToCheck)
	{
		if (actor == ignoreActor)
		{
			continue;
		}

		const auto& ActorLoc = actor->GetComponentsBoundingBox().GetCenter();
		TArray<FVector> Vertices;
		if (offset != FVector::ZeroVector)
		{
			Vertices.Add(actor->GetActorForwardVector() * offset.X + ActorLoc);      // Push front
			Vertices.Add(actor->GetActorForwardVector() * offset.X * -1 + ActorLoc); // Push back
			Vertices.Add(actor->GetActorRightVector() * offset.Y + ActorLoc);        // Push right
			Vertices.Add(actor->GetActorRightVector() * offset.Y * -1 + ActorLoc);   // Push left
			Vertices.Add(actor->GetActorUpVector() * offset.Z + ActorLoc);           // Push up
			if (!SkipDownOffset)
			{
				Vertices.Add(actor->GetActorUpVector() * offset.Z * -1 + ActorLoc); // Push down
			}
		}
		else
		{
			Vertices.Add(ActorLoc);
		}

		FTraceActorResult result;
		result.Distance = FVector::Distance(ActorLoc, GetActorLocation());
		for (const auto& vertex : Vertices)
		{
			FHitResult hit;
			UMCBlueprintLib::Trace(world, this, GetActorLocation(), vertex, hit, ECollisionChannel::ECC_Vehicle);
			if (hit.GetActor() && hit.GetActor() != actor)
			{
				continue;
			}

			const auto& vertexDistance = FVector::Distance(vertex, GetActorLocation());
			if (result.Distance > vertexDistance)
			{
				result.Actor = actor;
				result.Distance = vertexDistance;
				result.Location = vertex;
			}
		}

		if (!result.Actor)
		{
			continue;
		}

		outResult.Add(result);
	}

	return outResult;
}
