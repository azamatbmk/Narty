#include "NartyPlayerController.h"

#include "NartyGameInstance.h"
#include "NartyInteractComponent.h"
#include "GameFramework/Character.h"

void ANartyPlayerController::BeginPlay()
{
	Super::BeginPlay();

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

	if (ACharacter* HeroPawn = Cast<ACharacter>(InPawn))
	{
		UNartyInteractComponent::EnsureOn(HeroPawn);
	}

	if (UNartyGameInstance* GI = GetGameInstance<UNartyGameInstance>())
	{
		if (GI->HasSelectedHero())
		{
			GI->ApplySelectedHeroToLocalPawn();
		}
	}
}
