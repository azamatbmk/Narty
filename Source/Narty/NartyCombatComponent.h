#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NartyHeroTypes.h"
#include "NartyCombatComponent.generated.h"

class UAnimMontage;
class UStaticMeshComponent;

UCLASS(ClassGroup = (Narty), meta = (BlueprintSpawnableComponent))
class NARTY_API UNartyCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNartyCombatComponent();

	void SetHero(ENartyHero InHero);

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	void TryAttack();

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	void GrantForgeWeapon();

	UFUNCTION(BlueprintPure, Category = "Narty|Combat")
	bool HasForgeWeapon() const { return bHasForgeWeapon; }

protected:
	virtual void BeginPlay() override;

	void BindAttackInput();
	void PlayAttackAnimation();
	void PerformStrike();
	void EnsureWeaponMesh();

	UPROPERTY(VisibleAnywhere, Category = "Narty|Combat")
	ENartyHero Hero = ENartyHero::None;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackRange = 140.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackRadius = 45.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackDamage = 25.f;

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AttackCooldown = 0.45f;

	UPROPERTY(VisibleAnywhere, Category = "Narty|Combat")
	bool bHasForgeWeapon = false;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	UPROPERTY()
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	UPROPERTY()
	TArray<TObjectPtr<UAnimSequence>> AttackSequences;

	UPROPERTY()
	TObjectPtr<UStaticMesh> WeaponMeshAsset;

	float LastAttackTime = -100.f;
	int32 AttackComboIndex = 0;
	bool bInputBound = false;
	FTimerHandle StrikeDelayHandle;
};
