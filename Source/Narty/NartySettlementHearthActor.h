#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartySettlementHearthActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UBoxComponent;
class UTextRenderComponent;
class ACharacter;

/** Settlement hearth — return the mountain fire here. */
UCLASS()
class NARTY_API ANartySettlementHearthActor : public AActor
{
	GENERATED_BODY()

public:
	ANartySettlementHearthActor();

	void ActivateReturnObjective();
	void CompleteWithFire();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHearthOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Bowl;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> HearthLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	bool bReturnActive = false;
	bool bCompleted = false;
};
