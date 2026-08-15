#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyInteractPromptWidget.generated.h"

class UTextBlock;

UCLASS()
class NARTY_API UNartyInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPromptVisible(bool bVisible);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

	UPROPERTY()
	TObjectPtr<UTextBlock> PromptText;
};
