// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/MCGameModeBase.h"
#include "GameFramework/PlayerStart.h"
#include "BlueprintGameplayTagLibrary.h"
#include "MCGameInterface.h"
#include "MCPlayerStart.h"
#include "MCPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/RandomShuffle.h"

AActor* AMCGameModeBase::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	OnFindStartTriggeredEvent.Broadcast();
	FGameplayTag currentTeam;
	if (Player && Player->Implements<UMCGameInterface>())
	{
		currentTeam = IMCGameInterface::Execute_GetPlayerTeam(Player);
	}

	TArray<AActor*> FoundActors;
	if (currentTeam.IsValid())
	{
		UBlueprintGameplayTagLibrary::GetAllActorsOfClassMatchingTagQuery(GetWorld(), AMCPlayerStart::StaticClass(), FGameplayTagQuery::MakeQuery_MatchTag(currentTeam), FoundActors);
	}

	if (currentTeam.IsValid() && FoundActors.IsEmpty() || !currentTeam.IsValid())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);
	}

	if (FoundActors.IsEmpty())
	{
		return nullptr;
	}

	AMCPlayerStart* selectedStart = nullptr;
	if (PlayerSpawnMap.Contains(Player) && FoundActors.Contains(PlayerSpawnMap[Player]))
	{
		const auto& PlayerStart = Cast<AMCPlayerStart>(PlayerSpawnMap[Player]);
		if (PlayerStart && PlayerStart->OccupiedActor && PlayerStart->OccupiedActor == Player)
		{
			return PlayerSpawnMap[Player];
		}
	}

	Algo::RandomShuffle(FoundActors);
	for (AActor* StartPoint : FoundActors)
	{
		const auto& PlayerStart = Cast<AMCPlayerStart>(StartPoint);
		if (!PlayerStart)
		{
			continue;
		}

		if (!PlayerStart->OccupiedActor && !selectedStart) // Custom check function
		{
			PlayerStart->ChangeOccupiedActor(Player);
			selectedStart = PlayerStart;
			PlayerSpawnMap.Add(Player, selectedStart);
		}
	}

	//If we don't find the start for the first team check spawn spots for the second
	if (!selectedStart)
	{
		FGameplayTag oppositeTag = currentTeam == TAG_Team_A ? TAG_Team_B : TAG_Team_A;
		UBlueprintGameplayTagLibrary::GetAllActorsOfClassMatchingTagQuery(GetWorld(), AMCPlayerStart::StaticClass(), FGameplayTagQuery::MakeQuery_MatchTag(oppositeTag), FoundActors);

		Algo::RandomShuffle(FoundActors);
		for (AActor* StartPoint : FoundActors)
		{
			const auto& PlayerStart = Cast<AMCPlayerStart>(StartPoint);
			if (!PlayerStart)
			{
				continue;
			}

			if (!PlayerStart->OccupiedActor && !selectedStart)
			{
				PlayerStart->ChangeOccupiedActor(Player);
				selectedStart = PlayerStart;
				PlayerSpawnMap.Add(Player, selectedStart);
			}
		}
	}

	return selectedStart;
}
