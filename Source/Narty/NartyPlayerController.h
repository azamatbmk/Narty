#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NartyPlayerController.generated.h"

UCLASS()
class NARTY_API ANartyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
};
