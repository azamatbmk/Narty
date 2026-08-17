#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyUaigEnemy.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UNartyHealthComponent;

UCLASS()
class NARTY_API ANartyUaigEnemy : public AActor
{
	GENERATED_BODY()

public:
	ANartyUaigEnemy();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void TrySmash(AActor* Target);
	void RefreshLabel();
	bool HasLineOfSightTo(const AActor* Target) const;

	UFUNCTION()
	void HandleHealthChanged(float InHealth, float InMaxHealth);

	UFUNCTION()
	void HandleDied(AActor* DeadActor, AActor* Killer);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(VisibleAnywhere, Category = "Narty|Combat")
	TObjectPtr<UNartyHealthComponent> Health;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float WalkSpeed = 280.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AggroRange = 1600.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackRange = 155.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackDamage = 22.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackCooldown = 1.35f;

	float LastSmashTime = -100.f;
};
