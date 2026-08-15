#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyChoiceDialogWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNartyChoicePicked, int32, ChoiceIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyChoiceClosed);

UCLASS()
class NARTY_API UNartyChoiceDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Narty|Dialog")
	FOnNartyChoicePicked OnChoicePicked;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Dialog")
	FOnNartyChoiceClosed OnClosed;

	void Setup(const FText& Title, const FText& Body, const TArray<FText>& Choices, bool bAllowDismiss = true);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();
	UButton* AddChoiceButton(class UVerticalBox* Root, int32 Index);

	UFUNCTION()
	void HandleChoice0();
	UFUNCTION()
	void HandleChoice1();
	UFUNCTION()
	void HandleChoice2();
	UFUNCTION()
	void HandleClose();

	void Pick(int32 Index);

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> ChoiceButtons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> ChoiceLabels;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	FText CachedTitle;
	FText CachedBody;
	TArray<FText> CachedChoices;
	bool bCachedAllowDismiss = true;
};
