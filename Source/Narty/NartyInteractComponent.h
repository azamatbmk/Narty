#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NartyInteractComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class ACharacter;
class ULocalPlayer;
class UNartyInteractPromptWidget;

/**
 * Shared Interact (E) via Enhanced Input.
 * Lives on the pawn so it works even when the map uses BP_ThirdPersonGameMode.
 */
UCLASS(ClassGroup = (Narty), meta = (BlueprintSpawnableComponent))
class NARTY_API UNartyInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNartyInteractComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static UNartyInteractComponent* EnsureOn(ACharacter* Character);

	/** Re-evaluate E prompt (call after dialog open/close). */
	static void NotifyPromptChanged(ACharacter* Character);

	void PushInteractTarget(AActor* Interactable);
	void PopInteractTarget(AActor* Interactable);
	void ClearFocus();

	void RefreshPrompt();

protected:
	void EnsureInput();
	void RemoveInteractMapping();
	void UpdatePrompt();
	bool HasAvailableInteract() const;

	UFUNCTION()
	void HandleInteractStarted();

	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> InteractMappingContext;

	UPROPERTY()
	TObjectPtr<UNartyInteractPromptWidget> PromptWidget;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> InteractFocusStack;

	TWeakObjectPtr<ULocalPlayer> BoundLocalPlayer;

	bool bInputBound = false;
	bool bMappingAdded = false;
};
