#include "NartyGameMode.h"

#include "NartyPlayerController.h"
#include "UObject/ConstructorHelpers.h"

ANartyGameMode::ANartyGameMode()
{
	PlayerControllerClass = ANartyPlayerController::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> ThirdPersonPawn(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (ThirdPersonPawn.Succeeded())
	{
		DefaultPawnClass = ThirdPersonPawn.Class;
	}
}
