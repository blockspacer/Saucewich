// Copyright 2019 Team Sosweet. All Rights Reserved.

#include "SaucewichLibrary.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "SkeletalMeshMerge.h"

#include "Entity/ActorPool.h"
#include "Entity/DecalPoolActor.h"
#include "SaucewichGameInstance.h"

USkeletalMesh* USaucewichLibrary::MergeMeshes(const TArray<USkeletalMesh*>& Meshes)
{
	const auto BaseMesh = NewObject<USkeletalMesh>();
	FSkeletalMeshMerge Merger{BaseMesh, Meshes, {}, 0};
	if (Merger.DoMerge()) return BaseMesh;
	return nullptr;
}

ADecalPoolActor* USaucewichLibrary::SpawnSauceDecal(const FHitResult& HitInfo, UMaterialInterface* const Material, const FLinearColor& Color,
	const FVector SizeMin, const FVector SizeMax, const float LifeSpan)
{
	const auto Comp = HitInfo.GetComponent();
	if (!Comp || Comp->Mobility != EComponentMobility::Static) return nullptr;
	
	const auto World = Comp->GetWorld();
	if (!World) return nullptr;

	const auto Pool = GetActorPool(World);
	if (!Pool) return nullptr;

	auto Rot = HitInfo.ImpactNormal.Rotation();
	Rot.Roll = FMath::FRandRange(0, 360);
	
	const auto Decal = Pool->Spawn<ADecalPoolActor>({Rot, HitInfo.ImpactPoint});
	if (Decal)
	{
		Decal->SetDecalMaterial(Material);
		Decal->SetColor(Color);
		Decal->SetDecalSize({
			FMath::RandRange(SizeMin.X, SizeMax.X),
			FMath::RandRange(SizeMin.Y, SizeMax.Y),
			FMath::RandRange(SizeMin.Z, SizeMax.Z)
		});
		Decal->SetLifeSpan(LifeSpan);
	}

	return Decal;
}

AActorPool* USaucewichLibrary::GetActorPool(const UObject* const WorldContextObject)
{
	if (WorldContextObject)
		if (const auto World = WorldContextObject->GetWorld())
			if (const auto GI = World->GetGameInstance<USaucewichGameInstance>())
				return GI->GetActorPool();
	return nullptr;
}
