#include "NartyUatsamongaCup.h"

#include "NartyChoiceDialogWidget.h"
#include "NartyGameInstance.h"
#include "NartyHeroTypes.h"
#include "NartyInteractComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ANartyUatsamongaCup::ANartyUatsamongaCup()
{
	PrimaryActorTick.bCanEverTick = true;

	Table = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Table"));
	SetRootComponent(Table);

	CupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CupMesh"));
	CupMesh->SetupAttachment(Table);
	CupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CupLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CupLight"));
	CupLight->SetupAttachment(CupMesh);
	CupLight->SetIntensity(2500.f);
	CupLight->SetLightColor(FLinearColor(0.7f, 0.75f, 0.85f));
	CupLight->SetAttenuationRadius(900.f);
	CupLight->SetRelativeLocation(FVector(0.f, 0.f, 60.f));

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Table);
	Trigger->SetBoxExtent(FVector(180.f, 180.f, 120.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Table);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
	Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Label->SetWorldSize(34.f);
	Label->SetTextRenderColor(FColor(220, 210, 160));
	Label->SetText(NSLOCTEXT("Narty", "Cup_Label", "\u0423\u0430\u0446\u0430\u043c\u043e\u043d\u0433\u0430"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CubeMesh.Succeeded())
	{
		Table->SetStaticMesh(CubeMesh.Object);
		Table->SetWorldScale3D(FVector(3.5f, 3.5f, 0.35f));
	}
	if (CylinderMesh.Succeeded())
	{
		CupMesh->SetStaticMesh(CylinderMesh.Object);
		CupMesh->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
		CupMesh->SetWorldScale3D(FVector(0.55f, 0.55f, 0.7f));
	}
	else if (CubeMesh.Succeeded())
	{
		CupMesh->SetStaticMesh(CubeMesh.Object);
		CupMesh->SetRelativeLocation(FVector(0.f, 0.f, 70.f));
		CupMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.8f));
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ANartyUatsamongaCup::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ANartyUatsamongaCup::OnCupOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &ANartyUatsamongaCup::OnCupEndOverlap);
}

void ANartyUatsamongaCup::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bBoiling || !CupMesh)
	{
		return;
	}

	BoilTime += DeltaSeconds;
	const float Pulse = 0.55f + 0.12f * FMath::Sin(BoilTime * 10.f);
	CupMesh->SetWorldScale3D(FVector(Pulse, Pulse, 0.7f + 0.15f * FMath::Sin(BoilTime * 8.f)));
	CupLight->SetIntensity(6000.f + 4000.f * FMath::Sin(BoilTime * 9.f));
}

void ANartyUatsamongaCup::ActivateForQuest()
{
	bQuestActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	Label->SetText(NSLOCTEXT("Narty", "Cup_Active", "\u0423\u0430\u0446\u0430\u043c\u043e\u043d\u0433\u0430 \u2014 \u043f\u043e\u0434\u043e\u0439\u0434\u0438"));
	Label->SetTextRenderColor(FColor(255, 220, 120));
	CupLight->SetIntensity(5000.f);
	CupLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.4f));
	UE_LOG(LogTemp, Warning, TEXT("Narty: Uatsamonga cup activated"));
}

void ANartyUatsamongaCup::PlayBoil(bool bBoil)
{
	bBoiling = bBoil;
	BoilTime = 0.f;
	if (bBoil)
	{
		CupLight->SetLightColor(FLinearColor(1.f, 0.55f, 0.1f));
		Label->SetText(NSLOCTEXT("Narty", "Cup_Boils", "\u0427\u0430\u0448\u0430 \u0432\u0441\u043a\u0438\u043f\u0430\u0435\u0442"));
		Label->SetTextRenderColor(FColor(255, 140, 40));
	}
	else
	{
		CupLight->SetIntensity(800.f);
		CupLight->SetLightColor(FLinearColor(0.35f, 0.4f, 0.5f));
		Label->SetText(NSLOCTEXT("Narty", "Cup_Cold", "\u0427\u0430\u0448\u0430 \u043c\u043e\u043b\u0447\u0438\u0442"));
		Label->SetTextRenderColor(FColor(140, 150, 170));
	}
}

void ANartyUatsamongaCup::OnCupOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bQuestActive || bJudged)
	{
		return;
	}

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
		OpenJudgment(Character);
	}
}

