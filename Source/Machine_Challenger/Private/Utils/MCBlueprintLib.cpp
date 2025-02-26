// Fill out your copyright notice in the Description page of Project Settings.

#include "Utils/MCBlueprintLib.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Core/MCPlayerState.h"
#include "Abilities/MCAbilitySystemComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Core/MCGameInstance.h"
#include "Core/MCGameInterface.h"
#include "Core/MCServerInterface.h"
#include "Core/SaveGame/MCSaveGameSubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "Inventory/InventorySubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Vehicle/MCVehiclePawnBase.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_Health, "GameEffect.Health");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_Duration, "GameEffect.Damage.Duration");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_Energy, "GameEffect.Energy");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_CritChance, "GameEffect.Stats.CritChance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_GameEffect_DamageMultiplier, "GameEffect.Stats.DamageMultiplier");

void UMCBlueprintLib::MCDrawDebugLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
{
	// TODO: insert Draw debug logic
}

void UMCBlueprintLib::MCDrawDebugPoint(const UWorld* InWorld, FVector const& Position, float Size, FColor const& PointColor, bool bPersistentLines, float LifeTime, uint8 DepthPriority)
{
	// TODO: insert Draw debug logic
}

FHitResult UMCBlueprintLib::GetAimLocation(const UObject* WorldContextObject, AActor* AimingActor, float MaxDistance, ECollisionChannel collisionChannel, bool bIsIgnoringSelf, FVector2D CameraOffset)
{
	FHitResult hitData(ForceInit);
	if (!AimingActor)
	{
		return hitData;
	}

	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return hitData;
	}

	FVector cameraLoc;
	FRotator cameraRot;
	const auto& instigatorController = Cast<APlayerController>(AimingActor->GetInstigatorController());

	if (!instigatorController)
	{
		return hitData;
	}

	instigatorController->GetPlayerViewPoint(cameraLoc, cameraRot);

	// Get the screen dimensions
	int32 ViewportWidth, ViewportHeight;
	instigatorController->GetViewportSize(ViewportWidth, ViewportHeight);
	const auto& dpiScale = UWidgetLayoutLibrary::GetViewportScale(instigatorController);
	// Convert the screen space offset to world space
	FVector WorldOffset;
	instigatorController->DeprojectScreenPositionToWorld(ViewportWidth / 2 + CameraOffset.X * dpiScale, ViewportHeight / 2 + CameraOffset.Y * dpiScale, cameraLoc, WorldOffset);

	// Add the world space offset to the viewpoint location
	FVector ViewpointCenterWithOffset = cameraLoc + WorldOffset * MaxDistance;

	//	DrawDebugPoint(World, ViewpointCenterWithOffset, 10.f, FColor::Black, true);
	UMCBlueprintLib::MCDrawDebugLine(World, cameraLoc, ViewpointCenterWithOffset, FColor::Blue, false, 10.f);

	const auto& ignoreActor = bIsIgnoringSelf ? AimingActor : nullptr;
	if (!Trace(World, ignoreActor, cameraLoc, ViewpointCenterWithOffset, hitData, collisionChannel))
	{
		hitData.Location = FVector(ViewpointCenterWithOffset);
		UMCBlueprintLib::MCDrawDebugPoint(World, hitData.Location, 10.f, FColor::Yellow, false, 10.0f);
		return hitData;
	}

	UMCBlueprintLib::MCDrawDebugPoint(World, hitData.Location, 10.f, FColor::Red, false, 10.0f);
	return hitData;
}

bool UMCBlueprintLib::Trace(const UObject* WorldContextObject, const AActor* ActorToIgnore, const FVector& Start, const FVector& End, FHitResult& HitOut, ECollisionChannel CollisionChannel, bool ReturnPhysMat)
{
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams TraceParams(FName(TEXT("Trace Path")), true, ActorToIgnore);
	TraceParams.bTraceComplex = true;
	TraceParams.bReturnPhysicalMaterial = ReturnPhysMat;

	//Ignore Actors
	if (ActorToIgnore)
	{
		TraceParams.AddIgnoredActor(ActorToIgnore);
	}

	//Re-initialize hit info
	HitOut = FHitResult(ForceInit);

	//Trace!
	World->LineTraceSingleByChannel(HitOut, Start, End, CollisionChannel, TraceParams);

	//Hit any Actor?
	return (HitOut.GetActor() != nullptr);
}

FVector UMCBlueprintLib::GetCameraPosition(const AActor* Actor)
{
	FVector cameraLoc;
	FRotator cameraRot;
	const auto& instigatorController = Actor->GetInstigatorController();
	if (!instigatorController)
	{
		return cameraLoc;
	}

	instigatorController->GetPlayerViewPoint(cameraLoc, cameraRot);

	return cameraLoc;
}

