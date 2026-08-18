#include "NartyPauseComponent.h"

#include "NartyGameInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

UNartyPauseComponent::UNartyPauseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UNartyPauseComponent* UNartyPauseComponent::EnsureOn(ACharacter* Character)
{
	if (!Character)
	{
		return nullptr;
	}

	UNartyPauseComponent* Pause = Character->FindComponentByClass<UNartyPauseComponent>();
	if (!Pause)
	{
		Pause = NewObject<UNartyPauseComponent>(Character, TEXT("NartyPause"));
		Pause->RegisterComponent();
		Character->AddInstanceComponent(Pause);
	}

	return Pause;
}

void UNartyPauseComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsurePauseInput();
}

void UNartyPauseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemovePauseMapping();
	bInputBound = false;
	Super::EndPlay(EndPlayReason);
}

void UNartyPauseComponent::RemovePauseMapping()
{
	if (!bMappingAdded)
	{
		BoundLocalPlayer = nullptr;
		return;
	}

	if (ULocalPlayer* LP = BoundLocalPlayer.Get())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			if (PauseMappingContext)
			{
				Subsystem->RemoveMappingContext(PauseMappingContext);
			}
		}
	}

	BoundLocalPlayer = nullptr;
	bMappingAdded = false;
}

void UNartyPauseComponent::EnsurePauseInput()
{
	if (!PauseAction)
	{
		PauseAction = NewObject<UInputAction>(this, TEXT("IA_NartyPause"));
		PauseAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!PauseMappingContext)
	{
		PauseMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_NartyPause"));
		PauseMappingContext->MapKey(PauseAction, EKeys::Escape);
		PauseMappingContext->MapKey(PauseAction, EKeys::Gamepad_Special_Right);
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	APlayerController* PC = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	if (!PC)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UNartyPauseComponent::EnsurePauseInput));
		}
		return;
	}

	if (!bMappingAdded)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				Subsystem->AddMappingContext(PauseMappingContext, 3);
				BoundLocalPlayer = LP;
				bMappingAdded = true;
			}
		}
	}

	if (bInputBound)
	{
		return;
	}

	UInputComponent* IC = OwnerCharacter->InputComponent;
	if (!IC)
	{
		IC = PC->InputComponent;
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(IC))
	{
		EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &UNartyPauseComponent::HandlePauseStarted);
		bInputBound = true;
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyPauseComponent::EnsurePauseInput));
	}
}

void UNartyPauseComponent::HandlePauseStarted()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UNartyGameInstance* GI = World->GetGameInstance<UNartyGameInstance>())
	{
		GI->TogglePauseMenu();
	}
}
