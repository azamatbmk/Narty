#include "NartyForgeActor.h"

#include "NartyForgeDialogWidget.h"
#include "NartyCombatComponent.h"
#include "NartyGameInstance.h"
#include "NartyInteractComponent.h"
#include "NartyHeroTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ANartyForgeActor::ANartyForgeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	SetRootComponent(BaseMesh);

	AnvilMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AnvilMesh"));
	AnvilMesh->SetupAttachment(BaseMesh);

	ForgeLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ForgeLight"));
	ForgeLight->SetupAttachment(BaseMesh);
	ForgeLight->SetIntensity(8000.f);
	ForgeLight->SetLightColor(FLinearColor(1.f, 0.35f, 0.05f));
	ForgeLight->SetAttenuationRadius(1200.f);
	ForgeLight->SetRelativeLocation(FVector(0.f, 0.f, 180.f));

	InteractTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractTrigger"));
	InteractTrigger->SetupAttachment(BaseMesh);
	InteractTrigger->SetBoxExtent(FVector(180.f, 180.f, 120.f));
	InteractTrigger->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	InteractTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(BaseMesh);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Label->SetWorldSize(42.f);
	Label->SetTextRenderColor(FColor(255, 200, 120));
	Label->SetText(NSLOCTEXT("Narty", "Forge_Label", "\u041a\u0443\u0440\u0434\u0430\u043b\u0430\u0433\u043e\u043d"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		BaseMesh->SetStaticMesh(CubeMesh.Object);
		BaseMesh->SetWorldScale3D(FVector(2.2f, 2.2f, 0.6f));
		AnvilMesh->SetStaticMesh(CubeMesh.Object);
		AnvilMesh->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
		AnvilMesh->SetWorldScale3D(FVector(1.2f, 0.8f, 0.7f));
	}
}

void ANartyForgeActor::BeginPlay()
{
	Super::BeginPlay();
	InteractTrigger->OnComponentBeginOverlap.AddDynamic(this, &ANartyForgeActor::OnForgeOverlap);
	InteractTrigger->OnComponentEndOverlap.AddDynamic(this, &ANartyForgeActor::OnForgeEndOverlap);
}

void ANartyForgeActor::OnForgeOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ACharacter* Character = Cast<ACharacter>(OtherActor);
	if (!Character)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	InsideCharacter = Character;
	if (UNartyInteractComponent* Interact = UNartyInteractComponent::EnsureOn(Character))
	{
		Interact->PushInteractTarget(this);
	}

	if (!bDialogOpen)
	{
		OpenDialog(Character);
	}
}

void ANartyForgeActor::OnForgeEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor == InsideCharacter.Get())
	{
		InsideCharacter = nullptr;
	}

	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		if (UNartyInteractComponent* Interact = Character->FindComponentByClass<UNartyInteractComponent>())
		{
			Interact->PopInteractTarget(this);
		}
	}
}

bool ANartyForgeActor::CanNartyInteract() const
{
	return InsideCharacter.IsValid() && !bDialogOpen;
}

void ANartyForgeActor::TryNartyInteract(ACharacter* Character)
{
	if (!Character || bDialogOpen)
	{
		return;
	}
	OpenDialog(Character);
}

