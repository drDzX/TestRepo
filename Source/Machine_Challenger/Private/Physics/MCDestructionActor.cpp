// Fill out your copyright notice in the Description page of Project Settings.

#include "Physics/MCDestructionActor.h"
#include "TimerManager.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

#include "Components/PrimitiveComponent.h"
#include "PhysicsProxy/FieldSystemProxyHelper.h"
#include "PhysicsProxy/GeometryCollectionPhysicsProxy.h"

AMCDestructionActor::AMCDestructionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create Geometry Collection Component
	GeometryCollection = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollection"));
	RootComponent = GeometryCollection;

	// Enable breaking notifications
	GeometryCollection->SetNotifyBreaks(true);
}

void AMCDestructionActor::BeginPlay()
{
	Super::BeginPlay();

	if (GeometryCollection)
	{
		// Bind to the OnChaosBreakEvent
		GeometryCollection->OnChaosBreakEvent.AddDynamic(this, &AMCDestructionActor::OnChaosBreakEvent);
	}
}

void AMCDestructionActor::OnChaosBreakEvent(const FChaosBreakEvent& BreakEvent)
{
	DestroyBrokenChunks(BreakEvent.Index);
}

void AMCDestructionActor::DestroyBrokenChunks(int32 index)
{
	if (Chaos::FPhysicsSolver* RBDSolver = GeometryCollection->GetPhysicsProxy()->GetSolver<Chaos::FPhysicsSolver>())
	{
		RBDSolver->EnqueueCommandImmediate([this, index, RBDSolver]()
		{
			TSet<Chaos::FPBDRigidClusteredParticleHandle*> Processed;

			if (!GeometryCollection)
			{
				return;
			}
			const auto& phyCollection = GeometryCollection->GetPhysicsProxy();
			if (!phyCollection)
			{
				return;
			}

			const auto& particle = phyCollection->GetParticleByIndex_Internal(index);

			Chaos::UpdateMaterialDisableThreshold(RBDSolver, particle, 5.f);
		});
	}
}
