// Copyright 2019-2020 Seokjin Lee. All Rights Reserved.

#pragma once

#include "Player/SaucewichPlayerState.h"
#include "MakeSandwichPlayerState.generated.h"

class ASandwichIngredient;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerIngredientChanged, TSubclassOf<ASandwichIngredient>, NewIngredient);
DECLARE_EVENT_OneParam(AMakeSandwichPlayerState, FOnPlyIngChangedNative, TSubclassOf<ASandwichIngredient>)

USTRUCT(BlueprintType)
struct SAUCEWICH_API FIngredients
{
	GENERATED_BODY()

	FIngredients() = default;
	explicit FIngredients(AMakeSandwichPlayerState* Owner);
	
	auto& Get() const { return Ingredients; }
	void Reset();
	void Add(TSubclassOf<ASandwichIngredient> Ing);

private:
	void OnModify(TSubclassOf<ASandwichIngredient> NewIng) const;
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TMap<TSubclassOf<ASandwichIngredient>, uint8> Ingredients;

	UPROPERTY()
	AMakeSandwichPlayerState* Owner;
};

UCLASS()
class SAUCEWICH_API AMakeSandwichPlayerState : public ASaucewichPlayerState
{
	GENERATED_BODY()

public:
	AMakeSandwichPlayerState();
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void PickupIngredient(TSubclassOf<ASandwichIngredient> Class);

	void PutIngredientsInFridge();
	void BroadcastIngredientChanged(TSubclassOf<ASandwichIngredient> NewIng) const;
	auto& GetIngredients() const { return Ingredients.Get(); }

	UFUNCTION(BlueprintCallable)
	uint8 GetNumIngredients() const;

	UFUNCTION(BlueprintCallable)
	bool CanPickupIngredient() const;

	FOnPlyIngChangedNative OnIngChangedNative;
	ASandwichIngredient* PickingUp;

protected:
	void Reset() override;
	void OnDeath() override;
	void OnCharDestroyed() override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnPutIngredients();

	UFUNCTION(BlueprintImplementableEvent)
	void OnPickupIngredient();

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPickupIngredient(TSubclassOf<ASandwichIngredient> Class);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastResetIngredients();
	
	void DropIngredients();

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FIngredients Ingredients;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerIngredientChanged OnIngredientChanged;

	UPROPERTY(EditDefaultsOnly)
	uint8 MaxIngredients;
};
