// Copyright 2019-2020 Seokjin Lee. All Rights Reserved.

#pragma once

#include "Player/BasePC.h"
#include "Saucewich.h"
#include "SaucewichPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateSpawned, class ASaucewichPlayerState*, PlayerState);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPSSpawnedNative, ASaucewichPlayerState*)
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPlayerStateSpawnedSingle, ASaucewichPlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterSpawned, class ATpsCharacter*, Character);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCharacterSpawnedSingle, ATpsCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharRespawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharDied);
DECLARE_EVENT(ASaucewichPlayerController, FOnPlyRespawnNative)
DECLARE_EVENT(ASaucewichPlayerController, FOnPlyDeathNative)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnReceiveMessage, const FText&, Message, float, Duration, EMsgType, Type);

UCLASS()
class SAUCEWICH_API ASaucewichPlayerController : public ABasePC
{
	GENERATED_BODY()

public:
	UFUNCTION(NetMulticast, Reliable)
	void SetRespawnTimer(float RespawnTime);

	UFUNCTION(BlueprintCallable)
	float GetRemainingRespawnTime() const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	void Respawn();

	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerRespawn();

	UFUNCTION(BlueprintCallable)
	void SafePlayerState(const FOnPlayerStateSpawnedSingle& Delegate);
	void SafePS(FOnPSSpawnedNative::FDelegate&& Delegate);
	
	UFUNCTION(BlueprintCallable)
	void SafeCharacter(const FOnCharacterSpawnedSingle& Delegate);

	UFUNCTION(BlueprintCallable, Client, Reliable)
	void PrintMessage(const FText& Message, float Duration, EMsgType Type);
	void PrintMessageLocal(const FText& Message, float Duration, EMsgType Type) const;

	void SetSessionID(FString&& ID);
	const FString& GetSessionID() const;
	void SetPlayerID(FString&& ID);
	const FString& GetPlayerID() const;

	void BroadcastRespawn() const;
	void BroadcastDeath() const;

	float GetLatencyInMs() const { return LatencyInMs; }

	struct BroadcastPlayerStateSpawned;
	struct BroadcastCharacterSpawned;

	UPROPERTY(BlueprintAssignable)
	FOnCharRespawn OnCharRespawn;
	FOnPlyRespawnNative OnPlyRespawnNative;

	UPROPERTY(BlueprintAssignable)
	FOnCharDied OnCharDied;
	FOnPlyDeathNative OnPlyDeathNative;

	UPROPERTY(BlueprintAssignable)
	FOnReceiveMessage OnReceiveMessage;

protected:
	void BeginPlay() override;
	void InitPlayerState() override;
	
private:
	bool CanRespawn() const;

	void Ping();
	void OnPingFailed() const;
	void DisconnectWithError(const FText& Msg) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPing();

	UFUNCTION(Client, Reliable)
	void ClientPing();

	void MeasureLatency();

	UFUNCTION(Server, Reliable, WithValidation)
	void InitialMeasureLatency();
	
	UFUNCTION(Client, Reliable)
	void BeginMeasureLatency();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ReplyMeasureLatency();
	
	FOnPlayerStateSpawned OnPlayerStateSpawned;
	FOnPSSpawnedNative OnPSSpawnedNative;
	FOnCharacterSpawned OnCharacterSpawned;

	FString SessionID;
	FString PlayerID;

	FTimerHandle RespawnTimer;
	FTimerHandle PingTimer;
	FTimerHandle LatencyMeasureTimer;

	UPROPERTY(EditDefaultsOnly)
	float PingTimeout = 1;

	float LatencyMeasureBeginTime;
	float LatencyInMs = 1.f;
};

struct ASaucewichPlayerController::BroadcastPlayerStateSpawned
{
private:
	friend ASaucewichPlayerState;
	BroadcastPlayerStateSpawned(ASaucewichPlayerController* Controller, ASaucewichPlayerState* PlayerState)
	{
		Controller->OnPlayerStateSpawned.Broadcast(PlayerState);
		Controller->OnPlayerStateSpawned.Clear();
		Controller->OnPSSpawnedNative.Broadcast(PlayerState);
		Controller->OnPSSpawnedNative.Clear();
	}
};

struct ASaucewichPlayerController::BroadcastCharacterSpawned
{
private:
	friend ATpsCharacter;
	BroadcastCharacterSpawned(ASaucewichPlayerController* Controller, ATpsCharacter* Character)
	{
		Controller->OnCharacterSpawned.Broadcast(Character);
		Controller->OnCharacterSpawned.Clear();
	}
};