void ANartyForgeActor::OpenDialog(ACharacter* Character)
{
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC || bDialogOpen)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;

	if (!IsValid(DialogWidget))
	{
		DialogWidget = CreateWidget<UNartyForgeDialogWidget>(PC, UNartyForgeDialogWidget::StaticClass());
		if (DialogWidget)
		{
			DialogWidget->OnAccepted.AddDynamic(this, &ANartyForgeActor::HandleDialogAccepted);
			DialogWidget->OnClosed.AddDynamic(this, &ANartyForgeActor::HandleDialogClosed);
		}
	}

	FText Body;
	FText Accept;
	if (bWeaponGranted)
	{
		Body = NSLOCTEXT("Narty", "Forge_BodyDone",
			"\u041e\u0433\u043e\u043d\u044c \u0443\u0436\u0435 \u0432 \u0442\u0432\u043e\u0451\u043c \u043a\u043b\u0438\u043d\u043a\u0435.\n\u0418\u0434\u0438 \u0438 \u0434\u043e\u043a\u0430\u0436\u0438, \u0447\u0442\u043e \u0434\u043e\u0441\u0442\u043e\u0438\u043d \u043d\u0430\u0440\u0442\u043e\u0432.");
		Accept = NSLOCTEXT("Narty", "Forge_AcceptDone", "\u041f\u043e\u043d\u044f\u043b");
	}
	else
	{
		ENartyHero Hero = ENartyHero::None;
		if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			Hero = GI->GetSelectedHero();
		}

		switch (Hero)
		{
		case ENartyHero::Soslan:
			Body = NSLOCTEXT("Narty", "Forge_BodySoslan",
				"\u0421\u043e\u0441\u043b\u0430\u043d...\n\u042f \u0432\u044b\u0441\u0435\u043a \u0442\u0435\u0431\u044f \u0438\u0437 \u043a\u0430\u043c\u043d\u044f. \u0412\u043e\u0437\u044c\u043c\u0438 \u0436\u0430\u0440 \u0432 \u0440\u0443\u043a\u0443 \u2014 \u0438 \u043d\u0435 \u043f\u043e\u0437\u043e\u0440\u044c \u0440\u043e\u0434.");
			break;
		case ENartyHero::Batraz:
			Body = NSLOCTEXT("Narty", "Forge_BodyBatraz",
				"\u0411\u0430\u0442\u0440\u0430\u0437...\n\u0416\u0435\u043b\u0435\u0437\u043e \u0437\u043d\u0430\u0435\u0442 \u0442\u0432\u043e\u044e \u043a\u0440\u043e\u0432\u044c. \u0412\u043e\u0437\u044c\u043c\u0438 \u043a\u043b\u0438\u043d\u043e\u043a \u2014 \u043f\u0443\u0441\u0442\u044c \u043d\u0435\u0431\u043e \u0443\u0441\u043b\u044b\u0448\u0438\u0442 \u0443\u0434\u0430\u0440.");
			break;
		case ENartyHero::Syrdon:
			Body = NSLOCTEXT("Narty", "Forge_BodySyrdon",
				"\u0421\u044b\u0440\u0434\u043e\u043d...\n\u0421\u043b\u043e\u0432\u043e \u043e\u0441\u0442\u0440\u0435\u0435 \u043a\u043b\u0438\u043d\u043a\u0430. \u041d\u043e \u0438 \u043a\u043b\u0438\u043d\u043e\u043a \u0442\u0435\u0431\u0435 \u043f\u0440\u0438\u0433\u043e\u0434\u0438\u0442\u0441\u044f, \u043a\u043e\u0433\u0434\u0430 \u043f\u0440\u0430\u0432\u0434\u0430 \u043d\u0435 \u043f\u043e\u043c\u043e\u0436\u0435\u0442.");
			break;
		default:
			Body = NSLOCTEXT("Narty", "Forge_BodyDefault2",
				"\u041d\u0430\u0440\u0442...\n\u041a\u0443\u0437\u043d\u0438\u0446\u0430 \u0436\u0434\u0451\u0442. \u0412\u043e\u0437\u044c\u043c\u0438 \u043e\u0440\u0443\u0436\u0438\u0435.");
			break;
		}
		Accept = NSLOCTEXT("Narty", "Forge_AcceptTake", "\u0412\u0437\u044f\u0442\u044c \u043e\u0440\u0443\u0436\u0438\u0435");
	}

	if (DialogWidget)
	{
		DialogWidget->SetDialogTexts(
			NSLOCTEXT("Narty", "Forge_Title", "\u041a\u0443\u0440\u0434\u0430\u043b\u0430\u0433\u043e\u043d"),
			Body,
			Accept);

		if (!DialogWidget->IsInViewport())
		{
			DialogWidget->AddToViewport(1100);
		}
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (DialogWidget)
	{
		InputMode.SetWidgetToFocus(DialogWidget->TakeWidget());
	}
	PC->SetInputMode(InputMode);
	Character->DisableInput(PC);
	UNartyInteractComponent::NotifyPromptChanged(Character);
}

void ANartyForgeActor::CloseDialog()
{
	if (IsValid(DialogWidget))
	{
		DialogWidget->RemoveFromParent();
	}

	if (ACharacter* Character = InteractingCharacter.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			Character->EnableInput(PC);
		}
	}

	bDialogOpen = false;
	UNartyInteractComponent::NotifyPromptChanged(InteractingCharacter.Get());
}

void ANartyForgeActor::HandleDialogAccepted()
{
	if (!bWeaponGranted)
	{
		ACharacter* Character = InteractingCharacter.Get();
		if (!Character)
		{
			CloseDialog();
			return;
		}

		UNartyCombatComponent* Combat = Character->FindComponentByClass<UNartyCombatComponent>();
		if (!Combat)
		{
			Combat = NewObject<UNartyCombatComponent>(Character, TEXT("NartyCombat"));
			Combat->RegisterComponent();
			Character->AddInstanceComponent(Combat);
			if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
			{
				Combat->SetHero(GI->GetSelectedHero());
			}
		}

		Combat->GrantForgeWeapon();
		bWeaponGranted = true;
		Label->SetText(NSLOCTEXT("Narty", "Forge_LabelDone", "\u041a\u0443\u0440\u0434\u0430\u043b\u0430\u0433\u043e\u043d \u2713"));
		ForgeLight->SetLightColor(FLinearColor(0.3f, 0.85f, 1.f));

		if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			GI->StartFireQuest();
		}
	}

	CloseDialog();
}

void ANartyForgeActor::HandleDialogClosed()
{
	CloseDialog();
}
