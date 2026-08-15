#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyHeroTypes.h"
#include "NartyQuestTypes.h"
#include "NartyFinaleSite.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UTextRenderComponent;
class UNartyForgeDialogWidget;
class UNartyChoiceDialogWidget;
class ACharacter;

/** Final chapter — End of the Narts. */
UCLASS()
class NARTY_API ANartyFinaleSite : public AActor
{
	GENERATED_BODY()

public:
	ANartyFinaleSite();

	void ActivateFinale();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnSanctumOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnSanctumEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleSatanaAccepted();

	UFUNCTION()
	void HandleSatanaClosed();

	UFUNCTION()
	void HandleEndingChoice(int32 ChoiceIndex);

	UFUNCTION()
	void HandleEndingClosed();

	UFUNCTION()
	void HandleInteractPressed();

	void BuildSanctum();
	void OpenSatana(ACharacter* Character);
	void OpenEndingChoice(ACharacter* Character);
	void CloseDialogs();
	void ResolveEnding(int32 ChoiceIndex);
	void PlayEndingVisual(ENartyEnding Ending);
	void BindInteractInput(ACharacter* Character);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> SkyLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PillarLeft;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> PillarRight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Altar;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UNartyForgeDialogWidget> SatanaWidget;

	UPROPERTY()
	TObjectPtr<UNartyChoiceDialogWidget> EndingWidget;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InteractingCharacter;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InsideCharacter;

	bool bActive = false;
	bool bDialogOpen = false;
	bool bResolved = false;
	bool bStorm = false;
	bool bInteractBound = false;
	float StormTime = 0.f;
};
