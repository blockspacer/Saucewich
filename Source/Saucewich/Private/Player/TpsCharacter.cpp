// Copyright 2019 Team Sosweet. All Rights Reserved.

#include "TpsCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"
#include "UnrealNetwork.h"

#include "SaucewichGameMode.h"
#include "SaucewichGameState.h"
#include "SaucewichPlayerController.h"
#include "SaucewichPlayerState.h"
#include "TpsCharacterMovementComponent.h"
#include "WeaponComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTpsCharacter, Log, All)

ATpsCharacter::ATpsCharacter(const FObjectInitializer& ObjectInitializer)
	:Super{ObjectInitializer.SetDefaultSubobjectClass<UTpsCharacterMovementComponent>(CharacterMovementComponentName)},
	WeaponComponent{ CreateDefaultSubobject<UWeaponComponent>("WeaponComponent") },
	SpringArm{ CreateDefaultSubobject<USpringArmComponent>("SpringArm") },
	Camera{ CreateDefaultSubobject<UCameraComponent>("Camera") },
	Shadow{ CreateDefaultSubobject<UStaticMeshComponent>("Shadow") }
{
	WeaponComponent->SetupAttachment(GetMesh(), "Weapon");
	SpringArm->SetupAttachment(RootComponent);
	Camera->SetupAttachment(SpringArm);
	Shadow->SetupAttachment(RootComponent);
}

AWeapon* ATpsCharacter::GetActiveWeapon() const
{
	return WeaponComponent->GetActiveWeapon();
}

EGunTraceHit ATpsCharacter::GunTrace(FHitResult& OutHit) const
{
	return WeaponComponent->GunTrace(OutHit);
}

uint8 ATpsCharacter::GetTeam() const
{
	return State ? State->GetTeam() : 0;
}

FLinearColor ATpsCharacter::GetColor() const
{
	FLinearColor Color;
	if (const auto Mat = Cast<UMaterialInstanceDynamic>(GetMesh()->GetMaterial(GetMesh()->GetMaterialIndex("TeamColor"))))
		Mat->GetVectorParameterValue({"Color"}, Color);
	return Color;
}

FLinearColor ATpsCharacter::GetTeamColor() const
{
	if (const auto GameState = GetWorld()->GetGameState<ASaucewichGameState>())
	{
		return GetTeamColor(GameState);
	}
	return {};
}

void ATpsCharacter::SetColor(const FLinearColor& NewColor)
{
	if (const auto Mat = Cast<UMaterialInstanceDynamic>(GetMesh()->GetMaterial(GetMesh()->GetMaterialIndex("TeamColor"))))
		Mat->SetVectorParameterValue("Color", NewColor);
	WeaponComponent->SetColor(NewColor);
}

bool ATpsCharacter::IsInvincible() const
{
	return GetWorldTimerManager().GetTimerRemaining(RespawnInvincibleTimerHandle) > 0.f;
}

void ATpsCharacter::SetMaxHP(const float Ratio)
{
	const auto OldMaxHP = MaxHP;
	MaxHP = DefaultMaxHP * Ratio;
	if (HP == OldMaxHP) HP = MaxHP;
}

float ATpsCharacter::GetSpeedRatio() const
{
	return WeaponComponent->GetSpeedRatio();
}

FVector ATpsCharacter::GetPawnViewLocation() const
{
	if (Role == ROLE_SimulatedProxy)
	{
		const auto ArmLocation = GetSpringArmLocation();
		return ArmLocation - GetBaseAimRotation().Vector() * (ArmLocation - Camera->GetComponentLocation()).Size();
	}
	return Camera->GetComponentLocation();
}

FVector ATpsCharacter::GetSpringArmLocation() const
{
	return SpringArm->GetComponentLocation();
}

void ATpsCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	const auto Idx = FMath::Max(GetMesh()->GetMaterialIndex("TeamColor"), 0);
	DynamicMaterial = GetMesh()->CreateDynamicMaterialInstance(Idx);

	RegisterGameMode();
}