bool UMCBlueprintLib::TraceByType(const UObject* WorldContextObject, const AActor* ActorToIgnore, const FVector& Start, const FVector& End, FHitResult& HitOut, ECollisionChannel CollisionChannel, bool ReturnPhysMat)
{
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams TraceParams(FName(TEXT("Trace Path")), true, ActorToIgnore);
	TraceParams.bTraceComplex = true;
	TraceParams.bReturnPhysicalMaterial = ReturnPhysMat;

	//Ignore Actors
	if (ActorToIgnore)
	{
		TraceParams.AddIgnoredActor(ActorToIgnore);
	}

	//Re-initialize hit info
	HitOut = FHitResult(ForceInit);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(CollisionChannel);

	//Trace!
	World->LineTraceSingleByObjectType(HitOut, Start, End, ObjectParams, TraceParams);

	//Hit any Actor?
	return (HitOut.GetActor() != nullptr);
}

TArray<FHitResult> UMCBlueprintLib::TraceMultiObject(const UObject* WorldContextObject, const AActor* ActorToIgnore, const FVector& Start, const FVector& End, ECollisionChannel CollisionChannel)
{
	TArray<FHitResult> HitResult;
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return HitResult;
	}

	FCollisionQueryParams TraceParams(FName(TEXT("Trace Path")), true, ActorToIgnore);
	TraceParams.bTraceComplex = true;
	TraceParams.bReturnPhysicalMaterial = false;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(CollisionChannel);
	TArray<FHitResult> TempHitResult;
	World->LineTraceMultiByObjectType(
		TempHitResult,
		Start,
		End,
		ObjectParams,
		TraceParams
	);

	TArray<AActor*> UniqueActors;
	//Filter unique objects
	for (const auto& hit : TempHitResult)
	{
		AActor* HitActor = hit.GetActor();
		if (HitActor && !UniqueActors.Contains(HitActor))
		{
			HitResult.Add(hit);
			UniqueActors.Add(HitActor);
		}
	}

	return HitResult;
}

FActiveGameplayEffectHandle UMCBlueprintLib::ApplyDamage(const AActor* TargetActor, const AActor* EffectorActor, float damage, float AddCritChance, TSubclassOf<UGameplayEffect> GameplayEffectClass, const FHitResult hitResult, float DamageMultiplier, float Duration)
{
	const auto& targetActorDW = Cast<AMCVehiclePawnBase>(TargetActor);
	const auto& effectorActorDW = Cast<AMCVehiclePawnBase>(EffectorActor);

	if (!targetActorDW)
	{
		return FActiveGameplayEffectHandle();;
	}
	FGameplayEffectContextHandle effectContext;
	const auto& abilityComponentTarget = targetActorDW->AbilitySystemComponent;
	if (effectorActorDW)
	{
		const auto& abilityComponentEffector = effectorActorDW->AbilitySystemComponent;
		effectContext = abilityComponentEffector->MakeEffectContext();
	}

	effectContext.AddHitResult(hitResult);
	const auto NewHandle = abilityComponentTarget->MakeOutgoingSpec(GameplayEffectClass, 1, effectContext);
	NewHandle.Data.Get()->SetSetByCallerMagnitude(TAG_GameEffect_Health, damage);
	if (Duration > 0.f)
	{
		NewHandle.Data.Get()->SetSetByCallerMagnitude(TAG_GameEffect_Duration, Duration);
	}

	NewHandle.Data.Get()->SetSetByCallerMagnitude(TAG_GameEffect_CritChance, AddCritChance);
	NewHandle.Data.Get()->SetSetByCallerMagnitude(TAG_GameEffect_DamageMultiplier, DamageMultiplier);

	return abilityComponentTarget->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), abilityComponentTarget);
}

void UMCBlueprintLib::UpdateEnergy(const AActor* TargetActor, float Amount, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	const auto& targetActorDW = Cast<AMCVehiclePawnBase>(TargetActor);
	if (!targetActorDW)
	{
		return;
	}

	const auto& abilityComponentTarget = targetActorDW->AbilitySystemComponent;
	if (!abilityComponentTarget)
	{
		return;
	}

	auto effectContext = abilityComponentTarget->MakeEffectContext();
	effectContext.AddSourceObject(targetActorDW);
	const auto& EffectHandle = abilityComponentTarget->MakeOutgoingSpec(GameplayEffectClass, 0, effectContext);
	if (!EffectHandle.IsValid())
	{
		return;
	}

	const auto& SpecHandle = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectHandle, TAG_GameEffect_Energy, Amount);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	abilityComponentTarget->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), abilityComponentTarget);
}

void UMCBlueprintLib::ShowLoadingScreen(const UObject* WorldContextObject, float Duration)
{
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	if (!World->GetGameInstance()->Implements<UMCServerInterface>())
	{
		return;
	}

	IMCServerInterface::Execute_ShowLoadingScreen(World->GetGameInstance(), Duration);
}

