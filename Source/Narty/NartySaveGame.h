#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "NartyHeroTypes.h"
#include "NartyQuestTypes.h"
#include "NartyCampaignSaveTypes.h"
#include "NartySaveGame.generated.h"

UCLASS()
class NARTY_API UNartySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static const int32 CurrentVersion = 2;

	UPROPERTY()
	int32 SaveVersion = CurrentVersion;

	UPROPERTY()
	ENartyHero SelectedHero = ENartyHero::None;

	UPROPERTY()
	ENartyQuestStage QuestStage = ENartyQuestStage::None;

	UPROPERTY()
	bool bHasMountainFire = false;

	UPROPERTY()
	bool bToldTruthAtCup = false;

	UPROPERTY()
	bool bClanFeudStarted = false;

	UPROPERTY()
	bool bNobleTrialPath = false;

	UPROPERTY()
	bool bHasForgeWeapon = false;

	UPROPERTY()
	ENartyEnding ChosenEnding = ENartyEnding::None;

	UPROPERTY()
	float PlayerHealth = 0.f;

	UPROPERTY()
	bool bHasPlayerTransform = false;

	UPROPERTY()
	FVector PlayerLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PlayerRotation = FRotator::ZeroRotator;

	/** -1 = guardians not spawned yet; 0 = all defeated; 1-2 = alive count. */
	UPROPERTY()
	int32 FireGuardiansAlive = -1;

	UPROPERTY()
	FNartyTrialSaveState TrialState;
};
