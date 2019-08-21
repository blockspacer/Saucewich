// Copyright 2019 Team Sosweet. All Rights Reserved.

#pragma once

#include "GameFramework/GameState.h"
#include "SaucewichGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerChangedWeapon, class ASaucewichPlayerState*, Player, uint8, Slot, TSubclassOf<class AWeapon>, NewWeapon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerChangedTeam, ASaucewichPlayerState*, Player, uint8, OldTeam, uint8, NewTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerDeath, ASaucewichPlayerState*, Victim, ASaucewichPlayerState*, Attacker, AActor*, Inflictor);

USTRUCT(BlueprintType)
struct FTeam
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FLinearColor Color;
};

UCLASS()
class SAUCEWICH_API ASaucewichGameState : public AGameState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	const FTeam& GetTeamData(const uint8 Team) const { return Teams[Team]; }

	UFUNCTION(BlueprintCallable)
	TArray<class ASaucewichPlayerState*> GetPlayers(uint8 Team) const;

	UFUNCTION(BlueprintCallable)
	TArray<class ATpsCharacter*> GetCharacters(uint8 Team) const;

	UFUNCTION(BlueprintCallable)
	bool IsValidTeam(const uint8 Team) const { return Team > 0 && Team < Teams.Num(); }

	UFUNCTION(BlueprintCallable)
	uint8 GetPlayerNum(uint8 Team) const;
	   
	// 플레이어 수가 가장 적은 팀을 반환합니다. 여러 개일 경우 무작위로 반환됩니다.
	UFUNCTION(BlueprintCallable)
	uint8 GetMinPlayerTeam() const;

	// 무기 목록에서 특정 슬롯의 무기들만 반환합니다.
	// 시간복잡도가 O(n)이고 새 배열을 할당하므로 자주 호출하지는 마세요.
	UFUNCTION(BlueprintCallable)
	TArray<TSubclassOf<AWeapon>> GetWeapons(uint8 Slot) const;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayerDeath(ASaucewichPlayerState* Victim, ASaucewichPlayerState* Attacker, AActor* Inflictor);

	UPROPERTY(BlueprintAssignable)
	FOnPlayerChangedWeapon OnPlayerChangedWeapon;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerChangedTeam OnPlayerChangedTeam;

	/**
	 * 캐릭터가 죽을 때 호출됩니다.
	 * @param Victim		죽은 캐릭터
	 * @param Attacker		Victim을 죽인 컨트롤러
	 * @param Inflictor		Victim을 죽이는 데 사용된 액터. 대부분의 경우 무기입니다. 해당 액터의 bReplicates가 false면 Client에서는 null입니다.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnPlayerDeath OnPlayerDeath;

private:
	// 팀 정보를 저장하는 배열입니다. 게임 플레이 도중 바뀌지 않습니다.
	// 0번 요소는 unassigned/connecting 팀으로, 사용되지 않는 팀이어야 합니다.
	// 팀 개수는 사용되지 않는 0번 팀 포함 최소 2개여야 합니다.
	// 실제 팀 index는 1부터 시작합니다.
	UPROPERTY(EditDefaultsOnly)
	TArray<FTeam> Teams;

	// 게임에서 사용할 무기 목록입니다.
	// 플레이어는 무기 선택창에서 이 무기들중 하나를 선택하여 사용할 수 있습니다.
	// 특정 슬롯의 무기만을 구하고 싶으면 GetWeapons 함수를 사용하세요.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<TSubclassOf<AWeapon>> Weapons;
};