FActiveGameplayEffectHandle UMCBlueprintLib::UpdateGameEffect(const AActor* TargetActor, float Amount, TSubclassOf<UGameplayEffect> GameplayEffectClass, FGameplayTag tag)
{
	const auto& targetActorDW = Cast<AMCVehiclePawnBase>(TargetActor);
	if (!targetActorDW)
	{
		return FActiveGameplayEffectHandle();
	}

	const auto& abilityComponentTarget = targetActorDW->AbilitySystemComponent;
	if (!abilityComponentTarget)
	{
		return FActiveGameplayEffectHandle();
	}

	auto effectContext = abilityComponentTarget->MakeEffectContext();
	effectContext.AddSourceObject(targetActorDW);
	const auto& EffectHandle = abilityComponentTarget->MakeOutgoingSpec(GameplayEffectClass, 0, effectContext);
	if (!EffectHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	const auto& SpecHandle = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectHandle, tag, Amount);
	if (!SpecHandle.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	auto Modifiers = SpecHandle.Data->Def->Modifiers;
	for (const auto& modifier : SpecHandle.Data->Def->Modifiers)
	{
		const auto& tagMod = modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag;
		if (tagMod != tag)
		{
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectHandle, tagMod, 0);
		}
	}

	return abilityComponentTarget->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), abilityComponentTarget);
}

bool UMCBlueprintLib::RemoveGameEffect(const AActor* TargetActor, FActiveGameplayEffectHandle Handle, int32 StacksToRemove)
{
	const auto& targetActorDW = Cast<AMCVehiclePawnBase>(TargetActor);
	if (!targetActorDW)
	{
		return false;
	}

	const auto& abilityComponentTarget = targetActorDW->AbilitySystemComponent;
	if (!abilityComponentTarget)
	{
		return false;
	}

	return abilityComponentTarget->RemoveActiveGameplayEffect(Handle, StacksToRemove);
}

TArray<AMCPlayerState*> UMCBlueprintLib::GetTeamMembers(const UObject* WorldContextObject, FGameplayTag teamTag)
{
	TArray<AMCPlayerState*> FoundActors;
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return FoundActors;
	}

	const auto& gameState = World->GetGameState();
	if (!gameState)
	{
		return FoundActors;
	}

	if (gameState->PlayerArray.IsEmpty())
	{
		return FoundActors;
	}

	for (const auto& player : gameState->PlayerArray)
	{
		const auto& MCPlayerState = Cast<AMCPlayerState>(player);
		if (!MCPlayerState || !MCPlayerState->HasMatchingGameplayTag(teamTag))
		{
			continue;
		}

		FoundActors.Add(MCPlayerState);
	}

	return FoundActors;
}

bool UMCBlueprintLib::IsGameplayBlocked(const UObject* WorldContextObject)
{
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return true;
	}

	const auto& gameState = World->GetGameState();
	if (!gameState)
	{
		return true;
	}

	if (gameState->Implements<UMCGameInterface>())
	{
		return !IMCGameInterface::Execute_IsMovementAllowed(gameState);
	}

	return false;
}

UMaterialInterface* UMCBlueprintLib::GetMaterialByColorId(const UObject* WorldContextObject, const FString& ColorId, FGameplayTag TeamTag)
{
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	const auto& gameInstance = World->GetGameInstance<UMCGameInstance>();
	if (!gameInstance)
	{
		return nullptr;
	}

	if (!gameInstance->ColorMaterials)
	{
		UE_LOG(LogTemp, Warning, TEXT("CarMaterials DataTable is null!"));
		return nullptr;
	}

	// Iterate over the rows in the DataTable
	static const FString ContextString(TEXT("CarMaterialLookup"));
	TArray<FCarPaintData*> Rows;
	gameInstance->ColorMaterials->GetAllRows<FCarPaintData>(ContextString, Rows);

	// Iterate through rows to find the matching ColorId
	for (const auto& Row : Rows)
	{
		if (Row && Row->ColorId == ColorId)
		{
			if (TeamTag.IsValid() && TeamTag != Row->TeamTag)
			{
				continue;
			}

			UE_LOG(LogTemp, Log, TEXT("Found Row with ColorId: %s"), *Row->ColorId);
			return Row->Material; // Return the matching row
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("ColorId %s not found in CarMaterials DataTable!"), *ColorId);
	return nullptr;
}

void UMCBlueprintLib::SaveGame(const UObject* WorldContextObject)
{
	const auto& World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	const auto& gameInstance = World->GetGameInstance<UMCGameInstance>();
	if (!gameInstance)
	{
		return;
	}

	if (const auto& inventory = gameInstance->GetSubsystem<UInventorySubsystem>())
	{
		inventory->UpdateCarData(inventory->SelectedCarId);
	}

	if (const auto& saveSystem = gameInstance->GetSubsystem<UMCSaveGameSubsystem>())
	{
		saveSystem->WriteSaveGame();
	}
}
