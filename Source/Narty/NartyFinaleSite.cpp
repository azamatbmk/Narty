#include "NartyFinaleSite.h"

#include "NartyForgeDialogWidget.h"
#include "NartyChoiceDialogWidget.h"
#include "NartyGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ANartyFinaleSite::ANartyFinaleSite()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Root);
	Trigger->SetBoxExtent(FVector(200.f, 200.f, 140.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Root);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 280.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(40.f);
	Label->SetTextRenderColor(FColor(220, 200, 160));
	Label->SetText(NSLOCTEXT("Narty", "Finale_Idle", "\u0421\u0430\u0442\u0430\u043d\u0430"));

	SkyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetRelativeLocation(FVector(0.f, 0.f, 400.f));
	SkyLight->SetIntensity(0.f);
	SkyLight->SetAttenuationRadius(3000.f);
	SkyLight->SetLightColor(FLinearColor(0.7f, 0.55f, 1.f));

	PillarLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarLeft"));
	PillarLeft->SetupAttachment(Root);
	PillarRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PillarRight"));
	PillarRight->SetupAttachment(Root);
	Altar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Altar"));
	Altar->SetupAttachment(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
		PillarLeft->SetStaticMesh(CubeMesh);
		PillarRight->SetStaticMesh(CubeMesh);
		Altar->SetStaticMesh(CubeMesh);
		PillarLeft->SetRelativeLocation(FVector(-180.f, -120.f, 150.f));
		PillarLeft->SetWorldScale3D(FVector(0.7f, 0.7f, 4.f));
		PillarRight->SetRelativeLocation(FVector(-180.f, 120.f, 150.f));
		PillarRight->SetWorldScale3D(FVector(0.7f, 0.7f, 4.f));
		Altar->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
		Altar->SetWorldScale3D(FVector(2.5f, 2.5f, 0.5f));
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ANartyFinaleSite::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ANartyFinaleSite::OnSanctumOverlap);
}

void ANartyFinaleSite::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bStorm)
	{
		return;
	}

	StormTime += DeltaSeconds;
	const float Pulse = 8000.f + 6000.f * FMath::Sin(StormTime * 7.f);
	SkyLight->SetIntensity(Pulse);
}

void ANartyFinaleSite::ActivateFinale()
{
	bActive = true;
	bResolved = false;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	BuildSanctum();
	SkyLight->SetIntensity(7000.f);
	Label->SetText(NSLOCTEXT("Narty", "Finale_Active", "\u041a\u043e\u043d\u0435\u0446 \u043d\u0430\u0440\u0442\u043e\u0432"));
	Label->SetTextRenderColor(FColor(255, 210, 120));
	UE_LOG(LogTemp, Warning, TEXT("Narty: finale sanctum activated"));
}

void ANartyFinaleSite::BuildSanctum()
{
	// Visual already set in constructor; tint altar warm.
	if (Altar)
	{
		if (UMaterialInstanceDynamic* Dyn = Altar->CreateAndSetMaterialInstanceDynamic(0))
		{
			const FLinearColor Gold(0.55f, 0.4f, 0.15f);
			Dyn->SetVectorParameterValue(TEXT("Color"), Gold);
			Dyn->SetVectorParameterValue(TEXT("BaseColor"), Gold);
		}
	}
}

void ANartyFinaleSite::OnSanctumOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bActive || bResolved || bDialogOpen)
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

	OpenSatana(Character);
}

