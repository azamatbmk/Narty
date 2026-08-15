#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NartyPrototypeArena.generated.h"

class ANartyMountainFireActor;
class ANartySettlementHearthActor;
class ANartyUatsamongaCup;
class ANartyHeroTrialSite;
class ANartyFinaleSite;

UCLASS()
class NARTY_API ANartyPrototypeArena : public AActor
{
	GENERATED_BODY()

public:
	ANartyPrototypeArena();

	UFUNCTION(BlueprintCallable, Category = "Narty|Arena")
	FTransform GetPlayerStartTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Narty|Arena")
	static ANartyPrototypeArena* EnsureInWorld(UWorld* World);

	ANartyMountainFireActor* GetMountainFire() const { return MountainFire; }
	ANartySettlementHearthActor* GetHearth() const { return Hearth; }
	ANartyUatsamongaCup* GetUatsamongaCup() const { return UatsamongaCup; }
	ANartyHeroTrialSite* GetHeroTrialSite() const { return HeroTrialSite; }
	ANartyFinaleSite* GetFinaleSite() const { return FinaleSite; }

protected:
	virtual void BeginPlay() override;

	void BuildGeometry();
	UStaticMeshComponent* AddBlock(
		USceneComponent* Parent,
		const FName& Name,
		const FVector& RelativeLocation,
		const FVector& Scale,
		const FLinearColor& Color);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<ANartyMountainFireActor> MountainFire;

	UPROPERTY()
	TObjectPtr<ANartySettlementHearthActor> Hearth;

	UPROPERTY()
	TObjectPtr<ANartyUatsamongaCup> UatsamongaCup;

	UPROPERTY()
	TObjectPtr<ANartyHeroTrialSite> HeroTrialSite;

	UPROPERTY()
	TObjectPtr<ANartyFinaleSite> FinaleSite;

	bool bBuilt = false;
};
