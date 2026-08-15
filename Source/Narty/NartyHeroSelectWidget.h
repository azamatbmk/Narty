#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyHeroTypes.h"
#include "NartyHeroSelectWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNartyHeroChosen, ENartyHero, Hero);

UCLASS()
class NARTY_API UNartyHeroSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Narty|Hero")
	FOnNartyHeroChosen OnHeroChosen;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();
	UButton* AddHeroButton(UVerticalBox* Root, const FText& Label, ENartyHero Hero);

	UFUNCTION()
	void HandleSoslanClicked();

	UFUNCTION()
	void HandleBatrazClicked();

	UFUNCTION()
	void HandleSyrdonClicked();

	void ChooseHero(ENartyHero Hero);

	UPROPERTY()
	TObjectPtr<UButton> SoslanButton;

	UPROPERTY()
	TObjectPtr<UButton> BatrazButton;

	UPROPERTY()
	TObjectPtr<UButton> SyrdonButton;
};
