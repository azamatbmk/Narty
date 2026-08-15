#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyForgeDialogWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyForgeAccepted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyForgeClosed);

UCLASS()
class NARTY_API UNartyForgeDialogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Narty|Forge")
	FOnNartyForgeAccepted OnAccepted;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Forge")
	FOnNartyForgeClosed OnClosed;

	void SetDialogTexts(const FText& Title, const FText& Body, const FText& AcceptLabel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildLayout();

	UFUNCTION()
	void HandleAcceptClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY()
	TObjectPtr<UButton> AcceptButton;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> AcceptLabelText;

	FText CachedTitle;
	FText CachedBody;
	FText CachedAccept;
	bool bBuilt = false;
};
