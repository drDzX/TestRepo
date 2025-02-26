// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "MCVehicleInterface.h"
#include "NativeGameplayTags.h"
#include "WheeledVehiclePawn.h"
#include "Vehicle/RTuneVehicle.h"
#include "MCVehiclePawnBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBrakeActivity, bool, IsActive);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Handbrake);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Throttle);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Brake);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Input_Steer);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Movement_DisableAll);

class UInputMappingContext;
class UInputConfig;
struct FGameplayAbilitySpecHandle;
class UInputMappingContext;
struct FInputActionValue;
class UInputConfig;
class UGameplayEffect;
class UMCGameplayAbility;
class UMCAbilitySystemComponent;
class UMCAttributeSet;

/**
 *
 */
UCLASS()
class AMCVehiclePawnBase : public ARTuneVehicle, public IAbilitySystemInterface, public IGameplayTagAssetInterface, public IMCVehicleInterface
{
	GENERATED_BODY()

public:
	AMCVehiclePawnBase(const FObjectInitializer& ObjectInitializer);
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UMCAttributeSet* GetAttributes() const { return Attributes; }

	UFUNCTION(BlueprintCallable)
	FGameplayAbilitySpecHandle GiveGameplayAbility(TSubclassOf<UMCGameplayAbility> ability, int level, FGameplayTag InputTag);

	UFUNCTION(BlueprintCallable)
	void RefreshAbilityBinds();

	/** Returns CameraBoom subobject **/
	FORCEINLINE class UMCSpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Server, Reliable)
	void CheckBlockMovementTags();
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void CheckBlockMovementTagsEvent(FGameplayTag currentState);

	UFUNCTION(Server, Reliable)
	void SetServerHandbrake(bool newValue);
	UStaticMeshComponent* GetMesh();

	UFUNCTION(BlueprintPure)
	bool IsMovingOnGround();

	UFUNCTION(BlueprintCallable, NetMulticast, Reliable)
	void OnBrakeActivityChangedMulticast(bool NewActivity);

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	void InputAbilityInputTagPressed(FGameplayTag InputTag);
	void InputAbilityInputTagReleased(FGameplayTag InputTag);
	UFUNCTION(BlueprintCallable, Category = "GameplayTags")
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UseSteere(const FInputActionValue& InputActionValue);
	void UseThrottle(const FInputActionValue& InputActionValue);
	void UseBrake(const FInputActionValue& InputActionValue);
	void HandBrake(const FInputActionValue& InputActionValue);
	/**
 * Called via input to turn at a given rate.
 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handles mouse and stick look */
	void InputTurn(const FInputActionValue& InputActionValue);

	UFUNCTION(Server, Reliable)
	void ParkCarServer(bool isParked);
	UFUNCTION(Server, Reliable)
	void MoveCarEvent();

private:
	UFUNCTION(Client, Reliable)
	void AssignInputMapping(bool IsRemoving = false);
	void AddStartupGameplayAbilities();

	UFUNCTION(Client, Reliable)
	virtual void OnRep_MCTags();

public:
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_Controller() override;
	virtual void BeginPlay() override;
	virtual bool IsMovementAllowed_Implementation() override;
	virtual void NativeAsyncTick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputConfig* InputConfig;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TArray<UInputMappingContext*> InputMappings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Body;
	// --------------------- GAME PLAY ABILITIES
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMCAbilitySystemComponent> AbilitySystemComponent;
	//MCAttributeSet is defined in C++
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes")
	TObjectPtr<UMCAttributeSet> Attributes;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TMap<FGameplayTag, TSubclassOf<UMCGameplayAbility>> GameplayAbilities;
	UPROPERTY()
	bool bAbilitiesInitialized = false;

	UPROPERTY(ReplicatedUsing = OnRep_MCTags, EditAnywhere, BlueprintReadWrite, Category = "Tags")
	FGameplayTagContainer MCTags;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<AController> owningController = nullptr;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Input)
	float TurnRateGamepad = 90.f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
	bool bIsHandBraking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float SideSlipAngle = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTuneVehicle|Physics")
	float PushDownZForce = 1000000.f;
	UPROPERTY(BlueprintAssignable, BlueprintReadWrite, VisibleAnywhere)
	FBrakeActivity BrakeActivityEvent;

protected:
	/** Camera boom positioning the camera behind the car */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UMCSpringArmComponent* CameraBoom;
	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
};
