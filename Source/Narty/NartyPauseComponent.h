#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NartyPauseComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class ULocalPlayer;

UCLASS(ClassGroup = (Narty), meta = (BlueprintSpawnableComponent))
class NARTY_API UNartyPauseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNartyPauseComponent();

	static UNartyPauseComponent* EnsureOn(class ACharacter* Character);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void EnsurePauseInput();
	void RemovePauseMapping();
	void HandlePauseStarted();
	bool ConsumePauseKeyPress(class APlayerController* PC);

	UPROPERTY()
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> PauseMappingContext;

	TWeakObjectPtr<ULocalPlayer> BoundLocalPlayer;
	bool bMappingAdded = false;
	bool bInputBound = false;
};
