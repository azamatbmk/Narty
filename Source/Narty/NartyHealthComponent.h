#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NartyHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNartyHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnNartyDied, AActor*, DeadActor, AActor*, Killer);

UCLASS(ClassGroup = (Narty), meta = (BlueprintSpawnableComponent))
class NARTY_API UNartyHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNartyHealthComponent();

	UFUNCTION(BlueprintCallable, Category = "Narty|Health")
	float ApplyDamage(float Damage, AActor* InstigatorActor);

	UFUNCTION(BlueprintCallable, Category = "Narty|Health")
	void ResetToFull(float InvulnerableSeconds = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Narty|Health")
	void SetMaxHealth(float InMaxHealth, bool bFill = true);

	UFUNCTION(BlueprintPure, Category = "Narty|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Narty|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Narty|Health")
	bool IsDead() const { return bDead; }

	UFUNCTION(BlueprintPure, Category = "Narty|Health")
	bool IsInvulnerable() const;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Health")
	FOnNartyHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Narty|Health")
	FOnNartyDied OnDied;

	UPROPERTY(EditAnywhere, Category = "Narty|Health")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Health")
	float HitInvulnSeconds = 0.35f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Narty|Health")
	float Health = 100.f;

	bool bDead = false;
	double InvulnerableUntil = 0.0;
};