void ANartyUatsamongaCup::OnCupEndOverlap(
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

bool ANartyUatsamongaCup::CanNartyInteract() const
{
	return bQuestActive && !bJudged && InsideCharacter.IsValid() && !bDialogOpen;
}

void ANartyUatsamongaCup::TryNartyInteract(ACharacter* Character)
{
	if (!Character || !CanNartyInteract())
	{
		return;
	}
	OpenJudgment(Character);
}

void ANartyUatsamongaCup::OpenJudgment(ACharacter* Character)
{
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC || bDialogOpen || bJudged)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;
	ChoiceIsTruth.Reset();

	UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this));
	const ENartyHero Hero = GI ? GI->GetSelectedHero() : ENartyHero::None;

	FText Body = NSLOCTEXT("Narty", "Cup_Body",
		"\u041d\u044b\u0445\u0430\u0441 \u0441\u043c\u043e\u043b\u043a.\n\u0421\u043a\u0430\u0436\u0438 \u043e \u043f\u043e\u0434\u0432\u0438\u0433\u0435 \u0441 \u043e\u0433\u043d\u0451\u043c.\n\u0427\u0430\u0448\u0430 \u0432\u0441\u043a\u0438\u043f\u0430\u0435\u0442 \u043b\u0438\u0448\u044c \u043f\u0435\u0440\u0435\u0434 \u043f\u0440\u0430\u0432\u0434\u043e\u0439.");

	TArray<FText> Choices;
	switch (Hero)
	{
	case ENartyHero::Soslan:
		Choices.Add(NSLOCTEXT("Narty", "Cup_SoslanTruth", "\u042f \u0432\u0437\u044f\u043b \u043e\u0433\u043e\u043d\u044c \u0441\u043a\u043e\u0440\u043e\u0441\u0442\u044c\u044e \u0438 \u0445\u0438\u0442\u0440\u043e\u0441\u0442\u044c\u044e."));
		Choices.Add(NSLOCTEXT("Narty", "Cup_SoslanLie", "\u042f \u043e\u0434\u0438\u043d \u0440\u0430\u0437\u0431\u0438\u043b \u0432\u0441\u0435\u0445 \u0441\u0442\u0440\u0430\u0436\u0435\u0439 \u0433\u043e\u0440\u044b."));
		ChoiceIsTruth = { true, false };
		break;
	case ENartyHero::Batraz:
		Choices.Add(NSLOCTEXT("Narty", "Cup_BatrazTruth", "\u042f \u0441\u0440\u0430\u0437\u0438\u043b \u0441\u0442\u0440\u0430\u0436\u0435\u0439 \u0438 \u0432\u0437\u044f\u043b \u0436\u0430\u0440 \u0441\u0438\u043b\u043e\u0439."));
		Choices.Add(NSLOCTEXT("Narty", "Cup_BatrazLie", "\u0411\u043e\u0433\u0438 \u0441\u0430\u043c\u0438 \u043e\u0442\u0434\u0430\u043b\u0438 \u043c\u043d\u0435 \u043e\u0433\u043e\u043d\u044c."));
		ChoiceIsTruth = { true, false };
		break;
	case ENartyHero::Syrdon:
		Choices.Add(NSLOCTEXT("Narty", "Cup_SyrdonTruth", "\u042f \u0432\u044b\u043c\u0435\u043d\u044f\u043b \u043e\u0433\u043e\u043d\u044c \u0441\u043b\u043e\u0432\u043e\u043c."));
		Choices.Add(NSLOCTEXT("Narty", "Cup_SyrdonLie", "\u042f \u0441\u0438\u043b\u043e\u0439 \u043e\u0442\u043d\u044f\u043b \u043e\u0433\u043e\u043d\u044c \u0443 \u0441\u0442\u0440\u0430\u0436\u0435\u0439."));
		Choices.Add(NSLOCTEXT("Narty", "Cup_SyrdonTwist", "\u0427\u0430\u0448\u0430 \u0438 \u0442\u0430\u043a \u0437\u043d\u0430\u0435\u0442 \u043f\u0440\u0430\u0432\u0434\u0443 \u2014 \u044f \u043b\u0438\u0448\u044c \u0435\u0451 \u0433\u043e\u0441\u0442\u044c."));
		ChoiceIsTruth = { true, false, true }; // twist counts as truth (cunning honesty)
		break;
	default:
		Choices.Add(NSLOCTEXT("Narty", "Cup_DefaultTruth", "\u042f \u0432\u0435\u0440\u043d\u0443\u043b \u043e\u0433\u043e\u043d\u044c \u0441\u0435\u043b\u0435\u043d\u0438\u044e."));
		Choices.Add(NSLOCTEXT("Narty", "Cup_DefaultLie", "\u042f \u043f\u043e\u0431\u0435\u0434\u0438\u043b \u0431\u043e\u0433\u043e\u0432."));
		ChoiceIsTruth = { true, false };
		break;
	}

	if (!DialogWidget)
	{
		DialogWidget = CreateWidget<UNartyChoiceDialogWidget>(PC, UNartyChoiceDialogWidget::StaticClass());
		if (DialogWidget)
		{
			DialogWidget->OnChoicePicked.AddDynamic(this, &ANartyUatsamongaCup::HandleChoice);
			DialogWidget->OnClosed.AddDynamic(this, &ANartyUatsamongaCup::HandleClosed);
		}
	}

	if (DialogWidget)
	{
		DialogWidget->Setup(
			NSLOCTEXT("Narty", "Cup_Title", "\u0423\u0430\u0446\u0430\u043c\u043e\u043d\u0433\u0430"),
			Body,
			Choices);
		if (!DialogWidget->IsInViewport())
		{
			DialogWidget->AddToViewport(1300);
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
}

void ANartyUatsamongaCup::CloseDialog()
{
	if (DialogWidget)
	{
		DialogWidget->RemoveFromParent();
	}

	if (ACharacter* Character = InteractingCharacter.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly Mode;
			PC->SetInputMode(Mode);
			Character->EnableInput(PC);
		}
	}

	bDialogOpen = false;
}

void ANartyUatsamongaCup::HandleChoice(int32 ChoiceIndex)
{
	CloseDialog();
	ResolveChoice(ChoiceIndex);
}

void ANartyUatsamongaCup::HandleClosed()
{
	CloseDialog();
}

void ANartyUatsamongaCup::ResolveChoice(int32 ChoiceIndex)
{
	if (bJudged)
	{
		return;
	}

	bJudged = true;
	const bool bTruth = ChoiceIsTruth.IsValidIndex(ChoiceIndex) ? ChoiceIsTruth[ChoiceIndex] : false;
	PlayBoil(bTruth);

	if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->NotifyUatsamongaResolved(bTruth);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: Uatsamonga resolved truth=%d choice=%d"), bTruth ? 1 : 0, ChoiceIndex);
}
