#include "NartyPlayerController.h"

#include "NartyGameInstance.h"

void ANartyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Hero select is owned by UNartyGameInstance (map may use BP GameMode/PC).
	if (UNartyGameInstance* GI = GetGameInstance<UNartyGameInstance>())
	{
		if (GI->HasSelectedHero())
		{
			GI->ApplySelectedHeroToLocalPawn();
		}
	}
}

void ANartyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UNartyGameInstance* GI = GetGameInstance<UNartyGameInstance>())
	{
		if (GI->HasSelectedHero())
		{
			GI->ApplySelectedHeroToLocalPawn();
		}
	}
}
