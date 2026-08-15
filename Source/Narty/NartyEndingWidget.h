#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyEndingWidget.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyPlayAgain);

UCLASS()
class NARTY_API UNartyEndingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Narty|Ending")
	FOnNartyPlayAgain OnPlayAgain;

	void SetEndingTexts(const FText& Title, const FText& Body);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandlePlayAgain();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY()
	TObjectPtr<UButton> PlayAgainButton;

	FText CachedTitle;
	FText CachedBody;
};
