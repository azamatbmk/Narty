#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyInteractable.h"
#include "NartyMountainFireActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UTextRenderComponent;
class UNartyForgeDialogWidget;
class ANartyTrainingDummy;
class ACharacter;

/** Mountain fire objective for chapter "Fire of the settlement". */
UCLASS()
class NARTY_API ANartyMountainFireActor : public AActor, public INartyInteractable
{
	GENERATED_BODY()

public:
	ANartyMountainFireActor();

	void ActivateForQuest();
	int32 CaptureGuardiansAlive() const;
	void RestoreFromSave(bool bActive, bool bTaken, int32 GuardiansAlive);

	virtual bool CanNartyInteract() const override;
	virtual void TryNartyInteract(ACharacter* Character) override;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void OnFireOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnFireEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleBargainAccepted();

	UFUNCTION()
	void HandleBargainClosed();

	void TryTakeFire(ACharacter* Character);
	void GiveFireToPlayer(ACharacter* Character);
	bool AreGuardiansDefeated() const;
	void OpenBargainDialog(ACharacter* Character);
	void RetryTakeForOverlappingPlayers();
	void EnsureGuardiansSpawned();

	UFUNCTION()
	void OnGuardianDefeated(AActor* DestroyedActor);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Pedestal;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> FlameMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> FlameLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TArray<TObjectPtr<ANartyTrainingDummy>> Guardians;

	UPROPERTY()
	TObjectPtr<UNartyForgeDialogWidget> BargainWidget;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InteractingCharacter;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InsideCharacter;

	bool bQuestActive = false;
	bool bFireTaken = false;
	bool bDialogOpen = false;
};
