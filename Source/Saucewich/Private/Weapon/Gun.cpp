// Copyright 2019 Team Sosweet. All Rights Reserved.

#include "Gun.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "UnrealNetwork.h"

#include "Entity/ActorPool.h"
#include "Kismet/GameplayStatics.h"
#include "Online/SaucewichGameState.h"
#include "Player/TpsCharacter.h"
#include "Weapon/GunSharedData.h"
#include "Weapon/WeaponComponent.h"
#include "Weapon/Projectile/GunProjectile.h"

AGun::AGun()
	:FirePSC{CreateDefaultSubobject<UParticleSystemComponent>("FirePSC")}
{
	PrimaryActorTick.bCanEverTick = true;
	
	FirePSC->SetupAttachment(GetMesh(), "Muzzle");
	FirePSC->bAutoActivate = false;
}

void AGun::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFiring)
	{
		const auto CurTime = GetGameTimeSinceCreation();
		const auto Delay = 60.f / GetData<FGunData>(TEXT("AGun::Tick()"))->Rpm;
		for (; FireLag >= Delay; FireLag -= Delay)
		{
			Shoot();
			LastFire = CurTime;
		}
		FireLag += DeltaSeconds;
	}

	if (bFiring && CanFire()) FirePSC->Activate();
	else FirePSC->Deactivate();
	
	Reload(DeltaSeconds);
}

void AGun::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGun, Clip);
	DOREPLIFETIME(AGun, bDried);
	DOREPLIFETIME(AGun, bFiring);
}

void AGun::Shoot()
{
	if (!CanFire()) return;
	if (!GetPool()) return;

	const auto Shared = GetSharedData<UGunSharedData>();
	if (!Shared) return;

	auto& Data = GetGunData();

	const auto MuzzleTransform = GetMesh()->GetSocketTransform("Muzzle");
	const auto MuzzleLocation = MuzzleTransform.GetLocation();

	const auto ProjColProf = GetDefault<AGunProjectile>(Data.ProjectileClass)->GetCollisionProfile();

	FHitResult Hit;
	const auto HitType = GunTraceInternal(Hit, ProjColProf, Data);

	auto& Transform = GetRootComponent()->GetComponentTransform();
	const auto Forward = Transform.GetUnitAxis(EAxis::X);
	const auto Right = Transform.GetUnitAxis(EAxis::Y);

	const auto Dir =
		HitType != EGunTraceHit::None
		? (Hit.ImpactPoint - MuzzleLocation).GetSafeNormal()
		: MuzzleTransform.GetRotation().Vector();

	const auto V = 45 * Data.VerticalSpread;
	const auto H = 45 * Data.HorizontalSpread;
	
	TArray<FVector> RDirs;
	for (auto i = 0; i < Data.NumProjectile; ++i)
	{
		RDirs.Add(Dir.RotateAngleAxis(FireRand.FRandRange(-V, V), Forward).RotateAngleAxis(FireRand.FRandRange(-H, H), Right));
	}

	TSet<uint8> HitPawnIdx;
	if (HitType == EGunTraceHit::Pawn)
	{
		FHitResult PawnHitResult;
		for (auto i = 0; i < Data.NumProjectile; ++i)
		{
			if (GetWorld()->LineTraceSingleByProfile(PawnHitResult, MuzzleLocation, MuzzleLocation + RDirs[i] * Data.MaxDistance, ProjColProf))
			{
				Hit.GetActor()->TakeDamage(
					Data.Damage,
					FPointDamageEvent{ Data.Damage, PawnHitResult, RDirs[i], Data.DamageType },
					GetInstigator()->GetController(),
					this
				);
				HitPawnIdx.Add(i);
			}
		}
	}

	FTransform SpawnTransform{
		FQuat::Identity, MuzzleLocation,
		FVector{FireRand.FRandRange(Data.MinProjectileSize, Data.MaxProjectileSize)}
	};

	FActorSpawnParameters Parameters;
	Parameters.Owner = this;
	Parameters.Instigator = GetInstigator();

	for (auto i = 0; i < Data.NumProjectile; ++i)
	{
		SpawnTransform.SetRotation(RDirs[i].ToOrientationQuat());
		if (const auto Projectile = GetPool()->Spawn<AGunProjectile>(*Data.ProjectileClass, SpawnTransform, Parameters))
		{
			Projectile->bCosmetic = HitPawnIdx.Contains(i);
			Projectile->SetColor(GetColor());
		}
	}

	LastClip = --Clip;
	if (!bDried && Clip == 0 && HasAuthority())
	{
		bDried = true;
		OnRep_Dried();
	}
	ReloadAlpha = 0.f;
	ReloadWaitingTime = 0.f;

	UGameplayStatics::PlaySoundAtLocation(this, Data.FireSound.LoadSynchronous(), MuzzleLocation);

	OnShoot();
}

EGunTraceHit AGun::GunTrace(FHitResult& OutHit) const
{
	auto& Data = GetGunData();
	if (const auto Def = Data.ProjectileClass.GetDefaultObject())
	{
		const auto Profile = Def->GetCollisionProfile();
		return GunTraceInternal(OutHit, Profile, Data);
	}
	return EGunTraceHit::None;
}

