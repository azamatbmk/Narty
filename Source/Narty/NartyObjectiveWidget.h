#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyObjectiveWidget.generated.h"

class UTextBlock;
class UBorder;

UCLASS()
class NARTY_API UNartyObjectiveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetObjective(const FText& Title, const FText& Detail);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY()
	TObjectPtr<UTextBlock> DetailText;
};
