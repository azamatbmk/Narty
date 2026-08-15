#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyTrainingDummy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNartyDummyDefeated, AActor*, Dummy);

UCLASS()
class NARTY_API ANartyTrainingDummy : public AActor
{
	GENERATED_BODY()

public:
	ANartyTrainingDummy();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	float ReceiveStrike(float Damage, AActor* InstigatorActor);

	UFUNCTION(BlueprintPure, Category = "Narty|Combat")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	void SetMaxHealth(float InMaxHealth);

	/** Fired once when health reaches zero. */
	UPROPERTY(BlueprintAssignable, Category = "Narty|Combat")
	FOnNartyDummyDefeated OnDefeated;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> HPLabel;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "Narty|Combat")
	float Health = 100.f;

	void RefreshLabel();
};
