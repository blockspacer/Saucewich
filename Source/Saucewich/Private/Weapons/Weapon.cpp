// Copyright (c) 2019, Team Sosweet. All rights reserved.

#include "Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h"
#include "UnrealNetwork.h"
#include "SauceProjectile.h"
#include "ActorPoolComponent.h"

AWeapon::AWeapon()
	:SceneRoot{ CreateDefaultSubobject<USceneComponent>("SceneRoot") },
	Mesh{ CreateDefaultSubobject<UStaticMeshComponent>("Mesh") },
	Muzzle{ CreateDefaultSubobject<UArrowComponent>("Muzzle") },
	ProjectilePool{ CreateDefaultSubobject<UActorPoolComponent>("ProjectilePool") }
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = SceneRoot;
	Mesh->SetupAttachment(SceneRoot);
	Muzzle->SetupAttachment(Mesh);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	Role = GetOwner()->Role;
	CopyRemoteRoleFrom(GetOwner());

	RemainingSauceAmount = Capacity;
}

void AWeapon::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	HandleAttackDelay(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, RemainingSauceAmount);
	DOREPLIFETIME(AWeapon, bAttacking);
}

#define ENSURE_NOT_SIMULATED_PROXY() ensureMsgf(Role != ROLE_SimulatedProxy, TEXT(__FUNCTION__)TEXT(" called on simulated proxy. It won't have any effect!"))

void AWeapon::Attack()
{
	if (ENSURE_NOT_SIMULATED_PROXY() && CanAttack() && !bAttacking)
	{
		if (bFullAuto)
		{
			bAttacking = true;
			bOldAttacking = true;
			AttackLag = 0.f;
		}
		HandleAttack();

		if (Role != ROLE_Authority)
		{
			ServerAttack();
		}
	}
}

void AWeapon::StopAttack()
{
	if (ENSURE_NOT_SIMULATED_PROXY() && bAttacking)
	{
		bAttacking = false;
		bOldAttacking = false;

		if (Role != ROLE_Authority)
		{
			ServerStopAttack();
		}
	}
}

bool AWeapon::CanAttack() const
{
	return RemainingSauceAmount > 0.f;
}

void AWeapon::OnRep_Attacking()
{
	if (bAttacking && !bOldAttacking)
	{
		AttackLag = 0.f;
		HandleAttack();
	}
	bOldAttacking = bAttacking;
}

void AWeapon::HandleAttackDelay(const float DeltaTime)
{
	if (bAttacking)
	{
		if (AttackLag >= AttackDelay)
		{
			HandleAttack();
			AttackLag -= AttackDelay;
		}
		else
		{
			AttackLag += DeltaTime;
		}
	}
}

void AWeapon::HandleAttack()
{
	FActorSpawnParameters Param;
	Param.Instigator = GetInstigator();
	Param.Owner = this;
	ProjectilePool->GetActor<ASauceProjectile>(Projectile, Muzzle->GetComponentTransform(), Param);
	PostAttack();
}

void AWeapon::ServerAttack_Implementation() { Attack(); if (!bFullAuto) MulticastSingleAttack(); }
bool AWeapon::ServerAttack_Validate() { return true; }
void AWeapon::ServerStopAttack_Implementation() { StopAttack(); }
bool AWeapon::ServerStopAttack_Validate() { return true; }
void AWeapon::MulticastSingleAttack_Implementation() { if (Role == ROLE_SimulatedProxy) HandleAttack(); }
