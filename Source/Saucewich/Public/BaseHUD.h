// Copyright 2019 Othereum. All Rights Reserved.

#pragma once

#include "GameFramework/HUD.h"
#include "UObject/TextProperty.h"
#include "BaseHUD.generated.h"

UCLASS()
class SAUCEWICH_API ABaseHUD : public AHUD
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void ShowError(FText Message, bool bCritical);

protected:
	void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent)
	void OnShowError();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TSubclassOf<class UErrorWidget> ErrorWidgetClass;

	UPROPERTY(Transient)
	UErrorWidget* ErrorWidget;
};