void ANartyFinaleSite::OpenSatana(ACharacter* Character)
{
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;

	UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this));
	const bool bDarker = GI && (!GI->ToldTruthAtCup() || !GI->ChoseNobleTrialPath());

	FText Body = bDarker
		? NSLOCTEXT("Narty", "Finale_SatanaDark",
			"\u0421\u0430\u0442\u0430\u043d\u0430:\n\u041c\u0438\u0440 \u0443\u0436\u0435 \u0442\u0440\u0435\u0449\u0438\u0442. \u0411\u043e\u0433\u0438 \u043e\u0442\u0432\u0435\u0440\u043d\u0443\u043b\u0438\u0441\u044c, \u0440\u043e\u0434\u044b \u0433\u043e\u0442\u043e\u0432\u044b \u043a \u0432\u043e\u0439\u043d\u0435.\n\u0427\u0442\u043e \u043e\u0441\u0442\u0430\u043d\u0435\u0442\u0441\u044f \u043f\u043e\u0441\u043b\u0435 \u043d\u0430\u0440\u0442\u043e\u0432?")
		: NSLOCTEXT("Narty", "Finale_SatanaLight",
			"\u0421\u0430\u0442\u0430\u043d\u0430:\n\u0422\u044b \u043f\u0440\u043e\u043d\u0451\u0441 \u043e\u0433\u043e\u043d\u044c \u0438 \u043f\u0440\u0430\u0432\u0434\u0443. \u041d\u043e \u0440\u043e\u0434 \u0432\u0441\u0451 \u0440\u0430\u0432\u043d\u043e \u043d\u0430 \u043a\u0440\u0430\u044e.\n\u0427\u0442\u043e \u043e\u0441\u0442\u0430\u043d\u0435\u0442\u0441\u044f \u043f\u043e\u0441\u043b\u0435 \u043d\u0430\u0440\u0442\u043e\u0432?");

	if (!SatanaWidget)
	{
		SatanaWidget = CreateWidget<UNartyForgeDialogWidget>(PC, UNartyForgeDialogWidget::StaticClass());
		if (SatanaWidget)
		{
			SatanaWidget->OnAccepted.AddDynamic(this, &ANartyFinaleSite::HandleSatanaAccepted);
			SatanaWidget->OnClosed.AddDynamic(this, &ANartyFinaleSite::HandleSatanaClosed);
		}
	}

	if (SatanaWidget)
	{
		SatanaWidget->SetDialogTexts(
			NSLOCTEXT("Narty", "Finale_SatanaTitle", "\u0421\u0430\u0442\u0430\u043d\u0430"),
			Body,
			NSLOCTEXT("Narty", "Finale_SatanaAccept", "\u042f \u0433\u043e\u0442\u043e\u0432"));
		if (!SatanaWidget->IsInViewport())
		{
			SatanaWidget->AddToViewport(1600);
		}
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (SatanaWidget)
	{
		Mode.SetWidgetToFocus(SatanaWidget->TakeWidget());
	}
	PC->SetInputMode(Mode);
	Character->DisableInput(PC);
}

void ANartyFinaleSite::HandleSatanaAccepted()
{
	CloseDialogs();
	if (ACharacter* Character = InteractingCharacter.Get())
	{
		OpenEndingChoice(Character);
	}
}

void ANartyFinaleSite::HandleSatanaClosed()
{
	CloseDialogs();
}

