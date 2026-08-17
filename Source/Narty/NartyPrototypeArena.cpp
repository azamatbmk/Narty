#include "NartyPrototypeArena.h"

#include "Components/SceneComponent.h"

ANartyPrototypeArena::ANartyPrototypeArena()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