void ATpsCharacter::BeginPlay()
{
	Super::BeginPlay();

	ShadowData.Material = Shadow->CreateDynamicMaterialInstance(0);
	BindOnTeamChanged();

	if (HasAuthority())
	{
		bAlive = true;
		HP = MaxHP = DefaultMaxHP;

		if (State)
		{
			if (const auto GS = GetWorld()->GetGameState<ASaucewichGameState>())
			{
				if (!GS->IsValidTeam(GetTeam()))
				{
					State->SetTeam(GS->GetMinPlayerTeam());
				}
			}
		}
	}
}

void ATpsCharacter::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateShadow();
}

void ATpsCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	Input->BindAxis("MoveForward", this, &ATpsCharacter::MoveForward);
	Input->BindAxis("MoveRight", this, &ATpsCharacter::MoveRight);
	Input->BindAxis("Turn", this, &ATpsCharacter::AddControllerYawInput);
	Input->BindAxis("LookUp", this, &ATpsCharacter::AddControllerPitchInput);
	Input->BindAction("Respawn", IE_Pressed, this, &ATpsCharacter::Respawn);

	WeaponComponent->SetupPlayerInputComponent(Input);
}

void ATpsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATpsCharacter, HP);
	DOREPLIFETIME(ATpsCharacter, MaxHP);
	DOREPLIFETIME(ATpsCharacter, bAlive);
}

float ATpsCharacter::TakeDamage(const float DamageAmount, const FDamageEvent& DamageEvent, AController* const EventInstigator, AActor* const DamageCauser)
{
	const auto Damage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Damage != 0.f)
	{
		if (Damage > 0.f)
		{
			if (const auto Attacker = Cast<ASaucewichPlayerController>(EventInstigator))
			{
				Attacker->OnHitEnemy.Broadcast();
			}
		}

		HP = FMath::Clamp(HP - Damage, 0.f, MaxHP);
		if (HP == 0.f) Kill(EventInstigator->GetPlayerState<ASaucewichPlayerState>(), DamageCauser);
	}
	return Damage;
}

bool ATpsCharacter::ShouldTakeDamage(const float DamageAmount, const FDamageEvent& DamageEvent, AController* const EventInstigator, AActor* const DamageCauser) const
{
	if (!Super::ShouldTakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser))
		return false;

	if (!IsAlive())
		return false;

	if (IsInvincible())
		return false;

	if (!EventInstigator)
		return true;

	if (const auto InstigatorState = EventInstigator->GetPlayerState<ASaucewichPlayerState>())
		return DamageAmount > 0.f ? GetTeam() != InstigatorState->GetTeam() : GetTeam() == InstigatorState->GetTeam();

	return true;
}

void ATpsCharacter::SetPlayerDefaults()
{
	HP = MaxHP;
	bAlive = true;
	SetActorActivated(true);

	if (RespawnInvincibleTime > 0.f)
	{
		BeTransl();
		GetWorldTimerManager().SetTimer(
			RespawnInvincibleTimerHandle, 
			this, &ATpsCharacter::BeOpaque,
			RespawnInvincibleTime, false
		);
	}
	
	OnCharacterSpawn.Broadcast();
}

void ATpsCharacter::Kill(ASaucewichPlayerState* const Attacker, AActor* const Inflictor)
{
	HP = 0.f;
	bAlive = false;
	SetActorActivated(false);

	if (GameMode)
		if (const auto PC = GetController<ASaucewichPlayerController>())
			GameMode->SetPlayerRespawnTimer(PC);

	WeaponComponent->OnCharacterDeath();
	OnCharacterDeath.Broadcast();

	if (const auto GameState = GetWorld()->GetGameState<ASaucewichGameState>())
		GameState->MulticastPlayerDeath(State, Attacker, Inflictor);
}

void ATpsCharacter::MoveForward(const float AxisValue)
{
	AddMovementInput(GetActorForwardVector(), FMath::Sign(AxisValue));
}

void ATpsCharacter::MoveRight(const float AxisValue)
{
	AddMovementInput(GetActorRightVector(), FMath::Sign(AxisValue));
}

void ATpsCharacter::Respawn()
{
	if (IsAlive()) return;
	if (const auto PC = GetController<ASaucewichPlayerController>())
	{
		PC->Respawn();
	}
}

