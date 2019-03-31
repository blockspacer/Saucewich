// Copyright (c) 2019, 이석진, 강찬. All rights reserved.

#include "Weapon.h"
#include "Components/StaticMeshComponent.h"

AWeapon::AWeapon()
	:SceneRoot{ CreateDefaultSubobject<USceneComponent>("SceneRoot") },
	Mesh{ CreateDefaultSubobject<UStaticMeshComponent>("Mesh") }
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = SceneRoot;
	Mesh->SetupAttachment(SceneRoot);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

