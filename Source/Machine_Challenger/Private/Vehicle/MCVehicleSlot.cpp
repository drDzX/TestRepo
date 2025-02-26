// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCVehicleSlot.h"

#include "AsyncTickFunctions.h"
#include "MCVehicleCombat.h"
#include "Animation/AnimPhysicsSolver.h"
#include "Inventory/InventorySubsystem.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Root, "Vehicle.Slot.Root");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Roof, "Vehicle.Slot.Roof");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Door, "Vehicle.Slot.Door");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_SideSkirt, "Vehicle.Slot.SideSkirt");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Exhaust, "Vehicle.Slot.Exhaust");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Mirror, "Vehicle.Slot.Mirror");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Fender, "Vehicle.Slot.Fender");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Bumper, "Vehicle.Slot.Bumper");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Wheel, "Vehicle.Slot.Wheel");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Diffuser, "Vehicle.Slot.Diffuser");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Hood, "Vehicle.Slot.Hood");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Trunk, "Vehicle.Slot.Trunk");
UE_DEFINE_GAMEPLAY_TAG(TAG_VehicleSlotId_Grill, "Vehicle.Slot.Grill");

UE_DEFINE_GAMEPLAY_TAG(TAG_SideTag_Right, "SideTag.Right");
UE_DEFINE_GAMEPLAY_TAG(TAG_SideTag_Left, "SideTag.Left");
UE_DEFINE_GAMEPLAY_TAG(TAG_SideTag_Front, "SideTag.Front");
UE_DEFINE_GAMEPLAY_TAG(TAG_SideTag_Rear, "SideTag.Rear");

// Sets default values
AMCVehicleSlot::AMCVehicleSlot()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootSceneComponent;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SKMComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("STMComponent"));
	SkeletalMeshComponent->SetupAttachment(RootComponent);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
	bReplicates = true;
	bNetLoadOnClient = true;
	CreateMeshes();
}

void AMCVehicleSlot::SetDamage_Implementation(float NewDamage)
{
	CurrentDamage = NewDamage;
	OnRep_CurrentDamage();
}

// Called when the game starts or when spawned
void AMCVehicleSlot::BeginPlay()
{
	Super::BeginPlay();
	CreateMeshes();
	OnRep_BaseMaterial();
	if (!GetOwner())
	{
		Destroy();
	}

	Attach();
}

void AMCVehicleSlot::Attach()
{
	if (!IsValid(GetOwner()) || !GetOwner()->Implements<UMCVehicleInterface>())
	{
		return;
	}

	const auto& mesh = IMCVehicleInterface::Execute_GetAttachmentMesh(GetOwner());
	if (!mesh)
	{
		return;
	}

	AttachToComponent(mesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SlotState.SocketName);
	SetActorRelativeScale3D(FVector(1.f, SlotState.bIsFlippedRightLeft ? -1.f : 1.f, 1.f));
}

void AMCVehicleSlot::CreateMeshes()
{
	if (!SkeletalMeshComponent || !StaticMeshComponent)
	{
		return;
	}

	if (!bIsForceStaticMesh && SkeletalMesh)
	{
		SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
		SkeletalMeshComponent->SetVisibility(true);
		StaticMeshComponent->SetVisibility(false);
		bIsUsingSkeletal = true;
		return;
	}

	if (StaticMesh)
	{
		bIsStaticMesh = true;
		SkeletalMeshComponent->SetVisibility(false);
		StaticMeshComponent->SetVisibility(true);
		StaticMeshComponent->SetStaticMesh(StaticMesh);
	}
}

void AMCVehicleSlot::UpdateDynamicMaterialParameter(FName Name, float Value)
{
	if (!DynamicCarMaterial)
	{
		return;
	}

	DynamicCarMaterial->SetScalarParameterValue(Name, Value);
}

void AMCVehicleSlot::SetBaseMaterial_Implementation(UMaterialInterface* NewMaterial)
{
	if (BaseMaterial == NewMaterial)
	{
		return;
	}

	BaseMaterial = NewMaterial;
	OnRep_BaseMaterial(); // Immediate update on the server
}

void AMCVehicleSlot::UpdateColor(UMaterialInterface* Material)
{
	if (HasAuthority())
	{
		const auto& inventory = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>();
		if (!inventory)
		{
			return;
		}

		if (Material)
		{
			SetBaseMaterial(Material);
			return;
		}

		if (OwningCarCombat)
		{
			SetBaseMaterial(OwningCarCombat->CarColor);
		}
	}
}

void AMCVehicleSlot::Explode_Implementation(FVector CenterOfExplosion, float Force)
{
	if (bIsStaticMesh)
	{
		StaticMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
		StaticMeshComponent->SetSimulatePhysics(true);

		const FVector ForceDirection = (StaticMeshComponent->GetCenterOfMass() - CenterOfExplosion).GetSafeNormal();
		StaticMeshComponent->AddForce(ForceDirection * Force);
		return;
	}

	SkeletalMeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	SkeletalMeshComponent->SetSimulatePhysics(true);
	const FVector ForceDirection = (SkeletalMeshComponent->GetCenterOfMass() - CenterOfExplosion).GetSafeNormal();
	SkeletalMeshComponent->AddForce(ForceDirection * Force);
	SetLifeSpan(10.f);
}

// Called every frame
void AMCVehicleSlot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMCVehicleSlot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCVehicleSlot, OwningCarCombat);
	DOREPLIFETIME(AMCVehicleSlot, BaseMaterial);
	DOREPLIFETIME(AMCVehicleSlot, CurrentDamage);
	DOREPLIFETIME(AMCVehicleSlot, SlotState);
}

void AMCVehicleSlot::OnRep_BaseMaterial_Implementation()
{
	if (!StaticMeshComponent || !SkeletalMeshComponent || CarMaterialIndex < 0)
	{
		return;
	}

	if (!bIsUsingSkeletal && StaticMeshComponent && BaseMaterial && bIsStaticMesh)
	{
		DynamicCarMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		StaticMeshComponent->SetMaterial(CarMaterialIndex, DynamicCarMaterial);
		return;
	}

	if (bIsUsingSkeletal && SkeletalMeshComponent && BaseMaterial)
	{
		DynamicCarMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		SkeletalMeshComponent->SetMaterial(CarMaterialIndex, DynamicCarMaterial);
	}
}

void AMCVehicleSlot::OnRep_OwningCarCombat_Implementation()
{ }

void AMCVehicleSlot::OnRep_SlotState()
{
	Attach();
}

void AMCVehicleSlot::OnRep_CurrentDamage()
{
	const auto& damageDelta = CurrentDamage / MaxDamage;
	if (!bIsIgnoringDamageVFX)
	{
		UpdateDynamicMaterialParameter("Damage", damageDelta);
	}

	if (damageDelta >= 1.f)
	{
		UpdateDynamicMaterialParameter("Burn", 1.f);
	}
}

void AMCVehicleSlot::PostNetReceive()
{
	Super::PostNetReceive();
	Attach();
}
#if WITH_EDITOR
void AMCVehicleSlot::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AMCVehicleSlot, bIsForceStaticMesh) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMCVehicleSlot, StaticMesh) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(AMCVehicleSlot, SkeletalMesh))
	{
		CreateMeshes();
	}
}

#endif
