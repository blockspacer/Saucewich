// Copyright 2019 Team Sosweet. All Rights Reserved.

#include "Projectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "ProjectilePoolComponent.h"

AProjectile::AProjectile()
	:Movement{CreateDefaultSubobject<UProjectileMovementComponent>("Movement")}
{
}

void AProjectile::SetSpeed(const float Speed) const
{
	Movement->SetVelocityInLocalSpace(FVector::ForwardVector * Speed);
}

void AProjectile::Release()
{
	SetActivated(false);
	Pool->Release(this);
}

FName AProjectile::GetCollisionProfile() const
{
	return GetStaticMeshComponent()->GetCollisionProfileName();
}

void AProjectile::SetActivated(const bool bActive)
{
	SetActorTickEnabled(bActive);
	SetActorEnableCollision(bActive);
	SetActorHiddenInGame(!bActive);
	if (bActive)
	{
		Movement->SetUpdatedComponent(RootComponent);
	}
}

void AProjectile::NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, const bool bSelfMoved,
	const FVector HitLocation, const FVector HitNormal, const FVector NormalImpulse, const FHitResult& Hit)
{
	Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
	Release();
}
