#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyHealthWidget.generated.h"

class UTextBlock;
class UProgressBar;

UCLASS()
class NARTY_API UNartyHealthWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetHealth(float Health, float MaxHealth);
	void SetAbilityLine(const FText& Line);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildLayout();

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> AbilityText;
};
