// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCVehicleAssemblyComponent.h"

#include "MCSuspensionComponent.h"
#include "MCVehicleCombat.h"
#include "MCVehiclePawnBase.h"
#include "Core/SaveGame/MCSaveGameSubsystem.h"
#include "gtest/gtest-matchers.h"
#include "Vehicle/VehicleMainMenu.h"
#include "Inventory/InventorySubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Utils/MCBlueprintLib.h"

// Sets default values for this component's properties
UMCVehicleAssemblyComponent::UMCVehicleAssemblyComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	OwningCar = Cast<AMCVehiclePawnBase>(GetOwner());
	if (OwningCar)
	{
		//SkeletalMeshComponent = OwningCar->GetMesh();
	}
	else
	{
		MainMenuCar = Cast<AVehicleMainMenu>(GetOwner());
		if (MainMenuCar && !AttachmentSkeleton)
		{
			MeshComponent = MainMenuCar->GetMesh();
		}
	}
	// ...
}
void UMCVehicleAssemblyComponent::OnRep_CarData()
{
	if (MainMenuCar && CarData.CarBody)
	{
		if (const auto& staticMesh = Cast<UStaticMeshComponent>(MainMenuCar->GetMesh()))
		{
			staticMesh->SetStaticMesh(CarData.CarBody.Get());
		}
	}

	if (OwningCar && CarData.CarBody)
	{
		if (const auto& staticMesh = Cast<UStaticMeshComponent>(OwningCar->GetMesh()))
		{
			staticMesh->SetStaticMesh(CarData.CarBody.Get());
		}
	}
}

