#include "NartyInteractComponent.h"

#include "NartyInteractable.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

UNartyInteractComponent::UNartyInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UNartyInteractComponent* UNartyInteractComponent::EnsureOn(ACharacter* Character)
{
	if (!Character)
	{
		return nullptr;
	}

	if (UNartyInteractComponent* Existing = Character->FindComponentByClass<UNartyInteractComponent>())
	{
		return Existing;
	}

	UNartyInteractComponent* Comp = NewObject<UNartyInteractComponent>(Character, TEXT("NartyInteract"));
	Comp->RegisterComponent();
	Character->AddInstanceComponent(Comp);
	Comp->EnsureInput();
	return Comp;
}

void UNartyInteractComponent::BeginPlay()
{
	Super::BeginPlay();
	EnsureInput();
}

void UNartyInteractComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveInteractMapping();
	InteractFocusStack.Reset();
	bInputBound = false;
	Super::EndPlay(EndPlayReason);
}

void UNartyInteractComponent::RemoveInteractMapping()
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
			if (InteractMappingContext)
			{
				Subsystem->RemoveMappingContext(InteractMappingContext);
			}
		}
	}

	BoundLocalPlayer = nullptr;
	bMappingAdded = false;
}

void UNartyInteractComponent::EnsureInput()
{
	if (!InteractAction)
	{
		InteractAction = NewObject<UInputAction>(this, TEXT("IA_NartyInteract"));
		InteractAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!InteractMappingContext)
	{
		InteractMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_NartyInteract"));
		InteractMappingContext->MapKey(InteractAction, EKeys::E);
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	APlayerController* PC = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	if (!bMappingAdded)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				Subsystem->AddMappingContext(InteractMappingContext, 1);
				BoundLocalPlayer = LP;
				bMappingAdded = true;
			}
		}
	}

	if (bInputBound || !InteractAction)
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
		EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &UNartyInteractComponent::HandleInteractStarted);
		bInputBound = true;
	}
}

void UNartyInteractComponent::PushInteractTarget(AActor* Interactable)
{
	if (!Interactable || !Interactable->GetClass()->ImplementsInterface(UNartyInteractable::StaticClass()))
	{
		return;
	}

	EnsureInput();

	InteractFocusStack.RemoveAll([Interactable](const TWeakObjectPtr<AActor>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == Interactable;
	});
	InteractFocusStack.Add(Interactable);
}

void UNartyInteractComponent::PopInteractTarget(AActor* Interactable)
{
	InteractFocusStack.RemoveAll([Interactable](const TWeakObjectPtr<AActor>& Ptr)
	{
		return !Ptr.IsValid() || Ptr.Get() == Interactable;
	});
}

void UNartyInteractComponent::HandleInteractStarted()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	for (int32 i = InteractFocusStack.Num() - 1; i >= 0; --i)
	{
		AActor* Target = InteractFocusStack[i].Get();
		if (!Target)
		{
			InteractFocusStack.RemoveAt(i);
			continue;
		}

		if (INartyInteractable* Interactable = Cast<INartyInteractable>(Target))
		{
			if (Interactable->CanNartyInteract())
			{
				Interactable->TryNartyInteract(OwnerCharacter);
				return;
			}
		}
	}
}
