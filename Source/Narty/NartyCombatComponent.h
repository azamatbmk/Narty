#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NartyHeroTypes.h"
#include "NartyCombatComponent.generated.h"

class UAnimMontage;
class UStaticMeshComponent;
class UInputAction;
class UInputMappingContext;
class ULocalPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNartyAbilityChanged, FText, AbilityLine);

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
	void TryAbility();

	UFUNCTION(BlueprintCallable, Category = "Narty|Combat")
	void GrantForgeWeapon();

	void ResetForNewRun();

	UFUNCTION(BlueprintPure, Category = "Narty|Combat")
	bool HasForgeWeapon() const { return bHasForgeWeapon; }

	UFUNCTION(BlueprintPure, Category = "Narty|Combat")
	bool IsStealthed() const { return bStealthed; }

	UFUNCTION(BlueprintPure, Category = "Narty|Combat")
	FText GetAbilityLine() const;

	void CancelPendingStrike();
	void NotifyAbilityHud();

	UPROPERTY(BlueprintAssignable, Category = "Narty|Combat")
	FOnNartyAbilityChanged OnAbilityChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void EnsureAttackInput();
	void RemoveAttackMapping();
	void PlayAttackAnimation();
	void PerformStrike();
	void EnsureWeaponMesh();
	bool IsOwnerDead() const;
	void DealStrikeDamage(float Damage, float Range, float Radius);
	void ActivateSoslanDash();
	void ActivateBatrazFortitude();
	void ActivateSyrdonStealth();
	void EndSoslanDash();
	void EndBatrazFortitude();
	void EndSyrdonStealth();
	void SweepDashHits();

	UFUNCTION()
	void HandleAttackStarted();

	UFUNCTION()
	void HandleAbilityStarted();

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

	UPROPERTY(EditAnywhere, Category = "Narty|Combat")
	float AbilityCooldown = 6.f;

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

	UPROPERTY()
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY()
	TObjectPtr<UInputAction> AbilityAction;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> AttackMappingContext;

	TWeakObjectPtr<ULocalPlayer> BoundLocalPlayer;

	float LastAttackTime = -100.f;
	float LastAbilityTime = -100.f;
	int32 AttackComboIndex = 0;
	bool bInputBound = false;
	bool bMappingAdded = false;
	bool bAbilityBound = false;
	bool bStealthed = false;
	bool bDashing = false;
	bool bFortitude = false;
	FTimerHandle StrikeDelayHandle;
	FTimerHandle AbilityEndHandle;
	TSet<AActor*> DashHitActors;
	FText LastAbilityLine;
};