void UMCVehicleAssemblyComponent::AssembleCar_Server_Implementation(FCarDataInventory CarDataValue)
{
	if (!MeshComponent)
	{
		return;
	}
	DestroyComponents();

	AdjustAttachment(CarDataValue);

	if (MainMenuCar && CarDataValue.CarBody)
	{
		if (const auto& staticMesh = Cast<UStaticMeshComponent>(MainMenuCar->GetMesh()))
		{
			staticMesh->SetStaticMesh(CarDataValue.CarBody.Get());
		}
	}

	if (OwningCar && CarDataValue.CarBody)
	{
		if (const auto& staticMesh = Cast<UStaticMeshComponent>(OwningCar->GetMesh()))
		{
			staticMesh->SetStaticMesh(CarDataValue.CarBody.Get());
		}
	}


	CarData = CarDataValue;

	UE_LOG(LogTemp, Log, TEXT("[Server] Received CarData: %s"), *CarDataValue.CodeName.ToString());

	TArray<AMCVehicleSlot*> AssembleComponentsLocal;
	for (const auto& item : CarDataValue.SlotItems)
	{
		if (item.bIsGarageViewOnly && !MainMenuCar)
		{
			continue;
		}

		if (item.SlotId == TAG_VehicleSlotId_Wheel)
		{
			continue;
		}

		if (!item.Slot)
		{
			continue;
		}

		FActorSpawnParameters spawnParams;
		spawnParams.Owner = GetOwner();
		const auto& slotItem = GetWorld()->SpawnActor<AMCVehicleSlot>(item.Slot, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
		slotItem->SetReplicateMovement(true);
		auto SlotData = slotItem->SlotData;

		slotItem->OwningCarCombat = Cast<AMCVehicleCombat>(OwningCar);

		if (item.IsFlippedLeftRight)
		{
			if (SlotData.PositionId.HasTag(TAG_SideTag_Right))
			{
				SlotData.PositionId.RemoveTag(TAG_SideTag_Right);
				SlotData.PositionId.AddTag(TAG_SideTag_Left);
			}
			else if (SlotData.PositionId.HasTag(TAG_SideTag_Left))
			{
				SlotData.PositionId.RemoveTag(TAG_SideTag_Left);
				SlotData.PositionId.AddTag(TAG_SideTag_Right);
			}
		}

		FVehicleSlotServerState SlotInfo;
		if (const auto& attachmentIndex = CarDataValue.SlotMapping.Find(SlotData); attachmentIndex != INDEX_NONE)
		{
			SlotInfo.SocketName = CarDataValue.SlotMapping[attachmentIndex].SocketName;
		}

		SlotInfo.bIsFlippedRightLeft = item.IsFlippedLeftRight;
		slotItem->SlotState = SlotInfo;
		slotItem->ForceNetUpdate();
		AssembleComponentsLocal.Add(slotItem);
	}

	AssembleComponents = AssembleComponentsLocal;
	AttachSlots();
	AssembleCarClient(CarDataValue);
	UpdateCarColor(CarDataValue);
}

void UMCVehicleAssemblyComponent::AssembleCarClient_Implementation(FCarDataInventory CarDataValue)
{
	for (const auto& item : CarDataValue.SlotItems)
	{
		if (item.bIsGarageViewOnly && !MainMenuCar)
		{
			continue;
		}

		if (item.SlotId == TAG_VehicleSlotId_Wheel)
		{
			FActorSpawnParameters spawnParams;
			spawnParams.Owner = GetOwner();
			auto* VehicleSlotInstance = GetWorld()->SpawnActor<AMCVehicleSlot>(item.Slot, FVector::ZeroVector, FRotator::ZeroRotator, spawnParams);
			if (!VehicleSlotInstance)
			{
				continue;
			}

			auto* DefaultSlot = item.Slot->GetDefaultObject<AMCVehicleSlot>();
			const auto SlotData = DefaultSlot->SlotData;
			if (GetOwner() && GetOwner()->Implements<UMCVehicleInterface>())
			{
				for (const auto& suspension : IMCVehicleInterface::Execute_GetSuspensions(GetOwner(), SlotData.PositionId))
				{
					suspension->WheelMesh = VehicleSlotInstance->StaticMesh;
					if (suspension->WheelMeshComponent)
					{
						continue;
					}

					suspension->ConstructWheelMeshComponent();
				}
			}

			if (MainMenuCar && MainMenuCar->Implements<UMCVehicleInterface>())
			{
				for (const auto& WheelComponent : IMCVehicleInterface::Execute_GetWheelComponents(MainMenuCar, SlotData.PositionId))
				{
					WheelComponent->SetStaticMesh(VehicleSlotInstance->StaticMesh);
				}
			}

			if (IsValid(DefaultSlot))
			{
				DefaultSlot->ConditionalBeginDestroy();
				DefaultSlot = nullptr;
			}
			if (IsValid(VehicleSlotInstance))
			{
				VehicleSlotInstance->ConditionalBeginDestroy();
				VehicleSlotInstance = nullptr;
			}
		}
	}
}

void UMCVehicleAssemblyComponent::Init()
{
	const auto* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	const ENetMode NetMode = GetNetMode();

	// Handle Standalone, Client, and Listen Server
	if (NetMode == NM_Standalone || NetMode == NM_Client || NetMode == NM_ListenServer)
	{
		SetupClient(); // Call client setup for all non-dedicated cases
	}

	if (const auto& saveSystem = GetWorld()->GetGameInstance()->GetSubsystem<UMCSaveGameSubsystem>(); saveSystem && !saveSystem->bIsSaveLoaded)
	{
		saveSystem->LoadSaveGame();
	}

	AssembleCar();
}

// Called when the game starts
void UMCVehicleAssemblyComponent::BeginPlay()
{
	Super::BeginPlay();
	if (!OwningCar)
	{
		OwningCar = Cast<AMCVehiclePawnBase>(GetOwner());
	}

	MakeMeshReferences();
	Init();
}

void UMCVehicleAssemblyComponent::SetupClient_Implementation()
{
	if (bIsEventSubscribed)
		return;

	if (const auto& inventory = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		inventory->OnCarDataChangedEvent.AddDynamic(this, &UMCVehicleAssemblyComponent::OnCarDataChanged);
		inventory->OnCarColorChangedEvent.AddDynamic(this, &UMCVehicleAssemblyComponent::OnCarColorChanged);
		bIsEventSubscribed = true;
	}
}

void UMCVehicleAssemblyComponent::OnCarDataChanged_Implementation(FCarDataInventory NewCarData)
{
	if (const auto& inventory = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>(); inventory && inventory->IsCarDataInited())
	{
		AssembleCar();
	}
}

void UMCVehicleAssemblyComponent::AssembleCar_Implementation()
{
	const auto* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn && OwnerPawn->IsLocallyControlled())
	{
		if (const auto& inventory = GetWorld()->GetGameInstance()->GetSubsystem<UInventorySubsystem>(); inventory && inventory->IsCarDataInited())
		{
			// Log to confirm client data is used
			UE_LOG(LogTemp, Warning, TEXT("[AssembleCar] Using Local CarData: %s"), *inventory->CarData.CodeName.ToString());
			AssembleCar_Server(inventory->CarData);
		}
	}
}

void UMCVehicleAssemblyComponent::UpdateCarColor_Implementation(FCarDataInventory CarDataValue)
{
	const auto& combatCar = Cast<AMCVehicleCombat>(OwningCar);
	const auto& carColor = UMCBlueprintLib::GetMaterialByColorId(GetWorld(), CarDataValue.ForceMaterial.IsEmpty() ? CarDataValue.CarMaterial : CarDataValue.ForceMaterial, FGameplayTag());
	for (const auto& item : AssembleComponents)
	{
		if (!IsValid(item))
		{
			continue;
		}

		if (!item->OwningCarCombat)
		{
			item->OwningCarCombat = combatCar;
		}

		item->UpdateColor(carColor);
	}

	if (combatCar && carColor)
	{
		combatCar->SetCarColor(carColor);
	}

	if (MainMenuCar)
	{
		MainMenuCar->Body->SetMaterial(CarDataValue.BodyMaterialIndex, carColor);
	}
}