void ANartyFinaleSite::OpenEndingChoice(ACharacter* Character)
{
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC || bResolved)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;

	UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this));
	const ENartyHero Hero = GI ? GI->GetSelectedHero() : ENartyHero::None;

	FText Title = NSLOCTEXT("Narty", "Finale_ChoiceTitle", "\u041a\u043e\u043d\u0435\u0446 \u043d\u0430\u0440\u0442\u043e\u0432");
	FText Body;
	TArray<FText> Choices;

	switch (Hero)
	{
	case ENartyHero::Soslan:
		Body = NSLOCTEXT("Narty", "Finale_SoslanBody",
			"\u041a\u043e\u043b\u0435\u0441\u043e \u0411\u0430\u043b\u0441\u0430\u0433\u0430 \u0443\u0436\u0435 \u043a\u0430\u0442\u0438\u0442\u0441\u044f.\n\u0421\u043f\u0430\u0441\u0442\u0438 \u0441\u0435\u043b\u0435\u043d\u0438\u0435 \u2014 \u0438\u043b\u0438 \u0443\u043a\u043b\u043e\u043d\u0438\u0441\u044c \u0438 \u0436\u0438\u0432\u0438.");
		Choices.Add(NSLOCTEXT("Narty", "Finale_SoslanA", "\u0412\u0441\u0442\u0440\u0435\u0442\u0438\u0442\u044c \u043a\u043e\u043b\u0435\u0441\u043e \u2014 \u0441\u043f\u0430\u0441\u0442\u0438 \u0440\u043e\u0434 \u0446\u0435\u043d\u043e\u0439 \u0441\u0435\u0431\u044f."));
		Choices.Add(NSLOCTEXT("Narty", "Finale_SoslanB", "\u0423\u043a\u043b\u043e\u043d\u0438\u0442\u044c\u0441\u044f \u2014 \u0436\u0438\u0442\u044c, \u043d\u043e \u0432 \u043f\u043e\u0437\u043e\u0440\u0435."));
		break;
	case ENartyHero::Batraz:
		Body = NSLOCTEXT("Narty", "Finale_BatrazBody",
			"\u041d\u0435\u0431\u043e \u043c\u043e\u043b\u0447\u0438\u0442. \u041c\u0435\u0447 \u0433\u043e\u0440\u0438\u0442 \u0432 \u0440\u0443\u043a\u0435.\n\u0427\u0442\u043e \u0441\u0434\u0435\u043b\u0430\u0435\u0442 \u0436\u0435\u043b\u0435\u0437\u043d\u044b\u0439 \u043d\u0430\u0440\u0442?");
		Choices.Add(NSLOCTEXT("Narty", "Finale_BatrazA", "\u0411\u0440\u043e\u0441\u0438\u0442\u044c \u043c\u0435\u0447 \u0432 \u043c\u043e\u0440\u0435 \u2014 \u0434\u0430\u0442\u044c \u0440\u043e\u0434\u0443 \u043e\u0442\u0441\u0440\u043e\u0447\u043a\u0443."));
		Choices.Add(NSLOCTEXT("Narty", "Finale_BatrazB", "\u0423\u0434\u0430\u0440\u0438\u0442\u044c \u043f\u043e \u043d\u0435\u0431\u043e\u0436\u0438\u0442\u0435\u043b\u044f\u043c \u2014 \u043f\u0443\u0441\u0442\u044c \u043c\u043e\u0440\u0435 \u0432\u0441\u043a\u0438\u043f\u0438\u0442."));
		break;
	case ENartyHero::Syrdon:
		Body = NSLOCTEXT("Narty", "Finale_SyrdonBody",
			"\u041d\u044b\u0445\u0430\u0441 \u0436\u0434\u0451\u0442 \u043f\u043e\u0441\u043b\u0435\u0434\u043d\u0435\u0433\u043e \u0441\u043b\u043e\u0432\u0430.\n\u041f\u0440\u0430\u0432\u0434\u0430 \u0434\u0430\u0441\u0442 \u0445\u0440\u0443\u043f\u043a\u0438\u0439 \u043c\u0438\u0440. \u041b\u043e\u0436\u044c \u2014 \u0441\u043b\u0430\u0432\u0443 \u043d\u0430 \u043f\u0435\u043f\u043b\u0435.");
		Choices.Add(NSLOCTEXT("Narty", "Finale_SyrdonA", "\u0421\u043a\u0430\u0437\u0430\u0442\u044c \u043f\u0440\u0430\u0432\u0434\u0443 \u2014 \u0445\u0440\u0443\u043f\u043a\u0438\u0439 \u043c\u0438\u0440."));
		Choices.Add(NSLOCTEXT("Narty", "Finale_SyrdonB", "\u0421\u043a\u0430\u0437\u0430\u0442\u044c \u043b\u043e\u0436\u044c \u2014 \u0441\u043b\u0430\u0432\u0430 \u043d\u0430 \u043e\u0431\u043c\u0430\u043d\u0435."));
		break;
	default:
		Body = NSLOCTEXT("Narty", "Finale_DefaultBody", "\u0412\u044b\u0431\u0435\u0440\u0438 \u0441\u0443\u0434\u044c\u0431\u0443 \u0440\u043e\u0434\u0430.");
		Choices.Add(NSLOCTEXT("Narty", "Finale_DefaultA", "A"));
		Choices.Add(NSLOCTEXT("Narty", "Finale_DefaultB", "B"));
		break;
	}

	if (!EndingWidget)
	{
		EndingWidget = CreateWidget<UNartyChoiceDialogWidget>(PC, UNartyChoiceDialogWidget::StaticClass());
		if (EndingWidget)
		{
			EndingWidget->OnChoicePicked.AddDynamic(this, &ANartyFinaleSite::HandleEndingChoice);
			EndingWidget->OnClosed.AddDynamic(this, &ANartyFinaleSite::HandleEndingClosed);
		}
	}

	if (EndingWidget)
	{
		EndingWidget->Setup(Title, Body, Choices);
		if (!EndingWidget->IsInViewport())
		{
			EndingWidget->AddToViewport(1700);
		}
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (EndingWidget)
	{
		Mode.SetWidgetToFocus(EndingWidget->TakeWidget());
	}
	PC->SetInputMode(Mode);
	Character->DisableInput(PC);
}

