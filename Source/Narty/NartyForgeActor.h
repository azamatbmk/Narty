#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyForgeActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UTextRenderComponent;
class UNartyForgeDialogWidget;
class ACharacter;

UCLASS()
class NARTY_API ANartyForgeActor : public AActor
{
	GENERATED_BODY()

public:
	ANartyForgeActor();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnForgeOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void OnForgeEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	UFUNCTION()
	void HandleDialogAccepted();

	UFUNCTION()
	void HandleDialogClosed();

	UFUNCTION()
	void HandleInteractPressed();

	void OpenDialog(ACharacter* Character);
	void CloseDialog();
	void BindInteractInput(ACharacter* Character);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> AnvilMesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> ForgeLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> InteractTrigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY()
	TObjectPtr<UNartyForgeDialogWidget> DialogWidget;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InteractingCharacter;

	UPROPERTY()
	TWeakObjectPtr<ACharacter> InsideCharacter;

	bool bWeaponGranted = false;
	bool bDialogOpen = false;
	bool bInteractBound = false;
};
