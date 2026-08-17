#include "NartyGameMode.h"

#include "UObject/ConstructorHelpers.h"

ANartyGameMode::ANartyGameMode()
{
	// Keep Third Person Enhanced Input (IMC_Default / IMC_MouseLook) from the template PC.
	static ConstructorHelpers::FClassFinder<APlayerController> ThirdPersonPC(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (ThirdPersonPC.Succeeded())
	{
		PlayerControllerClass = ThirdPersonPC.Class;
	}

	static ConstructorHelpers::FClassFinder<APawn> ThirdPersonPawn(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (ThirdPersonPawn.Succeeded())
	{
		DefaultPawnClass = ThirdPersonPawn.Class;
	}
}