void ANartyFinaleSite::CloseDialogs()
{
	if (SatanaWidget)
	{
		SatanaWidget->RemoveFromParent();
	}
	if (EndingWidget)
	{
		EndingWidget->RemoveFromParent();
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

void ANartyFinaleSite::HandleEndingChoice(int32 ChoiceIndex)
{
	CloseDialogs();
	ResolveEnding(ChoiceIndex);
}

void ANartyFinaleSite::HandleEndingClosed()
{
	CloseDialogs();
}

void ANartyFinaleSite::ResolveEnding(int32 ChoiceIndex)
{
	if (bResolved)
	{
		return;
	}

	bResolved = true;

	UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this));
	const ENartyHero Hero = GI ? GI->GetSelectedHero() : ENartyHero::None;
	ENartyEnding Ending = ENartyEnding::None;

	switch (Hero)
	{
	case ENartyHero::Soslan:
		Ending = (ChoiceIndex == 0) ? ENartyEnding::SoslanSacrifice : ENartyEnding::SoslanShame;
		break;
	case ENartyHero::Batraz:
		Ending = (ChoiceIndex == 0) ? ENartyEnding::BatrazMercy : ENartyEnding::BatrazStorm;
		break;
	case ENartyHero::Syrdon:
		Ending = (ChoiceIndex == 0) ? ENartyEnding::SyrdonPeace : ENartyEnding::SyrdonLie;
		break;
	default:
		break;
	}

	PlayEndingVisual(Ending);

	if (GI)
	{
		GI->NotifyFinaleCompleted(Ending);
	}
}

void ANartyFinaleSite::PlayEndingVisual(ENartyEnding Ending)
{
	switch (Ending)
	{
	case ENartyEnding::SoslanSacrifice:
		SkyLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.3f));
		SkyLight->SetIntensity(18000.f);
		Label->SetText(NSLOCTEXT("Narty", "End_SoslanA", "\u041a\u043e\u043b\u0435\u0441\u043e \u043f\u0440\u0438\u043d\u044f\u0442\u043e"));
		break;
	case ENartyEnding::SoslanShame:
		SkyLight->SetLightColor(FLinearColor(0.4f, 0.4f, 0.45f));
		SkyLight->SetIntensity(2500.f);
		Label->SetText(NSLOCTEXT("Narty", "End_SoslanB", "\u041f\u043e\u0437\u043e\u0440 \u0438 \u0436\u0438\u0437\u043d\u044c"));
		break;
	case ENartyEnding::BatrazMercy:
		SkyLight->SetLightColor(FLinearColor(0.4f, 0.7f, 1.f));
		SkyLight->SetIntensity(12000.f);
		Label->SetText(NSLOCTEXT("Narty", "End_BatrazA", "\u041c\u0435\u0447 \u0443\u0448\u0451\u043b \u0432 \u043c\u043e\u0440\u0435"));
		break;
	case ENartyEnding::BatrazStorm:
		bStorm = true;
		SkyLight->SetLightColor(FLinearColor(0.7f, 0.2f, 1.f));
		Label->SetText(NSLOCTEXT("Narty", "End_BatrazB", "\u041d\u0435\u0431\u043e \u0432 \u044f\u0440\u043e\u0441\u0442\u0438"));
		break;
	case ENartyEnding::SyrdonPeace:
		SkyLight->SetLightColor(FLinearColor(0.45f, 0.9f, 0.55f));
		SkyLight->SetIntensity(10000.f);
		Label->SetText(NSLOCTEXT("Narty", "End_SyrdonA", "\u0425\u0440\u0443\u043f\u043a\u0438\u0439 \u043c\u0438\u0440"));
		break;
	case ENartyEnding::SyrdonLie:
		SkyLight->SetLightColor(FLinearColor(0.85f, 0.55f, 0.2f));
		SkyLight->SetIntensity(9000.f);
		Label->SetText(NSLOCTEXT("Narty", "End_SyrdonB", "\u0421\u043b\u0430\u0432\u0430 \u043d\u0430 \u043b\u0436\u0438"));
		break;
	default:
		break;
	}
}
