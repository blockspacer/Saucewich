// Copyright 2019 Team Sosweet. All Rights Reserved.

#pragma once

#include "Saucewich.h"
#include "Weapon/Weapon.h"
#include "Engine/CollisionProfile.h"
#include "Gun.generated.h"

UCLASS(Abstract)
class AGun : public AWeapon
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class UProjectilePoolComponent* ProjectilePool;

public:
	AGun();

	UProjectilePoolComponent* GetProjectilePool() const { return ProjectilePool; }

	float GetDamage() const { return Damage; }
	float GetProjectileSpeed() const { return ProjectileSpeed; }
	TSubclassOf<UDamageType> GetDamageType() const { return DamageType; }

	virtual void Shoot();

	UFUNCTION(BlueprintCallable)
	EGunTraceHit GunTrace(FHitResult& OutHit);

	void FireP() override;
	void FireR() override;
	void SlotP() override;

protected:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanFire() const;
	virtual void Reload(float DeltaSeconds);

private:
	UFUNCTION()
	void OnRep_FireRandSeed();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, meta=(AllowPrivateAccess=true))
	FCollisionProfileName PawnOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, meta=(AllowPrivateAccess=true))
	FCollisionProfileName NoPawn;

	// �ڵ����� ���� ũ��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FVector2D TraceBoxSize;
	FRandomStream FireRand;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, meta=(AllowPrivateAccess=true))
	TSubclassOf<UDamageType> DamageType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, AdvancedDisplay, meta=(AllowPrivateAccess=true))
	float TraceStartOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float Damage;

	// �д� �߻� �� (�����)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float Rpm;
	float FireLag;
	float LastFire;

	// �ҽ� �߻� ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(UIMin=0, UIMax=1, ClampMin=0, ClampMax=1, AllowPrivateAccess=true))
	float VerticalSpread;

	// �ҽ� �߻� ���� ����
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(UIMin=0, UIMax=1, ClampMin=0, ClampMax=1, AllowPrivateAccess=true))
	float HorizontalSpread;

	// �ڵ� ���� �ִ� �Ÿ�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float MaxDistance;

	// �߻�Ǵ� �ҽ� �߻�ü�� �ӷ�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float ProjectileSpeed;

	// �������� �ɸ��� �ð�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float ReloadTime;

	// ������ ���۱��� �ɸ��� �ð�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float ReloadWaitTime;

	// �ҽ��� ������ �Ҹ��ϰ� ������ ���۱��� �ɸ��� �ð�
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	float ReloadWaitTimeAfterDried;
	float ReloadWaitingTime;
	float ReloadAlpha;

	UPROPERTY(ReplicatedUsing=OnRep_FireRandSeed, Transient)
	int32 FireRandSeed;

	// źâ ũ��
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 ClipSize;

	UPROPERTY(Replicated, Transient, EditInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 Clip;
	uint8 LastClip;

	// �ҽ��� ������ �Ҹ��ϰ� �������߿� �ҽ��� �̸�ŭ ���� �ٽ� �߻簡 ���������ϴ�.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 MinClipToFireAfterDried;

	UPROPERTY(Replicated, Transient, EditInstanceOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	uint8 bDried : 1;

	UPROPERTY(Replicated, Transient, EditInstanceOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	uint8 bFiring : 1;
};
