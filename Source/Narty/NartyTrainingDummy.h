#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyTrainingDummy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UNartyHealthComponent;

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
	float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	void SetMaxHealth(float InMaxHealth, bool bFill = true);

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	void SetHealth(float InHealth);

	/** Fired once when health reaches zero. */
	UPROPERTY(BlueprintAssignable, Category = "Narty|Combat")
	FOnNartyDummyDefeated OnDefeated;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> HPLabel;

	UPROPERTY(VisibleAnywhere, Category = "Narty|Combat")
	TObjectPtr<UNartyHealthComponent> Health;

	void RefreshLabel();

	UFUNCTION()
	void HandleHealthChanged(float InHealth, float InMaxHealth);

	UFUNCTION()
	void HandleDied(AActor* DeadActor, AActor* Killer);
};
