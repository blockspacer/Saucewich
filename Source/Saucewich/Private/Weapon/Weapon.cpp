// Copyright 2019 Othereum. All Rights Reserved.

#include "Weapon/Weapon.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

#include "Player/TpsCharacter.h"
#include "Weapon/WeaponComponent.h"
#include "Names.h"

AWeapon* AWeapon::GetDefaultWeapon(const TSubclassOf<AWeapon> Class)
{
	return Class ? Class.GetDefaultObject() : nullptr;
}

AWeapon::AWeapon()
	:SceneRoot{CreateDefaultSubobject<USceneComponent>(NAME("SceneRoot"))},
	Mesh{ CreateDefaultSubobject<UStaticMeshComponent>(Names::Mesh) }
{
	bReplicates = true;
	
	RootComponent = SceneRoot;
	Mesh->SetupAttachment(SceneRoot);
}

void AWeapon::Init()
{
	if (const auto MyOwner = GetOwner())
	{
		if (const auto Character = Cast<ATpsCharacter>(MyOwner))
		{
			SetRole(MyOwner->GetLocalRole());
			const auto WeaponComponent = Character->GetWeaponComponent();
			UWeaponComponent::FBroadcastEquipWeapon(WeaponComponent, this);
			WeaponComponent->GetActiveWeapon() == this ? Deploy() : Holster();
			SetColor(Character->GetColor());
		}
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &AWeapon::Init);
	}
}

UMaterialInstanceDynamic* AWeapon::GetMaterial() const
{
	return CastChecked<UMaterialInstanceDynamic>(Mesh->GetMaterial(GetColIdx()));
}

void AWeapon::OnRep_Equipped()
{
	if (bEquipped) Deploy();
	else Holster();
}

int32 AWeapon::GetColIdx() const
{
	return Mesh->GetMaterialIndex(Names::TeamColor);
}

void AWeapon::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	const auto ColMatIdx = GetColIdx();
	if (ColMatIdx != INDEX_NONE)
	{
		GetMesh()->CreateDynamicMaterialInstance(ColMatIdx);
	}
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWeapon, bEquipped);
}

void AWeapon::OnActivated()
{
	Init();
}

void AWeapon::OnReleased()
{
	Holster();
}

const FWeaponData& AWeapon::GetWeaponData() const
{
	return GetData<FWeaponData>();
}

bool AWeapon::IsVisible() const
{
	return Mesh->IsVisible();
}

void AWeapon::SetVisibility(const bool bNewVisibility) const
{
	Mesh->SetVisibility(bNewVisibility);
}

FLinearColor AWeapon::GetColor() const
{
	FLinearColor Color;
	GetMaterial()->GetVectorParameterValue(Names::Color, Color);
	return Color;
}

void AWeapon::SetColor(const FLinearColor& NewColor)
{
	GetMaterial()->SetVectorParameterValue(Names::Color, NewColor);
}

void AWeapon::Deploy()
{
	Mesh->SetVisibility(true);
	if (HasAuthority())
	{
		bEquipped = true;
	}
}

void AWeapon::Holster()
{
	Mesh->SetVisibility(false);
	if (HasAuthority())
	{
		bEquipped = false;
	}
}
