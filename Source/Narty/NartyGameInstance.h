#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "NartyHeroTypes.h"
#include "NartyQuestTypes.h"
#include "NartyGameInstance.generated.h"

class UNartyHeroSelectWidget;
class UNartyObjectiveWidget;
class UNartyEndingWidget;
class UNartyHealthWidget;
class APlayerController;
class ACharacter;
class UNartyCombatComponent;
class UNartyHealthComponent;

UCLASS()
class NARTY_API UNartyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void OnStart() override;
	virtual void LoadComplete(const float LoadTime, const FString& MapName) override;

	UFUNCTION(BlueprintCallable, Category = "Narty|Hero")
	void SetSelectedHero(ENartyHero Hero);

	UFUNCTION(BlueprintPure, Category = "Narty|Hero")
	ENartyHero GetSelectedHero() const { return SelectedHero; }

	UFUNCTION(BlueprintPure, Category = "Narty|Hero")
	bool HasSelectedHero() const { return SelectedHero != ENartyHero::None; }

	UFUNCTION(BlueprintPure, Category = "Narty|Hero")
	FNartyHeroStats GetStatsForHero(ENartyHero Hero) const;

	UFUNCTION(BlueprintPure, Category = "Narty|Hero")
	FNartyHeroStats GetSelectedHeroStats() const;

	UFUNCTION(BlueprintCallable, Category = "Narty|Hero")
	void ApplySelectedHeroToLocalPawn();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void StartFireQuest();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void NotifyFireTaken();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void NotifyFireReturned();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void StartUatsamongaQuest();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void NotifyUatsamongaResolved(bool bToldTruth);

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void StartHeroTrial();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void NotifyHeroTrialCompleted(bool bNoblePath);

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void StartFinale();

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void NotifyFinaleCompleted(ENartyEnding Ending);

	UFUNCTION(BlueprintPure, Category = "Narty|Quest")
	bool HasMountainFire() const { return bHasMountainFire; }

	UFUNCTION(BlueprintPure, Category = "Narty|Quest")
	bool ToldTruthAtCup() const { return bToldTruthAtCup; }

	UFUNCTION(BlueprintPure, Category = "Narty|Quest")
	bool ChoseNobleTrialPath() const { return bNobleTrialPath; }

	UFUNCTION(BlueprintPure, Category = "Narty|Quest")
	ENartyEnding GetEnding() const { return ChosenEnding; }

	UFUNCTION(BlueprintPure, Category = "Narty|Quest")
	ENartyQuestStage GetQuestStage() const { return QuestStage; }

	UFUNCTION(BlueprintCallable, Category = "Narty|Quest")
	void RestartCampaign();

protected:
	UFUNCTION()
	void HandleHeroChosen(ENartyHero Hero);

	UFUNCTION()
	void HandlePlayAgain();

	void ShowHeroSelectMenu();
	void HideHeroSelectMenu();
	void ApplyHeroToCharacter(ACharacter* HeroCharacter, ENartyHero Hero);
	void EnsurePlayerReady();
	void BootstrapGorge();
	void EnsureObjectiveWidget();
	void EnsureHealthWidget();
	void UpdateObjectiveUI();
	void ShowEndingScreen(ENartyEnding Ending);
	void HideEndingScreen();
	void ResetCampaignState();
	void GetEndingTexts(ENartyEnding Ending, FText& OutTitle, FText& OutBody) const;
	FText GetHeroFireHint() const;
	void RespawnPlayerAtNykhas();

	UFUNCTION()
	void HandlePlayerDied(AActor* DeadActor, AActor* Killer);

	UFUNCTION()
	void HandlePlayerHealthChanged(float Health, float MaxHealth);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Hero")
	ENartyHero SelectedHero = ENartyHero::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Quest")
	ENartyQuestStage QuestStage = ENartyQuestStage::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Quest")
	bool bHasMountainFire = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Quest")
	bool bToldTruthAtCup = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Quest")
	bool bClanFeudStarted = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Quest")
	bool bNobleTrialPath = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Narty|Quest")
	ENartyEnding ChosenEnding = ENartyEnding::None;

	UPROPERTY()
	TObjectPtr<UNartyHeroSelectWidget> HeroSelectWidget;

	UPROPERTY()
	TObjectPtr<UNartyObjectiveWidget> ObjectiveWidget;

	UPROPERTY()
	TObjectPtr<UNartyEndingWidget> EndingWidget;

	UPROPERTY()
	TObjectPtr<UNartyHealthWidget> HealthWidget;

	FTimerHandle HeroSelectRetryHandle;
	FTimerHandle PlayerReadyRetryHandle;
	FTimerHandle ApplyHeroRetryHandle;
	FTimerHandle RespawnHandle;

	bool bRestartPending = false;
	bool bPlayerHealthBound = false;
};
