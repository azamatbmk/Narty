#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyUatsamongaCup.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UTextRenderComponent;
class UNartyChoiceDialogWidget;
class ACharacter;

/** Cup of truth — boils only before an honest deed. */
UCLASS()
class NARTY_API ANartyUatsamongaCup : public AActor
{
	GENERATED_BODY()

public:
	ANartyUatsamongaCup();

	void ActivateForQuest();
	void PlayBoil(bool bBoil);
	bool HasJudged() const { return bJudged; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnCupOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleChoice(int32 ChoiceIndex);

	UFUNCTION()
	void HandleClosed();

	void OpenJudgment(ACharacter* Character);
	void CloseDialog();
	void ResolveChoice(int32 ChoiceIndex);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Table;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> CupMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> CupLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UNartyChoiceDialogWidget> DialogWidget;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InteractingCharacter;

	/** ChoiceIndex -> is truth */
	TArray<bool> ChoiceIsTruth;

	bool bQuestActive = false;
	bool bDialogOpen = false;
	bool bJudged = false;
	bool bBoiling = false;
	float BoilTime = 0.f;
};
