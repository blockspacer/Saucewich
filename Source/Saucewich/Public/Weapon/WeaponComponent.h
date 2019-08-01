// Copyright 2019 Team Sosweet. All Rights Reserved.

#pragma once

#include "Saucewich.h"
#include "Components/SceneComponent.h"
#include "Colorable.h"
#include "WeaponComponent.generated.h"

/*
 * 캐릭터와 무기가 상호작용하는 중간다리입니다.
 */
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UWeaponComponent : public USceneComponent, public IColorable
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	virtual void SetupPlayerInputComponent(class UInputComponent* Input);

	/*
	 * [Server] Gives a weapon. Replaces if already in the same slot.
	 * @param WeaponClass: Class of weapon to give
	 * @return: The weapon given
	 */
	UFUNCTION(BlueprintCallable)
	virtual class AWeapon* Give(TSubclassOf<AWeapon> WeaponClass);

	/*
	 * [Shared] Returns active weapon.
	 * @return: The weapon currently equipped
	 */
	UFUNCTION(BlueprintCallable)
	AWeapon* GetActiveWeapon() const { return GetWeapon(Active); }

	UFUNCTION(BlueprintCallable)
	AWeapon* GetWeapon(const uint8 Slot) const { return Slot < Weapons.Num() ? Weapons[Slot] : nullptr; }

	UFUNCTION(BlueprintCallable)
	EGunTraceHit GunTrace(FHitResult& OutHit) const;

	void OnCharacterDeath();
	float GetSpeedRatio() const;
	virtual bool TrySelectWeapon(uint8 Slot);
	uint8 GetSlots() const { return WeaponSlots; }
	void SetColor(const FLinearColor& NewColor) override;

	class ATpsCharacter* const Owner = nullptr;

protected:
	void InitializeComponent() override;
	void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	virtual void OnRep_Weapons();

private:
	UFUNCTION(BlueprintCallable) void FireP();
	UFUNCTION(BlueprintCallable) void FireR();
	UFUNCTION(Server, Reliable, WithValidation) void ServerFireP();
	UFUNCTION(Server, Reliable, WithValidation) void ServerFireR();
	UFUNCTION(NetMulticast, Reliable) void MulticastFireP();
	UFUNCTION(NetMulticast, Reliable) void MulticastFireR();

	UFUNCTION(BlueprintCallable) void SlotP(uint8 Slot);
	UFUNCTION(BlueprintCallable) void SlotR(uint8 Slot);
	UFUNCTION(Server, Reliable, WithValidation) void ServerSlotP(uint8 Slot);
	UFUNCTION(Server, Reliable, WithValidation) void ServerSlotR(uint8 Slot);
	UFUNCTION(NetMulticast, Reliable) void MulticastSlotP(uint8 Slot);
	UFUNCTION(NetMulticast, Reliable) void MulticastSlotR(uint8 Slot);
	
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_Weapons, Transient)
	TArray<AWeapon*> Weapons;

	UPROPERTY(EditDefaultsOnly, meta = (UIMax = 9, ClampMax = 9))
	uint8 WeaponSlots;

	UPROPERTY(VisibleInstanceOnly, Replicated, Transient)
	uint8 Active;
};
