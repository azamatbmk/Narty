#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NartyInteractable.generated.h"

class ACharacter;

UINTERFACE(MinimalAPI)
class UNartyInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Sites that respond to the shared Interact action (E). */
class NARTY_API INartyInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanNartyInteract() const = 0;
	virtual void TryNartyInteract(ACharacter* Character) = 0;
};
