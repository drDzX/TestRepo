// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/MCVehiclePawnBase.h"

#include "AsyncTickFunctions.h"
#include "Abilities/MCAbilitySystemComponent.h"
#include "Abilities/MCAttributeSet.h"
#include "Abilities/MCGameplayAbility.h"
#include "EnhancedInputSubsystems.h"
#include "GameplayTagContainer.h"
#include "MCSpringArmComponent.h"
#include "MCVehicleMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Core/MCGameModeBase.h"
#include "Core/MCGameState.h"
#include "Core/MCPlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Inputs/MCEnhInputComponent.h"
#include "Net/UnrealNetwork.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Handbrake, "Input.Vehicle.Handbrake")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Throttle, "Input.Vehicle.Throttle")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Brake, "Input.Vehicle.Brake")
UE_DEFINE_GAMEPLAY_TAG(TAG_Input_Steer, "Input.Vehicle.Steer")
UE_DEFINE_GAMEPLAY_TAG(TAG_Movement_DisableAll, "Movement.DisableAllMovement");

AMCVehiclePawnBase::AMCVehiclePawnBase(const FObjectInitializer& ObjectInitializer) :
	Super()
{
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Car Body"));
	RootComponent = Body;

	AbilitySystemComponent = CreateDefaultSubobject<UMCAbilitySystemComponent>(TEXT("Ability System"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	Attributes = CreateDefaultSubobject<UMCAttributeSet>(TEXT("Attributes"));
	SetReplicates(true);

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<UMCSpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent, "Root");
	CameraBoom->TargetArmLength = 400.0f;       // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

UAbilitySystemComponent* AMCVehiclePawnBase::GetAbilitySystemComponent() const
{
	verify(AbilitySystemComponent);
	return AbilitySystemComponent;
}

FGameplayAbilitySpecHandle AMCVehiclePawnBase::GiveGameplayAbility(TSubclassOf<UMCGameplayAbility> ability, int level, FGameplayTag InputTag)
{
	check(AbilitySystemComponent);
	FGameplayAbilitySpec spec(ability, level);
	spec.DynamicAbilityTags.AddTag(InputTag);

	return AbilitySystemComponent->GiveAbility(spec);
}

void AMCVehiclePawnBase::RefreshAbilityBinds()
{
	if (!AbilitySystemComponent && !InputComponent)
	{
		return;
	}

	const FGameplayAbilityInputBinds binds(
		"Confirm",
		"Cancel",
		FTopLevelAssetPath(GetPathNameSafe(UClass::TryFindTypeSlowSafe<UEnum>("/Script/Machine_Challenger.EMCAbilityInputId"))),
		static_cast<int32>(EMCAbilityInputId::Confirm),
		static_cast<int32>(EMCAbilityInputId::Cancel)
	);

	AbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, binds);
}

void AMCVehiclePawnBase::CheckBlockMovementTags_Implementation()
{
	const auto& world = GetWorld();
	if (!world)
	{
		MCTags.RemoveTag(TAG_Movement_DisableAll);
		return;
	}
	const auto& gameState = GetWorld()->GetGameState();
	if (!gameState)
	{
		MCTags.RemoveTag(TAG_Movement_DisableAll);
		return;
	}

	if (gameState && gameState->Implements<UMCGameInterface>())
	{
		if (IMCGameInterface::Execute_IsMovementAllowed(gameState))
		{
			MCTags.RemoveTag(TAG_Movement_DisableAll);
			AbilitySystemComponent->RemoveGameplayTag(TAG_Movement_DisableAll);
		}
		else
		{
			MCTags.AddTag(TAG_Movement_DisableAll);
			AbilitySystemComponent->AddGameplayTag(TAG_Movement_DisableAll);
		}

		return;
	}

	MCTags.RemoveTag(TAG_Movement_DisableAll);
}

void AMCVehiclePawnBase::CheckBlockMovementTagsEvent_Implementation(FGameplayTag currentState)
{
	CheckBlockMovementTags();
	//TODO FIX GetVehicleMovementComponent()->StopMovementImmediately();
}

void AMCVehiclePawnBase::SetServerHandbrake_Implementation(bool newValue)
{
	bIsHandBraking = newValue;
	ApplyHandbrakeInput(newValue);
}

UStaticMeshComponent* AMCVehiclePawnBase::GetMesh()
{
	return Body;
}

bool AMCVehiclePawnBase::IsMovingOnGround()
{
	return !bInAir;
}

void AMCVehiclePawnBase::OnBrakeActivityChangedMulticast_Implementation(bool NewActivity)
{
	BrakeActivityEvent.Broadcast(NewActivity);
}

void AMCVehiclePawnBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UMCEnhInputComponent* mcEnhancedInputComponent = Cast<UMCEnhInputComponent>(PlayerInputComponent);
	if (!mcEnhancedInputComponent)
	{
		return;
	}

	// Set up gameplay key bindings
	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Steer, ETriggerEvent::Triggered, this, &AMCVehiclePawnBase::UseSteere);
	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Steer, ETriggerEvent::Completed, this, &AMCVehiclePawnBase::UseSteere);

	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Throttle, ETriggerEvent::Triggered, this, &AMCVehiclePawnBase::UseThrottle);
	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Throttle, ETriggerEvent::Completed, this, &AMCVehiclePawnBase::UseThrottle);

	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Brake, ETriggerEvent::Triggered, this, &AMCVehiclePawnBase::UseBrake);
	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Brake, ETriggerEvent::Completed, this, &AMCVehiclePawnBase::UseBrake);

	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Handbrake, ETriggerEvent::Triggered, this, &AMCVehiclePawnBase::HandBrake);
	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_Handbrake, ETriggerEvent::Completed, this, &AMCVehiclePawnBase::HandBrake);

	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_TurnMouse, ETriggerEvent::Triggered, this, &AMCVehiclePawnBase::InputTurn);
	mcEnhancedInputComponent->BindActionByTag(InputConfig, TAG_Input_TurnGamepad, ETriggerEvent::Triggered, this, &AMCVehiclePawnBase::InputTurn);

	// Set up gameplay key bindings
	TArray<uint32> BindHandles;
	mcEnhancedInputComponent->BindAbilityActions(InputConfig, this, &ThisClass::InputAbilityInputTagPressed, &ThisClass::InputAbilityInputTagReleased, /*out*/ BindHandles);
	RefreshAbilityBinds();
}