void UMCVehicleAssemblyComponent::Explode_Implementation()
{
	OwningCar = Cast<AMCVehiclePawnBase>(GetOwner());
	if (!OwningCar || AssembleComponents.IsEmpty())
	{
		return;
	}

	const auto& explosionCenter = OwningCar->GetActorLocation();
	for (const auto& component : AssembleComponents)
	{
		if (!component)
		{
			continue;
		}

		component->Explode(explosionCenter, ExplosionForce);
	}
}

void UMCVehicleAssemblyComponent::DestroyComponents()
{
	for (const auto& component : AssembleComponents)
	{
		if (!IsValid(component))
		{
			continue;
		}

		component->Destroy();
	}

	AssembleComponents.Empty();
}

void UMCVehicleAssemblyComponent::AdjustAttachment_Implementation(FCarDataInventory CarDataValue)
{
	if (!CarDataValue.AttachmentSkeleton && CarDataValue.AttachmentSkeletonOffset.IsZero())
	{
		return;
	}

	if (const auto& AttachmentSkeletalMesh = Cast<USkeletalMeshComponent>(MeshComponent); AttachmentSkeletalMesh && AttachmentSkeleton->IsValidLowLevel())
	{
		if (CarDataValue.AttachmentSkeleton)
		{
			AttachmentSkeletalMesh->SetSkeletalMesh(CarDataValue.AttachmentSkeleton);
		}

		if (!CarDataValue.AttachmentSkeletonOffset.IsZero())
		{
			AttachmentSkeletalMesh->SetRelativeLocation(CarDataValue.AttachmentSkeletonOffset);
		}
	}
}

void UMCVehicleAssemblyComponent::MakeMeshReferences()
{
	OwningCar = Cast<AMCVehiclePawnBase>(GetOwner());
	MainMenuCar = Cast<AVehicleMainMenu>(GetOwner());
	if (!MeshComponent)
	{
		auto* OwnerActor = OwningCar ? Cast<AActor>(OwningCar) : Cast<AActor>(Cast<AVehicleMainMenu>(GetOwner()));

		if (OwnerActor)
		{
			UStaticMeshComponent* OwnerMesh = OwningCar ? OwningCar->GetMesh() : (MainMenuCar ? MainMenuCar->GetMesh() : nullptr);

			if (AttachmentSkeleton)
			{
				MeshComponent = NewObject<USkeletalMeshComponent>(OwnerActor);
				MeshComponent->SetupAttachment(OwnerMesh);
			}
			else
			{
				MeshComponent = Cast<USkeletalMeshComponent>(OwnerMesh);
			}

			if (const auto& AttachmentSkeletalMesh = Cast<USkeletalMeshComponent>(MeshComponent); AttachmentSkeletalMesh && AttachmentSkeleton->IsValidLowLevel())
			{
				AttachmentSkeletalMesh->SetSkeletalMesh(AttachmentSkeleton);
				AttachmentSkeletalMesh->SetRelativeLocation(AttachmentSkeletonOffset);
				AttachmentSkeletalMesh->SetIsReplicated(true);
				MeshComponent->RegisterComponent();
			}
		}
	}
}

void UMCVehicleAssemblyComponent::AttachSlots_Implementation()
{
	if (!MeshComponent)
	{
		return;
	}

	for (const auto& slotItem : AssembleComponents)
	{
		if (!slotItem)
		{
			continue;
		}

		slotItem->Attach();
	}
}

void UMCVehicleAssemblyComponent::UpdateDynamicMaterialParameter(FName Name, float Value)
{
	if (AssembleComponents.IsEmpty())
	{
		return;
	}

	for (const auto& component : AssembleComponents)
	{
		if (!IsValid(component))
		{
			continue;
		}

		component->UpdateDynamicMaterialParameter(Name, Value);
	}
}

void UMCVehicleAssemblyComponent::SetDamageAllComponents_Implementation(float NewDamage)
{
	if (AssembleComponents.IsEmpty())
	{
		return;
	}

	for (const auto& component : AssembleComponents)
	{
		if (!IsValid(component))
		{
			continue;
		}

		component->SetDamage(NewDamage);
	}
}

void UMCVehicleAssemblyComponent::OnCarColorChanged_Implementation(FCarDataInventory NewCarData)
{
	UpdateCarColor(NewCarData);
}

void UMCVehicleAssemblyComponent::OnRep_AssembleComponents(TArray<AMCVehicleSlot*> oldValue)
{
	AttachSlots();
	if (const auto& inventory = GetOwner()->GetGameInstance()->GetSubsystem<UInventorySubsystem>())
	{
		AssembleCarClient(inventory->CarData);
	}
}

// Called every frame
void UMCVehicleAssemblyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UMCVehicleAssemblyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMCVehicleAssemblyComponent, AssembleComponents);
	DOREPLIFETIME(UMCVehicleAssemblyComponent, CarData);
}

void UMCVehicleAssemblyComponent::DestroyComponent(bool bPromoteChildren)
{
	DestroyComponents();
	Super::DestroyComponent(bPromoteChildren);
}
