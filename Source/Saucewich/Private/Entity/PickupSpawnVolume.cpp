// Copyright 2019-2020 Seokjin Lee. All Rights Reserved.

#include "Entity/PickupSpawnVolume.h"

#include "Components/BoxComponent.h"
#include "TimerManager.h"

#include "Entity/ActorPool.h"
#include "Entity/Pickup.h"

APickupSpawnVolume::APickupSpawnVolume()
	:Volume{CreateDefaultSubobject<UBoxComponent>("Volume")}
{
	RootComponent = Volume;
	Volume->BodyInstance.SetCollisionProfileNameDeferred("NoCollision");
}

void APickupSpawnVolume::Spawn(const TSubclassOf<class APickup> Class) const
{
	const auto Extent = Volume->GetScaledBoxExtent();
	const FTransform Transform{GetActorLocation() + FMath::RandPointInBox({-Extent, Extent})};
	AActorPool::Get(this)->Spawn(Class, Transform);
}