void AMCVehiclePawnBase::InputAbilityInputTagPressed(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagPressed(InputTag);
	}
}

void AMCVehiclePawnBase::InputAbilityInputTagReleased(FGameplayTag InputTag)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityInputTagReleased(InputTag);
	}
}

void AMCVehiclePawnBase::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AppendTags(MCTags);
	FGameplayTagContainer tagContainer;
	AbilitySystemComponent->GetOwnedGameplayTags(tagContainer);
	TagContainer.AppendTags(tagContainer);
}

void AMCVehiclePawnBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMCVehiclePawnBase, AbilitySystemComponent);
	DOREPLIFETIME(AMCVehiclePawnBase, MCTags);
	DOREPLIFETIME(AMCVehiclePawnBase, owningController);
	DOREPLIFETIME(AMCVehiclePawnBase, bIsHandBraking);
}

void AMCVehiclePawnBase::UseSteere(const FInputActionValue& InputActionValue)
{
	const auto& inputVal = InputActionValue.Get<float>();
	SetSteeringInput(inputVal);
}

void AMCVehiclePawnBase::UseThrottle(const FInputActionValue& InputActionValue)
{
	if (HasMatchingGameplayTag(TAG_Movement_DisableAll))
	{
		return;
	}

	const auto& inputVal = InputActionValue.Get<float>();

	ParkCarServer(false);
	SetThrottleInput(inputVal);
}

void AMCVehiclePawnBase::UseBrake(const FInputActionValue& InputActionValue)
{
	if (HasMatchingGameplayTag(TAG_Movement_DisableAll))
	{
		return;
	}

	const auto& inputVal = InputActionValue.Get<float>();
	ParkCarServer(false);

	SetBrakeInput(inputVal);
}

void AMCVehiclePawnBase::HandBrake(const FInputActionValue& InputActionValue)
{
	if (HasMatchingGameplayTag(TAG_Movement_DisableAll))
	{
		return;
	}

	const auto& inputVal = InputActionValue.Get<bool>();
	ApplyHandbrakeInput(inputVal);
	SetServerHandbrake(inputVal);
}

void AMCVehiclePawnBase::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}

void AMCVehiclePawnBase::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}

