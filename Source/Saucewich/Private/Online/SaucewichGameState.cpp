// Copyright 2019 Team Sosweet. All Rights Reserved.

#include "Online/SaucewichGameState.h"

#include "Engine/World.h"
#include "GameFramework/GameMode.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"

#include "Online/SaucewichGameMode.h"
#include "Player/SaucewichPlayerState.h"
#include "Player/TpsCharacter.h"
#include "Weapon/Weapon.h"
#include "SaucewichGameInstance.h"

template <class Fn>
void ForEachEveryPlayer(const TArray<APlayerState*>& PlayerArray, Fn&& Do)
{
	for (const auto PS : PlayerArray)
		if (const auto P = Cast<ASaucewichPlayerState>(PS))
			Do(P);
}

template <class Fn>
void ForEachPlayer(const TArray<APlayerState*>& PlayerArray, const uint8 Team, Fn&& Do)
{
	ForEachEveryPlayer(PlayerArray, [Team, &Do](const auto P){if (P->GetTeam() == Team) Do(P);});
}

TArray<ASaucewichPlayerState*> ASaucewichGameState::GetPlayersByTeam(const uint8 Team) const
{
	TArray<ASaucewichPlayerState*> Players;
	ForEachPlayer(PlayerArray, Team, [&Players](const auto P){Players.Add(P);});
	return Players;
}

TArray<ATpsCharacter*> ASaucewichGameState::GetCharactersByTeam(const uint8 Team) const
{
	TArray<ATpsCharacter*> Characters;
	ForEachPlayer(PlayerArray, Team, [&Characters](ASaucewichPlayerState* const P) {
		if (const auto C = P->GetPawn<ATpsCharacter>()) Characters.Add(C);
	});
	return Characters;
}

uint8 ASaucewichGameState::GetMinPlayerTeam() const
{
	TArray<uint8, TInlineAllocator<4>> Num;
	Num.AddZeroed(Teams.Num() - 1);
	ForEachEveryPlayer(PlayerArray, [&Num](auto P)
	{
		const auto Team = P->GetTeam() - 1;
		if (Num.IsValidIndex(Team)) ++Num[Team];
	});
	TArray<uint8, TInlineAllocator<4>> Min{0};
	for (auto i = 1; i < Num.Num(); ++i)
	{
		const auto This = Num[i];
		const auto MinV = Num[Min[0]];
		if (This < MinV)
		{
			Min.Empty();
			Min.Add(i);
		}
		else if (This == MinV)
		{
			Min.Add(i);
		}
	}
	return Min[FMath::RandHelper(Min.Num())] + 1;
}

const FScoreData& ASaucewichGameState::GetScoreData(const FName ForWhat) const
{
	static const FScoreData Default{};
	const auto Found = ScoreData.Find(ForWhat);
	return Found ? *Found : Default;
}


TArray<TSubclassOf<AWeapon>> ASaucewichGameState::GetAvailableWeapons(const uint8 Slot) const
{
	TArray<TSubclassOf<AWeapon>> SlotWep;
	for (const auto Class : AvailableWeapons)
	{
		if (const auto Def = Class.GetDefaultObject())
		{
			const auto WepDat = Def->GetData(TEXT("ASaucewichGameState::GetAvailableWeapons()"));
			if (WepDat && WepDat->Slot == Slot)
			{
				SlotWep.Add(Class);
			}
		}
	}
	return SlotWep;
}

float ASaucewichGameState::GetRemainingRoundSeconds() const
{
	return GetWorldTimerManager().GetTimerRemaining(RoundTimer);
}

void ASaucewichGameState::BeginPlay()
{
	Super::BeginPlay();
	
	if (const auto GI = GetGameInstance<USaucewichGameInstance>())
		USaucewichGameInstance::BroadcastGameStateSpawned(GI, this);
}

void ASaucewichGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
	
	if (const auto GameMode = GetWorld()->GetAuthGameMode<ASaucewichGameMode>())
	{
		GetWorldTimerManager().SetTimer(
			RoundTimer, GameMode, &ASaucewichGameMode::UpdateMatchState, RoundMinutes * 60, false
		);
		RoundStartTime = GetWorld()->GetTimeSeconds();
	}
}

void ASaucewichGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	if (HasAuthority())
	{
		const auto Won = GetWinningTeam();
		if (Won != 0)
		{
			ForEachPlayer(PlayerArray, Won, [](ASaucewichPlayerState* const Player)
			{
				Player->AddScore("Win");
			});
		}
	}
	
	OnMatchEnd.Broadcast();
}

void ASaucewichGameState::HandleLeavingMap()
{
	Super::HandleLeavingMap();
	OnLeavingMap.Broadcast();
}

void ASaucewichGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASaucewichGameState, RoundStartTime);
	DOREPLIFETIME(ASaucewichGameState, TeamScore);
}

void ASaucewichGameState::OnRep_RoundStartTime()
{
	GetWorldTimerManager().SetTimer(RoundTimer, RoundMinutes * 60 - (GetServerWorldTimeSeconds() - RoundStartTime), false);
}

void ASaucewichGameState::MulticastPlayerDeath_Implementation(
	ASaucewichPlayerState* const Victim, ASaucewichPlayerState* const Attacker, AActor* const Inflictor)
{
	Attacker->OnKill();
	Victim->OnDeath();
	OnPlayerDeath.Broadcast(Victim, Attacker, Inflictor);
}

uint8 ASaucewichGameState::GetNumPlayers(const uint8 Team) const
{
	uint8 Num = 0;
	ForEachPlayer(PlayerArray, Team, [&Num](auto){++Num;});
	return Num;
}

uint8 ASaucewichGameState::GetWinningTeam() const
{
	const auto A = TeamScore[1], B = TeamScore[2];
	return A > B ? 1 : A < B ? 2 : 0;
}
