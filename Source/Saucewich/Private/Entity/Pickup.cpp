// Copyright 2019 Team Sosweet. All Rights Reserved.

#include "Pickup.h"

#include "Components/SphereComponent.h"
#include "TimerManager.h"

#include "ShadowComponent.h"

APickup::APickup()
	:Collision{CreateDefaultSubobject<USphereComponent>("Collision")},
	Mesh{CreateDefaultSubobject<UStaticMeshComponent>("Mesh")},
	Shadow{CreateDefaultSubobject<UShadowComponent>("Shadow")}
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bReplicateMovement = true;
	
	RootComponent = Collision;
	Collision->BodyInstance.bLockXRotation = true;
	Collision->BodyInstance.bLockYRotation = true;
	Collision->BodyInstance.bLockZRotation = true;
	Collision->BodyInstance.bSimulatePhysics = true;
	Collision->BodyInstance.SetCollisionProfileNameDeferred("Pickup");
	
	Mesh->SetupAttachment(Collision);
	Mesh->BodyInstance.SetCollisionProfileNameDeferred("NoCollision");
	
	Shadow->SetupAttachment(Mesh);
	Shadow->RelativeScale3D = FVector{Collision->GetScaledSphereRadius() / 50};
}

void APickup::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	const auto Time = GetGameTimeSinceCreation();
	Mesh->RelativeLocation.Z = FMath::Sin(Time * BounceSpeed) * BounceScale;
	Mesh->RelativeRotation.Yaw += DeltaSeconds * RotateSpeed;
	Mesh->UpdateComponentToWorld();

	TArray<AActor*> Actors;
	GetOverlappingActors(Actors, GetClass());
	for (const auto Actor : Actors)
	{
		auto Force = Actor->GetActorLocation();
		Force -= GetActorLocation();
		const auto SizeSqr = Force.SizeSquared();
		if (SizeSqr <= SMALL_NUMBER)
		{
			Force = FMath::VRand();
			Force *= PushStrength;
		}
		else
		{
			Force *= FMath::InvSqrt(SizeSqr) * PushStrength;
		}
		static_cast<APickup*>(Actor)->Collision->AddForce(Force, NAME_None, true);
	}
}

void APickup::OnReleased()
{
	Collision->SetSimulatePhysics(false);
}

void APickup::OnActivated()
{
	Collision->SetSimulatePhysics(true);

	// 물리 시뮬레이션을 키면 이번 틱이 끝나고 시작하는데, 액터 위치가 시뮬레이션을 끄기 전의 위치로 돌아간다
	// 때문에 현재 위치를 저장해뒀다가 다음 틱에서 복구시켜야 한다
	const auto Location = GetActorLocation();
	GetWorldTimerManager().SetTimerForNextTick([this, Location]{SetActorLocation(Location);});
}
