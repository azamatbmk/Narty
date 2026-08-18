#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyHeroTypes.h"
#include "NartyInteractable.h"
#include "NartyCampaignSaveTypes.h"
#include "NartyHeroTrialSite.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UTextRenderComponent;
class UNartyChoiceDialogWidget;
class UNartyForgeDialogWidget;
class ANartyTrainingDummy;
class ACharacter;

/** Unique chapter 4 site — content depends on selected hero. */
UCLASS()
class NARTY_API ANartyHeroTrialSite : public AActor, public INartyInteractable
{
	GENERATED_BODY()

public:
	ANartyHeroTrialSite();

	void ActivateForHero(ENartyHero Hero);
	void CaptureSaveState(FNartyTrialSaveState& OutState) const;
	void RestoreFromSave(ENartyHero Hero, bool bInActive, bool bCompleted, const FNartyTrialSaveState& State);
	void ResetForNewRun();
	FVector GetGateWorldLocation() const;

	virtual bool CanNartyInteract() const override;
	virtual void TryNartyInteract(ACharacter* Character) override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnGateOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnGateEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void OnVisionOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleFinalChoice(int32 ChoiceIndex);

	UFUNCTION()
	void HandleFinalClosed();

	UFUNCTION()
	void HandleIntroAccepted();

	UFUNCTION()
	void HandleIntroClosed();

	void BuildSharedShell();
	void SetupSoslanTrial();
	void SetupBatrazTrial();
	void SetupSyrdonTrial();
	void EnterTrial(ACharacter* Character);
	void OpenIntro(ACharacter* Character);
	void OpenFinalDialog(ACharacter* Character);
	void CloseAnyDialog();
	void CompleteTrial(bool bNoblePath);
	void CheckBatrazCleared();
	void ClearTrialDummies();
	void ApplyTrialProgress(const FNartyTrialSaveState& State);
	void DisableVisionTrigger(int32 VisionIndex);
	void ApplyDummyHealthFromSave(const TArray<float>& HealthValues);
	UStaticMeshComponent* AddBlock(const FName& Name, const FVector& RelLoc, const FVector& Scale, const FLinearColor& Color);
	UBoxComponent* AddTrigger(const FName& Name, const FVector& RelLoc, const FVector& Extent, int32 VisionIndex);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> GateTrigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> GateLabel;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> AtmosphereLight;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> VisionTriggers;

	UPROPERTY()
	TArray<TObjectPtr<ANartyTrainingDummy>> TrialDummies;

	UPROPERTY()
	TObjectPtr<UNartyForgeDialogWidget> IntroWidget;

	UPROPERTY()
	TObjectPtr<UNartyChoiceDialogWidget> FinalWidget;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InteractingCharacter;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InsideGateCharacter;

	ENartyHero ActiveHero = ENartyHero::None;
	bool bActive = false;
	bool bEntered = false;
	bool bCompleted = false;
	bool bDialogOpen = false;
	bool bShellBuilt = false;
	bool bAwaitingFinalChoice = false;
	int32 VisionsSeen = 0;
	int32 SyrdonDeals = 0;
	FTimerHandle BatrazCheckHandle;
};
