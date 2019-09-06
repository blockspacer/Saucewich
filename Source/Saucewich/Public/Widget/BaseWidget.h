// Copyright 2019 Team Sosweet. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "BaseWidget.generated.h"

UCLASS()
class SAUCEWICH_API UBaseWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	void NativeOnInitialized() override;
	
public:
	UFUNCTION(BlueprintCallable)
	void SetComponent(TSubclassOf<class UComponentWidget> ComponentClass);
	
private:
	UPROPERTY(Transient)
	UComponentWidget* Component;

	UPROPERTY(Transient)
	class UCanvasPanel* RootPanel;
};