EGunTraceHit AGun::GunTraceInternal(FHitResult& OutHit, const FName ProjColProf, const FGunData& Data) const
{
	const auto Shared = GetSharedData<UGunSharedData>();
	if (!Shared) return EGunTraceHit::None;

	const auto Character = Cast<ATpsCharacter>(GetOwner());
	if (!Character->IsValidLowLevel()) return EGunTraceHit::None;

	const auto GS = GetWorld()->GetGameState<ASaucewichGameState>();
	if (!GS) return EGunTraceHit::None;

	const auto AimRotation = Character->GetBaseAimRotation();
	const auto AimDir = AimRotation.Vector();
	const auto Start = Character->GetSpringArmLocation() + AimDir * Shared->TraceStartOffset;
	const auto End = Start + AimDir * Data.MaxDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActors(TArray<AActor*>{GS->GetCharactersByTeam(Character->GetTeam())});

	TArray<FHitResult> BoxHits;
	GetWorld()->SweepMultiByProfile(
		BoxHits, Start, End, AimRotation.Quaternion(), Shared->PawnOnly.Name,
		FCollisionShape::MakeBox({ 0.f, Data.TraceBoxSize.X, Data.TraceBoxSize.Y }), Params
	);

	auto HitPawn = -1;
	for (auto i = 0; i < BoxHits.Num(); ++i)
	{
		const auto Chr = Cast<ATpsCharacter>(BoxHits[i].GetActor());
		if (!Chr || Chr->IsInvincible()) continue;

		if (!GetWorld()->LineTraceTestByProfile(BoxHits[i].ImpactPoint, Start, Shared->NoPawn.Name, Params))
		{
			HitPawn = i;
			break;
		}
	}

	if (HitPawn != -1)
	{
		OutHit = BoxHits[HitPawn];
		return EGunTraceHit::Pawn;
	}

	return GetWorld()->LineTraceSingleByProfile(OutHit, Start, End, ProjColProf, Params) ? EGunTraceHit::Other : EGunTraceHit::None;
}

void AGun::OnRep_Dried() const
{
	if (const auto Char = Cast<ATpsCharacter>(GetOwner()))
		Char->GetWeaponComponent()->OnGunDried.Broadcast(bDried);
}

const FGunData& AGun::GetGunData() const
{
	static const FGunData Default{};
	const auto Data = GetData<FGunData>(TEXT("AGun::GetGunData()"));
	return Data ? *Data : Default;
}

void AGun::BeginPlay()
{
	Super::BeginPlay();

	if (const auto Data = GetData<FGunData>(TEXT("AGun::BeginPlay()")))
	{
		FirePSC->SetFloatParameter("RPM", Data->Rpm);
	}
}

void AGun::FireP()
{
	const auto Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled())
	{
		const auto Seed = FMath::Rand();
		StartFire(Seed);
		ServerStartFire(Seed);
	}
}

void AGun::FireR()
{
	bFiring = false;
}

void AGun::SlotP()
{
	if (const auto Character = Cast<ATpsCharacter>(GetOwner()))
		if (const auto Data = GetData(TEXT("AGun::SlotP()")))
			Character->GetWeaponComponent()->TrySelectWeapon(Data->Slot);
}

void AGun::OnActivated()
{
	Super::OnActivated();

	if (const auto Data = GetData<FGunData>(TEXT("AGun::OnActivated()")))
	{
		Clip = Data->ClipSize;
	}
	else
	{
		SetActorTickEnabled(false);
	}
}

void AGun::OnReleased()
{
	Super::OnReleased();
	bFiring = false;
	FireLag = 0.f;
	LastFire = 0.f;
	bDried = false;
	ReloadWaitingTime = 0.f;
	ReloadAlpha = 0.f;
}

void AGun::SetColor(const FLinearColor& NewColor)
{
	Super::SetColor(NewColor);
	FirePSC->SetColorParameter("Color", NewColor);
}

void AGun::StartFire(const int32 RandSeed)
{
	const auto Data = GetData<FGunData>(TEXT("AGun::StartFire()"));
	if (!Data) return;

	FireRand.Initialize(RandSeed);

	bFiring = true;
	FireLag = 0.f;
	if (LastFire + 60.f / Data->Rpm <= GetGameTimeSinceCreation())
	{
		Shoot();
		LastFire = GetGameTimeSinceCreation();
	}
}

void AGun::MulticastStartFire_Implementation(const int32 RandSeed)
{
	const auto Pawn = Cast<APawn>(GetOwner());
	if (Pawn && !Pawn->IsLocallyControlled()) StartFire(RandSeed);
}

void AGun::ServerStartFire_Implementation(const int32 RandSeed)
{
	MulticastStartFire(RandSeed);
}

bool AGun::ServerStartFire_Validate(int32)
{
	return true;
}

bool AGun::CanFire() const
{
	return IsActive() && Clip > 0 && !bDried;
}

void AGun::Reload(const float DeltaSeconds)
{
	if (!HasAuthority()) return;

	const auto Data = GetData<FGunData>(TEXT("AGun::Reload()"));
	if (!Data) return;

	if (Clip < Data->ClipSize)
	{
		if (ReloadWaitingTime >= (bDried ? Data->ReloadWaitTimeAfterDried : Data->ReloadWaitTime))
		{
			ReloadAlpha = FMath::Clamp(ReloadAlpha + DeltaSeconds / Data->ReloadTime, 0.f, 1.f);
			Clip = FMath::CubicInterp<float>(LastClip, 0.f, Data->ClipSize, 0.f, ReloadAlpha);

			if (bDried && Clip >= Data->MinClipToFireAfterDried)
			{
				bDried = false;
				OnRep_Dried();
			}
		}
		else
		{
			ReloadWaitingTime += DeltaSeconds;
		}
	}
}
