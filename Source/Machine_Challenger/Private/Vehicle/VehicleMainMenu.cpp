// Fill out your copyright notice in the Description page of Project Settings.

#include "Vehicle/VehicleMainMenu.h"

// Sets default values
AVehicleMainMenu::AVehicleMainMenu()
{
	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Car Body"));
	Body->SetupAttachment(RootComponent, "Root");
}

UStaticMeshComponent* AVehicleMainMenu::GetMesh()
{
	return Body;
}

// Called when the game starts or when spawned
void AVehicleMainMenu::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AVehicleMainMenu::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AVehicleMainMenu::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
