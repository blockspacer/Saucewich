// Copyright (c) 2019, Team Sosweet. All rights reserved.

#include "Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "UnrealNetwork.h"
#include "SauceProjectile.h"
#include "ActorPoolComponent.h"
#include "SaucewichCharacter.h"

AWeapon::AWeapon()
	:Mesh{ CreateDefaultSubobject<UStaticMeshComponent>("Mesh") },
	Muzzle{ CreateDefaultSubobject<USceneComponent>("Muzzle") },
	ProjectilePool{ CreateDefaultSubobject<UActorPoolComponent>("ProjectilePool") }
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	RootComponent = Mesh;
	Muzzle->SetupAttachment(Mesh);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	ASaucewichCharacter* const Owner = Cast<ASaucewichCharacter>(GetOwner());
	if (Owner)
	{
		Role = Owner->Role;
		CopyRemoteRoleFrom(Owner);
		Owner->OnDeath.AddUObject(this, &AWeapon::SetActivated, false);
	}
}

void AWeapon::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	OrientRotationToAimingLocation();
	HandleAttackDelay(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, SauceAmount);
	DOREPLIFETIME(AWeapon, bAttacking);
}

//////////////////////////////////////////////////////////////////////////////

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
	ASaucewichCharacter* const Owner = GetInstigator<ASaucewichCharacter>();
	return Owner && Owner->CanAttack() && SauceAmount > 0;
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
	if (bAttacking && CanAttack())
	{
		if (AttackDelay > 0.f)
		{
			if (AttackLag >= AttackDelay)
			{
				HandleAttack();
				AttackLag = FMath::Fmod(AttackLag, AttackDelay);
			}
			else
			{
				AttackLag += DeltaTime;
			}
		}
		else
		{
			HandleAttack();
		}
	}
}

void AWeapon::HandleAttack()
{
	AActor* const Sauce = ShootSauce();
	if (Sauce)
	{
		--SauceAmount;
	}
}

AActor* AWeapon::ShootSauce()
{
	const FVector Location = Muzzle->GetComponentLocation();
	const FRotator Rotation = Muzzle->GetComponentRotation();
	FActorSpawnParameters Param;
	Param.Instigator = GetInstigator();
	Param.Owner = this;
	ASauceProjectile* const Sauce = ProjectilePool->SpawnActor<ASauceProjectile>(Location, Rotation, Param);
	if (Sauce)
	{
		Sauce->Init(Damage);
	}
	return Sauce;
}

void AWeapon::ServerAttack_Implementation() { Attack(); if (!bFullAuto) MulticastSingleAttack(); }
bool AWeapon::ServerAttack_Validate() { return true; }
void AWeapon::ServerStopAttack_Implementation() { StopAttack(); }
bool AWeapon::ServerStopAttack_Validate() { return true; }
void AWeapon::MulticastSingleAttack_Implementation() { if (Role == ROLE_SimulatedProxy) HandleAttack(); }

//////////////////////////////////////////////////////////////////////////////

void AWeapon::SetActivated(bool bActive)
{
	SetActorTickEnabled(bActive);
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
}

//////////////////////////////////////////////////////////////////////////////

void AWeapon::OrientRotationToAimingLocation()
{
	const FVector Start = GetInstigator()->GetPawnViewLocation();
	const FVector End = Start + GetInstigator()->GetBaseAimRotation().Vector() * AimTraceLength;
	FHitResult Hit;
	bOriented = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility);
	if (bOriented)
	{
		SetActorRotation((Hit.Location - GetActorLocation()).Rotation() + MeshRotationOffset);
	}
	else
	{
		SetActorRelativeRotation(RotationOffsetWhileNotOriented);
	}
}
