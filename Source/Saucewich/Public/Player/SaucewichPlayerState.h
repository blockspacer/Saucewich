// Copyright 2019-2020 Seokjin Lee. All Rights Reserved.

#pragma once

#include "GameFramework/PlayerState.h"
#include "SaucewichPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnScoreAdded, FName, ScoreID, int32, ActualScore, int32, NewScore);
DECLARE_EVENT_ThreeParams(ASaucewichPlayerState, FOnScoreAddedNative, FName, int32, int32)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, uint8, NewTeam);
DECLARE_EVENT_OneParam(ASaucewichPlayerState, FOnTeamChangedNative, uint8)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNameChanged, const FString&, NewName);

UCLASS(Config=UserSettings)
class SAUCEWICH_API ASaucewichPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	void SetTeam(uint8 NewTeam);
	uint8 GetTeam() const { return Team; }
	bool IsValidTeam() const { return Team != static_cast<uint8>(-1); }

	UFUNCTION(BlueprintCallable)
	void SetWeapon(uint8 Slot, const TSoftClassPtr<class AWeapon>& Weapon);

	UFUNCTION(BlueprintCallable)
	void SaveWeaponLoadout();

	void GiveWeapons();

	virtual void OnKill();
	virtual void OnDeath();
	virtual void OnCharDestroyed() {}

	/**
	 * 점수를 추가(혹은 차감)합니다. Authority 전용입니다.
	 * @param ScoreID		점수 ID
	 * @param ActualScore	0이 아닌 값이면 미리 지정된 점수 말고 이 값만큼 추가
	 * @param bForce		강제 여부
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddScore(FName ScoreID, int32 ActualScore = 0, bool bForce = false);

	uint8 GetKill() const { return Kill; }
	uint8 GetDeath() const { return Death; }
	uint8 GetObjective() const { return Objective; }
	void SetObjective(uint8 NewObjective);

	UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation)
	void RequestSetPlayerName(const FString& NewPlayerName);

	void BindOnTeamChanged(FOnTeamChangedNative::FDelegate&& Callback);

	UPROPERTY(BlueprintAssignable)
	FOnTeamChanged OnTeamChangedDelegate;

	/**
	 * 점수가 추가될때 호출됩니다.
	 * @warning 클라이언트에서는 이 델리게이트가 호출된 시점에서 점수 추가가 실제로 반영되었다는 보장이 없습니다.
	 * @param ScoreName 이 점수의 이름입니다. GameState에서 점수 정보를 얻을 때 사용하는 식별자입니다.
	 * @param ActualScore 실제로 주어진 점수입니다.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnScoreAdded OnScoreAdded;
	FOnScoreAddedNative OnScoreAddedNative;

protected:
	void BeginPlay() override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetPlayerName(const FString& S) override;
	void OnRep_PlayerName() override;

	UFUNCTION()
	void OnTeamChanged(uint8 OldTeam);

private:
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetWeapon(uint8 Slot, const TSoftClassPtr<class AWeapon>& Weapon);
	void SetWeapon_Internal(uint8 Slot, const TSoftClassPtr<class AWeapon>& Weapon);

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerSetWeaponLoadout(const TArray<TSoftClassPtr<AWeapon>>& Loadout);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastAddScore(FName ScoreID, int32 ActualScore, int32 NewScore);

	void ValidateLoadout();

	FOnTeamChangedNative OnTeamChangedNative;

	UPROPERTY(BlueprintAssignable)
	FOnNameChanged OnNameChanged;

	UPROPERTY(Transient, Config, VisibleInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<TSoftClassPtr<AWeapon>> WeaponLoadout;

	UPROPERTY(EditDefaultsOnly)
	TArray<TSoftClassPtr<AWeapon>> DefaultWeaponLoadout;

	UPROPERTY(ReplicatedUsing=OnTeamChanged, Transient, VisibleInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 Team = -1;
	
	UPROPERTY(Replicated, Transient, VisibleInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 Objective;

	UPROPERTY(Replicated, Transient, VisibleInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 Kill;
	
	UPROPERTY(Replicated, Transient, VisibleInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 Death;
};
