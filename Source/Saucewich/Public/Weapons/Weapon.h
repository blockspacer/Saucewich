// Copyright (c) 2019, Team Sosweet. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

UCLASS(Abstract)
class SAUCEWICH_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:
	AWeapon();

private:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void Attack();
	void StopAttack();
	bool CanAttack() const;

private:
	UPROPERTY()
	class USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	class UArrowComponent* Muzzle;

	UPROPERTY(VisibleAnywhere)
	class UActorPoolComponent* ProjectilePool;

	UPROPERTY(EditDefaultsOnly)
	FText Name;

	UPROPERTY(EditDefaultsOnly)
	int32 Capacity;

	UPROPERTY(EditDefaultsOnly)
	uint8 bFullAuto : 1;

	UPROPERTY(EditAnywhere, meta = (EditCondition = bFullAuto))
	float AttackDelay = .1f;

	UPROPERTY(EditDefaultsOnly)
	int32 NumberOfProjectilesFiredAtOnce = 1;

	//////////////////////////////////////////////////////////////////////////

	UPROPERTY(Replicated, Transient)
	int32 RemainingSauceAmount;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Attacking, Transient, meta = (AllowPrivateAccess = true))
	uint8 bAttacking : 1;
	uint8 bOldAttacking : 1;

	UFUNCTION()
	void OnRep_Attacking();

	float AttackLag;

	void HandleAttackDelay(float DeltaTime);
	void HandleAttack();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAttack();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSingleAttack();

	//////////////////////////////////////////////////////////////////////////
	
	UPROPERTY(EditAnywhere)
	float AimTraceLength = 3000.f;

	void OrientRotationToAimingLocation();
};
