// Copyright (c) 2019, Team Sosweet. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Weapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponGripType : uint8
{
	Rifle, Pistol
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	UPROPERTY(EditAnywhere)
	float Damage;

	UPROPERTY(EditAnywhere)
	int32 NumberOfProjectilesFiredAtOnce;

	UPROPERTY(EditAnywhere)
	uint8 bFullAuto : 1;

	UPROPERTY(EditAnywhere)
	float AttackDelay;

	UPROPERTY(EditAnywhere)
	int32 SauceAmount;

	UPROPERTY(EditAnywhere)
	int32 MinSauceAmountToShootWhenFullReload;

	UPROPERTY(EditAnywhere)
	float ReloadTime;

	UPROPERTY(EditAnywhere)
	float ReloadWaitTime;

	UPROPERTY(EditAnywhere)
	float ProjectileSpeed;

	UPROPERTY(EditAnywhere)
	float Weight;

	UPROPERTY(EditAnywhere)
	EWeaponGripType GripType;
};

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

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere)
	class USceneComponent* Muzzle;

	UPROPERTY(VisibleAnywhere)
	class UActorPoolComponent* ProjectilePool;

	UPROPERTY(EditAnywhere)
	FDataTableRowHandle DataTableHandle;

	mutable const FWeaponData* DataTable;

public:
	const FWeaponData& GetData() const;

	void Attack();
	void StopAttack();
	bool CanAttack() const;

private:
	void SetActivated(bool bActive);

	UPROPERTY(EditInstanceOnly, Replicated, Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	int32 SauceAmount;
	int32 LastSauceAmount;
	float ReloadAlpha;
	float ReloadWaitTime;
	uint8 bDried : 1;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, ReplicatedUsing = OnRep_Attacking, Transient, meta = (AllowPrivateAccess = true))
	uint8 bAttacking : 1;
	uint8 bOldAttacking : 1;

	UFUNCTION()
	void OnRep_Attacking();

	float NextAttackTime;

	void HandleAttack();
	AActor* ShootSauce();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerAttack();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerStopAttack();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSingleAttack();

	void Reload(float DeltaTime);
};
