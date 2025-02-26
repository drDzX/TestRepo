// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MCBlueprintLib.generated.h"

struct FGameplayTag;

/**
 *
 */
UCLASS()
class UMCBlueprintLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//****** DEBUG
	UFUNCTION(BlueprintCallable, Category = "Utils")
	static void MCDrawDebugLine(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Utils")
	static void MCDrawDebugPoint(const UWorld* InWorld, FVector const& Position, float Size, FColor const& PointColor, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0);

	//*****
	UFUNCTION(BlueprintCallable, Category = "Utilities|Trace", meta = (WorldContext = "WorldContextObject"))
	static FHitResult GetAimLocation(const UObject* WorldContextObject, AActor* AimingActor, float MaxDistance, ECollisionChannel collisionChannel = ECC_WorldStatic, bool bIsIgnoringSelf = true, FVector2D CameraOffset = FVector2D::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "Utilities|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool Trace(const UObject* WorldContextObject, const AActor* ActorToIgnore, const FVector& Start, const FVector& End, FHitResult& HitOut, ECollisionChannel CollisionChannel = ECC_WorldStatic, bool ReturnPhysMat = false);

	UFUNCTION(BlueprintCallable, Category = "Utilities|Camera")
	static FVector GetCameraPosition(const AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = "Utilities|Trace", meta = (WorldContext = "WorldContextObject"))
	static bool TraceByType(const UObject* WorldContextObject, const AActor* ActorToIgnore, const FVector& Start, const FVector& End, FHitResult& HitOut, ECollisionChannel CollisionChannel = ECC_WorldStatic, bool ReturnPhysMat = false);

	UFUNCTION(BlueprintCallable, Category = "Utilities|Trace", meta = (WorldContext = "WorldContextObject"))
	static TArray<FHitResult> TraceMultiObject(const UObject* WorldContextObject, const AActor* ActorToIgnore, const FVector& Start, const FVector& End, ECollisionChannel CollisionChannel = ECC_WorldStatic);

	UFUNCTION(BlueprintCallable, Category = "MC Abilities", meta = (Keywords = "ApplyDamage", UnsafeDuringActorConstruction = "false"))
	static FActiveGameplayEffectHandle ApplyDamage(const AActor* TargetActor, const AActor* EffectorActor, float damage, float AddCritChance, TSubclassOf<UGameplayEffect> GameplayEffectClass, const FHitResult hitResult, float DamageMultiplier = 1.f, float Duration = 0.f);

	UFUNCTION(BlueprintCallable, Category = "MC Abilities", meta = (Keywords = "UpdateEnergy", UnsafeDuringActorConstruction = "false"))
	static void UpdateEnergy(const AActor* TargetActor, float Amount, TSubclassOf<UGameplayEffect> GameplayEffectClass);

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (WorldContext = "WorldContextObject", Keywords = "Loading Screen", UnsafeDuringActorConstruction = "false"))
	static void ShowLoadingScreen(const UObject* WorldContextObject, float Duration);

	UFUNCTION(BlueprintCallable, Category = "MC Abilities", meta = (Keywords = "UpdateGameEffect", UnsafeDuringActorConstruction = "false"))
	static FActiveGameplayEffectHandle UpdateGameEffect(const AActor* TargetActor, float Amount, TSubclassOf<UGameplayEffect> GameplayEffectClass, FGameplayTag tag);

	UFUNCTION(BlueprintCallable, Category = "MC Abilities", meta = (Keywords = "RemoveGameEffect", UnsafeDuringActorConstruction = "false"))
	static bool RemoveGameEffect(const AActor* TargetActor, FActiveGameplayEffectHandle Handle, int32 StacksToRemove);
	UFUNCTION(BlueprintCallable, Category = "Team", meta = (WorldContext = "WorldContextObject", Keywords = "Team", UnsafeDuringActorConstruction = "false"))
	static TArray<AMCPlayerState*> GetTeamMembers(const UObject* WorldContextObject, FGameplayTag teamTag);
	UFUNCTION(BlueprintCallable, Category = "Gameplay", meta = (WorldContext = "WorldContextObject", Keywords = "Gameplay stable", UnsafeDuringActorConstruction = "false"))
	static bool IsGameplayBlocked(const UObject* WorldContextObject);
	UFUNCTION(BlueprintCallable, Category = "Gameplay", meta = (WorldContext = "WorldContextObject", Keywords = "Material color", UnsafeDuringActorConstruction = "false"))
	static UMaterialInterface* GetMaterialByColorId(const UObject* WorldContextObject, const FString& ColorId, FGameplayTag TeamTag);

	UFUNCTION(BlueprintCallable, Category = "Utils", meta = (WorldContext = "WorldContextObject", Keywords = "Save game", UnsafeDuringActorConstruction = "false"))
	static void SaveGame(const UObject* WorldContextObject);
};