void ATpsCharacter::SetActorActivated(const bool bActive)
{
	SetActorTickEnabled(bActive);
	SetActorHiddenInGame(!bActive);
	SetActorEnableCollision(bActive);
	GetCharacterMovement()->SetComponentTickEnabled(bActive);
}

void ATpsCharacter::OnTeamChanged(const uint8 NewTeam)
{
	SetColorToTeamColor();
}

void ATpsCharacter::BindOnTeamChanged()
{
	if (const auto PState = GetPlayerState())
	{
		State = Cast<ASaucewichPlayerState>(PState);
		if (State)
		{
			State->OnTeamChangedDelegate.AddDynamic(this, &ATpsCharacter::OnTeamChanged);
			OnTeamChanged(State->GetTeam());
		}
		else
		{
			UE_LOG(LogTpsCharacter, Error, TEXT("PlayerState -> SaucewichPlayerState 변환에 실패했습니다. 일부 기능이 작동하지 않을 수 있습니다."));
		}
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ATpsCharacter::BindOnTeamChanged);
	}
}

void ATpsCharacter::SetColorToTeamColor()
{
	if (const auto GameState = GetWorld()->GetGameState())
	{
		if (const auto GS = Cast<ASaucewichGameState>(GameState))
		{
			SetColor(GetTeamColor(GS));
		}
		else
		{
			UE_LOG(LogTpsCharacter, Error, TEXT("GameState가 있긴 한데 ASaucewichGameState가 아닙니다!"));
		}
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ATpsCharacter::SetColorToTeamColor);
	}
}

FLinearColor ATpsCharacter::GetTeamColor(ASaucewichGameState* const GameState) const
{
	return GameState->GetTeamData(GetTeam()).Color;
}

void ATpsCharacter::OnRep_Alive()
{
	if (bAlive) SetPlayerDefaults();
	else Kill();
}

void ATpsCharacter::RegisterGameMode()
{
	if (!HasAuthority()) return;
	GameMode = GetWorld()->GetAuthGameMode<ASaucewichGameMode>();
}

void ATpsCharacter::UpdateShadow() const
{
	auto Start = GetActorLocation();
	Start.Z -= GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	auto End = Start;
	End.Z -= ShadowData.MaxDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	FHitResult Hit;
	const auto bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	Shadow->SetVisibility(bHit);
	if (bHit)
	{
		Hit.Location.Z += .01f;
		Shadow->SetWorldLocationAndRotation(
			Hit.Location,
			Hit.Normal.RotateAngleAxis(90.f, FVector::RightVector).Rotation()
		);
		ShadowData.Material->SetScalarParameterValue("Darkness", (1.f - (Start.Z - Hit.Location.Z) / ShadowData.MaxDistance) * ShadowData.Darkness);
	}
}

void ATpsCharacter::BeTransl()
{
	if (bTransl) return;

	const auto Colored = GetMesh()->GetMaterialIndex("TeamColor");
	const auto NumMat = GetMesh()->GetNumMaterials();
	for (auto i = 0; i < NumMat; ++i)
	{
		const auto Ptr = TranslMatByIdx.Find(i);
		const auto Mat = Ptr ? *Ptr : DefTranslMat;

		if (i == Colored)
		{
			const auto Color = GetColor();
			GetMesh()->CreateDynamicMaterialInstance(i, Mat)->SetVectorParameterValue("Color", Color);
		}
		else
		{
			GetMesh()->SetMaterial(i, Mat);
		}
	}

	bTransl = true;
}

void ATpsCharacter::BeOpaque()
{
	if (!bTransl) return;

	const auto* const DefMesh = GetDefault<ACharacter>(GetClass())->GetMesh();
	const auto Colored = GetMesh()->GetMaterialIndex("TeamColor");
	const auto NumMat = GetMesh()->GetNumMaterials();
	for (auto i = 0; i < NumMat; ++i)
	{
		if (i == Colored)
		{
			FLinearColor Color;
			GetMesh()->GetMaterial(i)->GetVectorParameterValue({"Color"}, Color);
			DynamicMaterial->SetVectorParameterValue("Color", Color);
			GetMesh()->SetMaterial(i, DynamicMaterial);
		}
		else
		{
			GetMesh()->SetMaterial(i, DefMesh->GetMaterial(i));
		}
	}

	bTransl = false;
}
