#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NartyPauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyPauseResume);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyPauseSave);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyPauseNewGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNartyPauseQuit);

UCLASS()
class NARTY_API UNartyPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Narty|Pause")
	FOnNartyPauseResume OnResume;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Pause")
	FOnNartyPauseSave OnSave;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Pause")
	FOnNartyPauseNewGame OnNewGame;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Pause")
	FOnNartyPauseQuit OnQuit;

	void SetStatusText(const FText& Status);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildLayout();
	UButton* AddMenuButton(UVerticalBox* Root, const FText& Label, const FName& Name);

	UFUNCTION()
	void HandleResume();

	UFUNCTION()
	void HandleSave();

	UFUNCTION()
	void HandleNewGame();

	UFUNCTION()
	void HandleQuit();

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY()
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY()
	TObjectPtr<UButton> SaveButton;

	UPROPERTY()
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;

	FText CachedStatus;
};
