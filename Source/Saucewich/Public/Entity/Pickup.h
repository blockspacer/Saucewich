// Copyright 2019-2020 Seokjin Lee. All Rights Reserved.

#pragma once

#include "Entity/PoolActor.h"
#include "Pickup.generated.h"

class ATpsCharacter;

UCLASS()
class SAUCEWICH_API APickup : public APoolActor
{
	GENERATED_BODY()

public:
	APickup();
	UStaticMeshComponent* GetMesh() const { return Mesh; }
	
	uint8 bSpawnedFromSpawner : 1;

protected:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;

	void NotifyActorBeginOverlap(AActor* OtherActor) override;
	void NotifyActorEndOverlap(AActor* OtherActor) override;

	void OnActivated() override;
	void OnReleased() override;

	void BePickedUp(ATpsCharacter* By);
	
	UFUNCTION(BlueprintNativeEvent)
	void OnPickedUp(ATpsCharacter* By);

	UFUNCTION(BlueprintNativeEvent)
	void StartPickUp(ATpsCharacter* By);

	UFUNCTION(BlueprintNativeEvent)
	void CancelPickUp(ATpsCharacter* By);
	
	UFUNCTION(BlueprintNativeEvent)
	bool CanPickedUp(const ATpsCharacter* By) const;
	
	UFUNCTION(BlueprintNativeEvent)
	bool CanEverPickedUp(const ATpsCharacter* By) const;
	
private:
	void Freeze();

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSetLocation(FVector Location);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	class UShadowComponent* Shadow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TSoftObjectPtr<UTexture2D> Icon;

	ATpsCharacter* PickingChar;
	float PickingTimer;

	// 재료를 획득하는데 걸리는 시간
	UPROPERTY(EditDefaultsOnly)
	float PickupTime = 1;
	
	UPROPERTY(EditAnywhere)
	float BounceScale = 10;

	UPROPERTY(EditAnywhere)
	float BounceSpeed = 5;

	UPROPERTY(EditAnywhere)
	float RotateSpeed = 100;

	// Pickup끼리 겹쳐있을 때 서로 밀어내는 힘의 세기입니다.
	UPROPERTY(EditAnywhere)
	float PushStrength = 1000;

	float Time;
};