void AMCVehiclePawnBase::InputTurn(const FInputActionValue& InputActionValue)
{
	if (!Controller)
	{
		return;
	}

	const auto& LookValue = InputActionValue.Get<FVector2D>();

	if (LookValue.X != 0.0f)
	{
		TurnAtRate(LookValue.X);
	}

	if (LookValue.Y != 0.0f)
	{
		LookUpAtRate(LookValue.Y);
	}
}

void AMCVehiclePawnBase::MoveCarEvent_Implementation()
{
	const auto& world = GetWorld();
	if (!world)
	{
		return;
	}

	if (const auto& MCGameMode = Cast<AMCGameModeBase>(world->GetAuthGameMode()))
	{
		MCGameMode->OnSpawnReleasedEvent.Broadcast(owningController);
	}
}

void AMCVehiclePawnBase::ParkCarServer_Implementation(bool isParked)
{
	if (bIsIdleLocked == isParked)
	{
		return;
	}

	if (!isParked)
	{
		MoveCarEvent();
	}

	bIsIdleLocked = isParked;
}

void AMCVehiclePawnBase::AddStartupGameplayAbilities()
{
	check(AbilitySystemComponent);
	if (GetLocalRole() != ROLE_Authority && !bAbilitiesInitialized)
	{
		return;
	}
	//Grant abilities on server
	for (const auto& [abilityInput, abilityTemplate] : GameplayAbilities)
	{
		GiveGameplayAbility(abilityTemplate, 1, abilityInput);
	}

	for (const auto& gameEffect : PassiveGameplayEffects)
	{
		auto effectContext = AbilitySystemComponent->MakeEffectContext();
		effectContext.AddSourceObject(this);

		auto NewHandle = AbilitySystemComponent->MakeOutgoingSpec(gameEffect, 1, effectContext);

		if (!NewHandle.IsValid())
		{
			continue;
		}

		auto activeGameplayEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent);
	}

	bAbilitiesInitialized = true;
}

void AMCVehiclePawnBase::OnRep_MCTags_Implementation()
{
}

void AMCVehiclePawnBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	owningController = NewController;
	AssignInputMapping();
	AddStartupGameplayAbilities();
	if (const auto& controller = Cast<AMCPlayerController>(NewController); controller && HasAuthority())
	{
		controller->SetMainCar(this);
	}
}

void AMCVehiclePawnBase::UnPossessed()
{
	Super::UnPossessed();
	AssignInputMapping(true);
	owningController = nullptr;
}

void AMCVehiclePawnBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	AssignInputMapping();
}

void AMCVehiclePawnBase::BeginPlay()
{
	Super::BeginPlay();
	CheckBlockMovementTags();
	if (HasAuthority())
	{
		ParkCarServer(true);
	}
	const auto& mcGameState = Cast<AMCGameState>(GetWorld()->GetGameState());
	if (!mcGameState)
	{
		return;
	}

	mcGameState->OnStateChangedDelegate.AddDynamic(this, &AMCVehiclePawnBase::CheckBlockMovementTagsEvent);
}

bool AMCVehiclePawnBase::IsMovementAllowed_Implementation()
{
	return !HasMatchingGameplayTag(TAG_Movement_DisableAll);
}

void AMCVehiclePawnBase::NativeAsyncTick(float DeltaTime)
{
	if (CameraBoom)
	{
		CameraBoom->AsyncTick(DeltaTime);
	}
	Super::NativeAsyncTick(DeltaTime);
	SideSlipAngle = GetSideSlipAngle();

	if (AirbornWheelCount > 2)
	{
		UAsyncTickFunctions::ATP_AddForce(PrimitiveComponent, FVector::ZAxisVector * -PushDownZForce, false);
	}
}

void AMCVehiclePawnBase::AssignInputMapping_Implementation(bool IsRemoving)
{
	if (InputMappings.IsEmpty() || !owningController)
	{
		return;
	}

	const auto& playerController = GetController<APlayerController>();
	if (!playerController)
	{
		return;
	}

	const auto& localPlayer = playerController->GetLocalPlayer();
	if (!ensure(localPlayer))
	{
		return;
	}

	const auto& subsystem = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!ensure(subsystem))
	{
		return;
	}

	int i = 0;
	subsystem->ClearAllMappings();
	for (const auto& context : InputMappings)
	{
		if (!context)
		{
			continue;
		}

		if (IsRemoving)
		{
			subsystem->RemoveMappingContext(context);
			continue;
		}

		subsystem->AddMappingContext(context, i);
		i++;
	}
}
