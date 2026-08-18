#include "NartyHeroTrialSite.h"

#include "NartyTrainingDummy.h"
#include "NartyForgeDialogWidget.h"
#include "NartyChoiceDialogWidget.h"
#include "NartyGameInstance.h"
#include "NartyInteractComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ANartyHeroTrialSite::ANartyHeroTrialSite()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	GateTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("GateTrigger"));
	GateTrigger->SetupAttachment(Root);
	GateTrigger->SetBoxExtent(FVector(140.f, 140.f, 120.f));
	GateTrigger->SetRelativeLocation(FVector(-200.f, 0.f, 80.f));
	GateTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	GateLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("GateLabel"));
	GateLabel->SetupAttachment(Root);
	GateLabel->SetRelativeLocation(FVector(-200.f, 0.f, 220.f));
	GateLabel->SetHorizontalAlignment(EHTA_Center);
	GateLabel->SetWorldSize(32.f);
	GateLabel->SetTextRenderColor(FColor(180, 180, 200));
	GateLabel->SetText(NSLOCTEXT("Narty", "Trial_GateIdle", "\u041f\u0443\u0442\u044c \u0433\u0435\u0440\u043e\u044f"));

	AtmosphereLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AtmosphereLight"));
	AtmosphereLight->SetupAttachment(Root);
	AtmosphereLight->SetRelativeLocation(FVector(600.f, 0.f, 300.f));
	AtmosphereLight->SetIntensity(0.f);
	AtmosphereLight->SetAttenuationRadius(2500.f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

FVector ANartyHeroTrialSite::GetGateWorldLocation() const
{
	return GetActorTransform().TransformPosition(FVector(-200.f, 0.f, 80.f));
}

void ANartyHeroTrialSite::BeginPlay()
{
	Super::BeginPlay();
	GateTrigger->OnComponentBeginOverlap.AddDynamic(this, &ANartyHeroTrialSite::OnGateOverlap);
	GateTrigger->OnComponentEndOverlap.AddDynamic(this, &ANartyHeroTrialSite::OnGateEndOverlap);
}

void ANartyHeroTrialSite::ActivateForHero(ENartyHero Hero)
{
	ClearTrialDummies();

	ActiveHero = Hero;
	bActive = true;
	bEntered = false;
	bCompleted = false;
	VisionsSeen = 0;
	SyrdonDeals = 0;

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	BuildSharedShell();

	switch (Hero)
	{
	case ENartyHero::Soslan:
		SetupSoslanTrial();
		GateLabel->SetText(NSLOCTEXT("Narty", "Trial_SoslanGate", "\u0426\u0430\u0440\u0441\u0442\u0432\u043e \u043c\u0451\u0440\u0442\u0432\u044b\u0445"));
		GateLabel->SetTextRenderColor(FColor(140, 180, 255));
		AtmosphereLight->SetLightColor(FLinearColor(0.35f, 0.45f, 1.f));
		break;
	case ENartyHero::Batraz:
		SetupBatrazTrial();
		GateLabel->SetText(NSLOCTEXT("Narty", "Trial_BatrazGate", "\u041a\u0440\u043e\u0432\u044c \u0440\u043e\u0434\u0430"));
		GateLabel->SetTextRenderColor(FColor(255, 90, 70));
		AtmosphereLight->SetLightColor(FLinearColor(1.f, 0.25f, 0.15f));
		break;
	case ENartyHero::Syrdon:
		SetupSyrdonTrial();
		GateLabel->SetText(NSLOCTEXT("Narty", "Trial_SyrdonGate", "\u0428\u0451\u043f\u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430"));
		GateLabel->SetTextRenderColor(FColor(120, 220, 140));
		AtmosphereLight->SetLightColor(FLinearColor(0.3f, 0.85f, 0.45f));
		break;
	default:
		break;
	}

	AtmosphereLight->SetIntensity(9000.f);
	UE_LOG(LogTemp, Warning, TEXT("Narty: hero trial activated for hero=%d"), static_cast<int32>(Hero));
}

void ANartyHeroTrialSite::ClearTrialDummies()
{
	for (TObjectPtr<ANartyTrainingDummy>& Dummy : TrialDummies)
	{
		if (IsValid(Dummy))
		{
			Dummy->Destroy();
		}
	}
	TrialDummies.Empty();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BatrazCheckHandle);
	}
}

void ANartyHeroTrialSite::CaptureSaveState(FNartyTrialSaveState& OutState) const
{
	if (!bActive && !bCompleted)
	{
		return;
	}

	OutState.bHasData = true;
	OutState.bEntered = bEntered;
	OutState.bAwaitingFinalChoice = bAwaitingFinalChoice;
	OutState.VisionsSeen = VisionsSeen;
	OutState.SyrdonDeals = SyrdonDeals;
	OutState.DummyHealth.Reset();
	for (const TObjectPtr<ANartyTrainingDummy>& Dummy : TrialDummies)
	{
		OutState.DummyHealth.Add(IsValid(Dummy) ? Dummy->GetHealth() : 0.f);
	}
}

void ANartyHeroTrialSite::DisableVisionTrigger(int32 VisionIndex)
{
	const FName TagName(*FString::Printf(TEXT("Vision_%d"), VisionIndex));
	for (TObjectPtr<UBoxComponent>& Trigger : VisionTriggers)
	{
		if (IsValid(Trigger) && Trigger->ComponentTags.Contains(TagName))
		{
			Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			return;
		}
	}
}

void ANartyHeroTrialSite::ApplyDummyHealthFromSave(const TArray<float>& HealthValues)
{
	for (int32 Index = 0; Index < TrialDummies.Num(); ++Index)
	{
		ANartyTrainingDummy* Dummy = TrialDummies[Index].Get();
		if (!IsValid(Dummy) || !HealthValues.IsValidIndex(Index))
		{
			continue;
		}

		const float SavedHealth = HealthValues[Index];
		if (SavedHealth <= 0.f)
		{
			Dummy->ReceiveStrike(99999.f, this);
		}
		else
		{
			Dummy->SetMaxHealth(160.f, false);
			Dummy->SetHealth(SavedHealth);
		}
	}
}

void ANartyHeroTrialSite::ApplyTrialProgress(const FNartyTrialSaveState& State)
{
	if (!State.bHasData)
	{
		return;
	}

	bEntered = State.bEntered;
	bAwaitingFinalChoice = State.bAwaitingFinalChoice;
	VisionsSeen = State.VisionsSeen;
	SyrdonDeals = State.SyrdonDeals;

	if (ActiveHero == ENartyHero::Soslan)
	{
		for (int32 Index = 0; Index < State.VisionsSeen; ++Index)
		{
			DisableVisionTrigger(Index);
		}
	}
	else if (ActiveHero == ENartyHero::Syrdon)
	{
		if (State.SyrdonDeals >= 1)
		{
			DisableVisionTrigger(10);
		}
		if (State.SyrdonDeals >= 2)
		{
			DisableVisionTrigger(11);
		}
	}

	ApplyDummyHealthFromSave(State.DummyHealth);

	if (bEntered)
	{
		if (bAwaitingFinalChoice)
		{
			GateLabel->SetText(NSLOCTEXT("Narty", "Trial_ChooseE", "E \u2014 \u0432\u044b\u0431\u043e\u0440"));
		}
		else
		{
			GateLabel->SetText(NSLOCTEXT("Narty", "Trial_Inside", "\u0418\u0441\u043f\u044b\u0442\u0430\u043d\u0438\u0435"));
		}

		if (ActiveHero == ENartyHero::Batraz && !bCompleted)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					BatrazCheckHandle,
					this,
					&ANartyHeroTrialSite::CheckBatrazCleared,
					0.5f,
					true);
			}
		}
	}
}

void ANartyHeroTrialSite::RestoreFromSave(
	ENartyHero Hero, bool bInActive, bool bCompletedState, const FNartyTrialSaveState& State)
{
	if (Hero == ENartyHero::None)
	{
		return;
	}

	if (bCompletedState)
	{
		ActiveHero = Hero;
		bCompleted = true;
		bActive = false;
		bEntered = true;
		bAwaitingFinalChoice = false;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		BuildSharedShell();
		GateLabel->SetText(NSLOCTEXT("Narty", "Trial_Done", "\u041f\u0443\u0442\u044c \u043f\u0440\u043e\u0439\u0434\u0435\u043d"));
		GateLabel->SetTextRenderColor(FColor(140, 200, 120));
		AtmosphereLight->SetIntensity(5000.f);
	}
	else if (bInActive)
	{
		ActivateForHero(Hero);
		ApplyTrialProgress(State);
	}
}

void ANartyHeroTrialSite::ResetForNewRun()
{
	CloseAnyDialog();
	ClearTrialDummies();

	ActiveHero = ENartyHero::None;
	bActive = false;
	bEntered = false;
	bCompleted = false;
	bDialogOpen = false;
	bAwaitingFinalChoice = false;
	VisionsSeen = 0;
	SyrdonDeals = 0;
	InteractingCharacter = nullptr;
	InsideGateCharacter = nullptr;

	for (TObjectPtr<UBoxComponent>& Trigger : VisionTriggers)
	{
		if (IsValid(Trigger))
		{
			Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	if (GateLabel)
	{
		GateLabel->SetText(NSLOCTEXT("Narty", "Trial_GateIdle", "\u041f\u0443\u0442\u044c \u0433\u0435\u0440\u043e\u044f"));
		GateLabel->SetTextRenderColor(FColor(180, 180, 200));
	}
	if (AtmosphereLight)
	{
		AtmosphereLight->SetIntensity(0.f);
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ANartyHeroTrialSite::BuildSharedShell()
{
	if (bShellBuilt)
	{
		return;
	}
	bShellBuilt = true;

	AddBlock(TEXT("TrialFloor"), FVector(700.f, 0.f, -40.f), FVector(18.f, 8.f, 1.f), FLinearColor(0.08f, 0.08f, 0.1f));
	AddBlock(TEXT("TrialWallL"), FVector(700.f, -420.f, 300.f), FVector(18.f, 1.2f, 8.f), FLinearColor(0.12f, 0.12f, 0.14f));
	AddBlock(TEXT("TrialWallR"), FVector(700.f, 420.f, 300.f), FVector(18.f, 1.2f, 8.f), FLinearColor(0.12f, 0.12f, 0.14f));
	AddBlock(TEXT("TrialEnd"), FVector(1500.f, 0.f, 300.f), FVector(1.2f, 9.f, 8.f), FLinearColor(0.1f, 0.1f, 0.12f));
	AddBlock(TEXT("GateArchL"), FVector(-120.f, -120.f, 120.f), FVector(0.6f, 0.6f, 3.f), FLinearColor(0.2f, 0.2f, 0.25f));
	AddBlock(TEXT("GateArchR"), FVector(-120.f, 120.f, 120.f), FVector(0.6f, 0.6f, 3.f), FLinearColor(0.2f, 0.2f, 0.25f));
}

void ANartyHeroTrialSite::SetupSoslanTrial()
{
	AddBlock(TEXT("VisionPillar1"), FVector(300.f, -150.f, 80.f), FVector(0.7f, 0.7f, 2.2f), FLinearColor(0.25f, 0.35f, 0.7f));
	AddBlock(TEXT("VisionPillar2"), FVector(600.f, 160.f, 80.f), FVector(0.7f, 0.7f, 2.2f), FLinearColor(0.25f, 0.35f, 0.7f));
	AddBlock(TEXT("VisionPillar3"), FVector(950.f, -40.f, 80.f), FVector(0.7f, 0.7f, 2.2f), FLinearColor(0.25f, 0.35f, 0.7f));

	AddTrigger(TEXT("Vision1"), FVector(300.f, -150.f, 60.f), FVector(100.f, 100.f, 90.f), 0);
	AddTrigger(TEXT("Vision2"), FVector(600.f, 160.f, 60.f), FVector(100.f, 100.f, 90.f), 1);
	AddTrigger(TEXT("Vision3"), FVector(950.f, -40.f, 60.f), FVector(100.f, 100.f, 90.f), 2);
}

void ANartyHeroTrialSite::SetupBatrazTrial()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector Base = GetActorLocation();
	const FVector Offsets[] = {
		FVector(450.f, -160.f, 90.f),
		FVector(700.f, 140.f, 90.f),
		FVector(1000.f, -40.f, 90.f)
	};

	for (const FVector& Off : Offsets)
	{
		if (ANartyTrainingDummy* Dummy = World->SpawnActor<ANartyTrainingDummy>(
			ANartyTrainingDummy::StaticClass(), Base + Off, FRotator::ZeroRotator, Params))
		{
			Dummy->SetMaxHealth(160.f);
			TrialDummies.Add(Dummy);
		}
	}
	// Batraz clear-check timer starts after EnterTrial.
}

void ANartyHeroTrialSite::SetupSyrdonTrial()
{
	AddBlock(TEXT("CouncilA"), FVector(400.f, -180.f, 70.f), FVector(1.f, 1.f, 1.8f), FLinearColor(0.2f, 0.45f, 0.28f));
	AddBlock(TEXT("CouncilB"), FVector(400.f, 180.f, 70.f), FVector(1.f, 1.f, 1.8f), FLinearColor(0.2f, 0.45f, 0.28f));
	AddTrigger(TEXT("WhisperA"), FVector(400.f, -180.f, 60.f), FVector(110.f, 110.f, 90.f), 10);
	AddTrigger(TEXT("WhisperB"), FVector(400.f, 180.f, 60.f), FVector(110.f, 110.f, 90.f), 11);
}

UStaticMeshComponent* ANartyHeroTrialSite::AddBlock(
	const FName& Name, const FVector& RelLoc, const FVector& Scale, const FLinearColor& Color)
{
	if (!CubeMesh)
	{
		return nullptr;
	}

	TArray<UStaticMeshComponent*> ExistingBlocks;
	GetComponents<UStaticMeshComponent>(ExistingBlocks);
	for (UStaticMeshComponent* Existing : ExistingBlocks)
	{
		if (Existing && Existing->GetFName() == Name)
		{
			Existing->SetVisibility(true);
			Existing->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			return Existing;
		}
	}

	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this, Name);
	Block->SetupAttachment(Root);
	Block->SetStaticMesh(CubeMesh);
	Block->SetRelativeLocation(RelLoc);
	Block->SetRelativeScale3D(Scale);
	Block->SetCollisionProfileName(TEXT("BlockAll"));
	Block->RegisterComponent();
	AddInstanceComponent(Block);

	if (UMaterialInstanceDynamic* Dyn = Block->CreateAndSetMaterialInstanceDynamic(0))
	{
		Dyn->SetVectorParameterValue(TEXT("Color"), Color);
		Dyn->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
	return Block;
}

UBoxComponent* ANartyHeroTrialSite::AddTrigger(
	const FName& Name, const FVector& RelLoc, const FVector& Extent, int32 VisionIndex)
{
	TArray<UBoxComponent*> ExistingBoxes;
	GetComponents<UBoxComponent>(ExistingBoxes);
	for (UBoxComponent* Existing : ExistingBoxes)
	{
		if (Existing && Existing->GetFName() == Name)
		{
			Existing->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			VisionTriggers.AddUnique(Existing);
			return Existing;
		}
	}

	UBoxComponent* Box = NewObject<UBoxComponent>(this, Name);
	Box->SetupAttachment(Root);
	Box->SetRelativeLocation(RelLoc);
	Box->SetBoxExtent(Extent);
	Box->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Box->ComponentTags.Add(*FString::Printf(TEXT("Vision_%d"), VisionIndex));
	Box->RegisterComponent();
	AddInstanceComponent(Box);
	Box->OnComponentBeginOverlap.AddDynamic(this, &ANartyHeroTrialSite::OnVisionOverlap);
	VisionTriggers.Add(Box);
	return Box;
}

void ANartyHeroTrialSite::OnGateOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bActive || bCompleted)
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

	InsideGateCharacter = Character;
	if (UNartyInteractComponent* Interact = UNartyInteractComponent::EnsureOn(Character))
	{
		Interact->PushInteractTarget(this);
	}

	if (!bEntered && !bDialogOpen)
	{
		OpenIntro(Character);
	}
}

void ANartyHeroTrialSite::OnGateEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor == InsideGateCharacter.Get())
	{
		InsideGateCharacter = nullptr;
	}

	// Keep interact focus while waiting on the irreversible final choice.
	if (bAwaitingFinalChoice)
	{
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(OtherActor))
	{
		if (UNartyInteractComponent* Interact = Character->FindComponentByClass<UNartyInteractComponent>())
		{
			Interact->PopInteractTarget(this);
		}
	}
}

bool ANartyHeroTrialSite::CanNartyInteract() const
{
	if (!bActive || bCompleted || bDialogOpen)
	{
		return false;
	}
	if (bAwaitingFinalChoice)
	{
		return true;
	}
	return !bEntered && InsideGateCharacter.IsValid();
}

void ANartyHeroTrialSite::TryNartyInteract(ACharacter* Character)
{
	if (!Character || !CanNartyInteract())
	{
		return;
	}

	if (bAwaitingFinalChoice)
	{
		OpenFinalDialog(Character);
		return;
	}

	OpenIntro(Character);
}

void ANartyHeroTrialSite::OpenIntro(ACharacter* Character)
{
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC || bDialogOpen || bCompleted || bEntered)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;

	FText Title;
	FText Body;
	FText Accept = NSLOCTEXT("Narty", "Trial_Enter", "\u0412\u043e\u0439\u0442\u0438");

	switch (ActiveHero)
	{
	case ENartyHero::Soslan:
		Title = NSLOCTEXT("Narty", "Trial_SoslanTitle", "\u0426\u0430\u0440\u0441\u0442\u0432\u043e \u043c\u0451\u0440\u0442\u0432\u044b\u0445");
		Body = NSLOCTEXT("Narty", "Trial_SoslanIntro",
			"\u0421\u043e\u0441\u043b\u0430\u043d...\n\u0417\u0434\u0435\u0441\u044c \u0432\u0438\u0434\u043d\u0430 \u0446\u0435\u043d\u0430 \u043b\u0436\u0438, \u0441\u043a\u0443\u043f\u043e\u0441\u0442\u0438 \u0438 \u043d\u0430\u0440\u0443\u0448\u0435\u043d\u043d\u043e\u0433\u043e \u0433\u043e\u0441\u0442\u0435\u043f\u0440\u0438\u0438\u043c\u0441\u0442\u0432\u0430.\n\u041f\u0440\u043e\u0439\u0434\u0438 \u0442\u0440\u0438 \u0432\u0438\u0434\u0435\u043d\u0438\u044f.");
		break;
	case ENartyHero::Batraz:
		Title = NSLOCTEXT("Narty", "Trial_BatrazTitle", "\u041a\u0440\u043e\u0432\u044c \u0440\u043e\u0434\u0430");
		Body = NSLOCTEXT("Narty", "Trial_BatrazIntro",
			"\u0411\u0430\u0442\u0440\u0430\u0437...\n\u0412\u0440\u0430\u0436\u0434\u0430 \u0440\u043e\u0434\u043e\u0432 \u0443\u0436\u0435 \u043a\u0440\u043e\u0432\u043e\u0442\u043e\u0447\u0438\u0442.\n\u0421\u0440\u0430\u0437\u0438 \u0432\u0440\u0430\u0433\u043e\u0432 \u2014 \u0438 \u0440\u0435\u0448\u0438, \u0447\u0442\u043e \u0434\u0435\u043b\u0430\u0442\u044c \u0441 \u044f\u0440\u043e\u0441\u0442\u044c\u044e.");
		break;
	case ENartyHero::Syrdon:
		Title = NSLOCTEXT("Narty", "Trial_SyrdonTitle", "\u0428\u0451\u043f\u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430");
		Body = NSLOCTEXT("Narty", "Trial_SyrdonIntro",
			"\u0421\u044b\u0440\u0434\u043e\u043d...\n\u0414\u0432\u0430 \u0433\u043e\u043b\u043e\u0441\u0430 \u0440\u0430\u0441\u043a\u0430\u043b\u044b\u0432\u0430\u044e\u0442 \u0441\u043e\u0432\u0435\u0442.\n\u0412\u044b\u0441\u043b\u0443\u0448\u0430\u0439 \u043e\u0431\u0430 \u2014 \u0438 \u0432\u044b\u0431\u0435\u0440\u0438, \u0447\u0442\u043e \u0441\u043a\u0430\u0437\u0430\u0442\u044c \u043d\u044b\u0445\u0430\u0441\u0443.");
		break;
	default:
		Title = NSLOCTEXT("Narty", "Trial_DefaultTitle", "\u0418\u0441\u043f\u044b\u0442\u0430\u043d\u0438\u0435");
		Body = NSLOCTEXT("Narty", "Trial_DefaultIntro", "\u0412\u043e\u0439\u0434\u0438.");
		break;
	}

	if (!IntroWidget)
	{
		IntroWidget = CreateWidget<UNartyForgeDialogWidget>(PC, UNartyForgeDialogWidget::StaticClass());
		if (IntroWidget)
		{
			IntroWidget->OnAccepted.AddDynamic(this, &ANartyHeroTrialSite::HandleIntroAccepted);
			IntroWidget->OnClosed.AddDynamic(this, &ANartyHeroTrialSite::HandleIntroClosed);
		}
	}

	if (IntroWidget)
	{
		IntroWidget->SetDialogTexts(Title, Body, Accept);
		if (!IntroWidget->IsInViewport())
		{
			IntroWidget->AddToViewport(1400);
		}
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (IntroWidget)
	{
		Mode.SetWidgetToFocus(IntroWidget->TakeWidget());
	}
	PC->SetInputMode(Mode);
	Character->DisableInput(PC);
	UNartyInteractComponent::NotifyPromptChanged(Character);
}

void ANartyHeroTrialSite::HandleIntroAccepted()
{
	CloseAnyDialog();
	if (ACharacter* Character = InteractingCharacter.Get())
	{
		EnterTrial(Character);
	}
}

void ANartyHeroTrialSite::HandleIntroClosed()
{
	CloseAnyDialog();

	if (bCompleted || !bEntered)
	{
		return;
	}

	if (ACharacter* Character = InteractingCharacter.Get())
	{
		if (ActiveHero == ENartyHero::Soslan && VisionsSeen >= 3)
		{
			OpenFinalDialog(Character);
		}
		else if (ActiveHero == ENartyHero::Syrdon && SyrdonDeals >= 2)
		{
			OpenFinalDialog(Character);
		}
	}
}

void ANartyHeroTrialSite::EnterTrial(ACharacter* Character)
{
	bEntered = true;
	const FVector Inside = GetActorTransform().TransformPosition(FVector(280.f, 0.f, 90.f));
	Character->SetActorLocation(Inside, false, nullptr, ETeleportType::TeleportPhysics);
	GateLabel->SetText(NSLOCTEXT("Narty", "Trial_Inside", "\u0418\u0441\u043f\u044b\u0442\u0430\u043d\u0438\u0435"));

	if (ActiveHero == ENartyHero::Batraz)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				BatrazCheckHandle,
				this,
				&ANartyHeroTrialSite::CheckBatrazCleared,
				0.5f,
				true);
		}
	}
}

void ANartyHeroTrialSite::OnVisionOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bEntered || bCompleted || bDialogOpen || !OverlappedComponent)
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

	int32 VisionIndex = -1;
	for (const FName& Tag : OverlappedComponent->ComponentTags)
	{
		const FString TagStr = Tag.ToString();
		if (TagStr.StartsWith(TEXT("Vision_")))
		{
			VisionIndex = FCString::Atoi(*TagStr.RightChop(7));
			break;
		}
	}

	if (VisionIndex < 0)
	{
		return;
	}

	// Disable this trigger after use
	OverlappedComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractingCharacter = Character;

	if (ActiveHero == ENartyHero::Soslan && VisionIndex <= 2)
	{
		FText Body;
		switch (VisionIndex)
		{
		case 0:
			Body = NSLOCTEXT("Narty", "Vision_Greed",
				"\u0412\u0438\u0434\u0435\u043d\u0438\u0435:\n\u0421\u043a\u0443\u043f\u0435\u0446 \u0445\u0440\u0430\u043d\u0438\u0442 \u0445\u043b\u0435\u0431, \u043f\u043e\u043a\u0430 \u0441\u0435\u043b\u0435\u043d\u0438\u0435 \u0433\u043e\u043b\u043e\u0434\u0430\u0435\u0442.");
			break;
		case 1:
			Body = NSLOCTEXT("Narty", "Vision_Lie",
				"\u0412\u0438\u0434\u0435\u043d\u0438\u0435:\n\u041b\u0436\u0435\u0446 \u043a\u043b\u044f\u043d\u0451\u0442\u0441\u044f \u0447\u0430\u0448\u0435\u0439 \u2014 \u0438 \u0447\u0430\u0448\u0430 \u043c\u043e\u043b\u0447\u0438\u0442 \u0432\u0435\u0447\u043d\u043e.");
			break;
		default:
			Body = NSLOCTEXT("Narty", "Vision_Guest",
				"\u0412\u0438\u0434\u0435\u043d\u0438\u0435:\n\u041a\u0442\u043e \u043d\u0430\u0440\u0443\u0448\u0438\u043b \u0433\u043e\u0441\u0442\u0435\u043f\u0440\u0438\u0438\u043c\u0441\u0442\u0432\u043e, \u043f\u044c\u0451\u0442 \u043e\u0434\u0438\u043d \u0432 \u0442\u0435\u043c\u043d\u043e\u0442\u0435.");
			break;
		}

		bDialogOpen = true;
		if (!IntroWidget)
		{
			IntroWidget = CreateWidget<UNartyForgeDialogWidget>(PC, UNartyForgeDialogWidget::StaticClass());
		}
		// Temporarily reuse accept to continue
		IntroWidget->OnAccepted.Clear();
		IntroWidget->OnClosed.Clear();
		IntroWidget->OnAccepted.AddDynamic(this, &ANartyHeroTrialSite::HandleIntroClosed);
		IntroWidget->OnClosed.AddDynamic(this, &ANartyHeroTrialSite::HandleIntroClosed);
		IntroWidget->SetDialogTexts(
			NSLOCTEXT("Narty", "Vision_Title", "\u0412\u0438\u0434\u0435\u043d\u0438\u0435"),
			Body,
			NSLOCTEXT("Narty", "Vision_Continue", "\u0414\u0430\u043b\u044c\u0448\u0435"));
		IntroWidget->AddToViewport(1400);
		PC->bShowMouseCursor = true;
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(IntroWidget->TakeWidget());
		PC->SetInputMode(Mode);
		Character->DisableInput(PC);
		UNartyInteractComponent::NotifyPromptChanged(Character);

		VisionsSeen++;
		return;
	}

	if (ActiveHero == ENartyHero::Syrdon && (VisionIndex == 10 || VisionIndex == 11))
	{
		bDialogOpen = true;
		const bool bFirst = VisionIndex == 10;
		const FText Body = bFirst
			? NSLOCTEXT("Narty", "Whisper_A",
				"\u0413\u043e\u043b\u043e\u0441 \u0411\u043e\u0440\u0430\u0442\u0430:\n\u00ab\u041f\u0443\u0441\u0442\u044c \u0432\u043e\u0438\u043d\u044b \u043f\u043b\u0430\u0442\u044f\u0442 \u043a\u0440\u043e\u0432\u044c\u044e. \u041c\u044b \u0441\u043e\u0445\u0440\u0430\u043d\u0438\u043c \u0437\u043e\u043b\u043e\u0442\u043e.\u00bb")
			: NSLOCTEXT("Narty", "Whisper_B",
				"\u0413\u043e\u043b\u043e\u0441 \u0410\u0445\u0441\u0430\u0440\u0442\u0430\u0433\u0433\u0430\u0442\u0430:\n\u00ab\u0411\u0435\u0437 \u0447\u0435\u0441\u0442\u0438 \u0437\u043e\u043b\u043e\u0442\u043e \u2014 \u043f\u0443\u0441\u0442\u043e\u0439 \u0437\u0432\u043e\u043d.\u00bb");

		if (!IntroWidget)
		{
			IntroWidget = CreateWidget<UNartyForgeDialogWidget>(PC, UNartyForgeDialogWidget::StaticClass());
		}
		IntroWidget->OnAccepted.Clear();
		IntroWidget->OnClosed.Clear();
		IntroWidget->OnAccepted.AddDynamic(this, &ANartyHeroTrialSite::HandleIntroClosed);
		IntroWidget->OnClosed.AddDynamic(this, &ANartyHeroTrialSite::HandleIntroClosed);
		IntroWidget->SetDialogTexts(
			NSLOCTEXT("Narty", "Whisper_Title", "\u0428\u0451\u043f\u043e\u0442"),
			Body,
			NSLOCTEXT("Narty", "Whisper_Heard", "\u0421\u043b\u044b\u0448\u0443"));
		IntroWidget->AddToViewport(1400);
		PC->bShowMouseCursor = true;
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(IntroWidget->TakeWidget());
		PC->SetInputMode(Mode);
		Character->DisableInput(PC);
		UNartyInteractComponent::NotifyPromptChanged(Character);

		SyrdonDeals++;
		return;
	}
}

void ANartyHeroTrialSite::CloseAnyDialog()
{
	if (IntroWidget)
	{
		IntroWidget->RemoveFromParent();
	}
	if (FinalWidget)
	{
		FinalWidget->RemoveFromParent();
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
	UNartyInteractComponent::NotifyPromptChanged(InteractingCharacter.Get());
}

void ANartyHeroTrialSite::CheckBatrazCleared()
{
	if (!bEntered || bCompleted || ActiveHero != ENartyHero::Batraz)
	{
		return;
	}

	for (const TObjectPtr<ANartyTrainingDummy>& Dummy : TrialDummies)
	{
		if (IsValid(Dummy) && Dummy->GetHealth() > 0.f)
		{
			return;
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BatrazCheckHandle);
	}

	if (ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		OpenFinalDialog(Character);
	}
}

void ANartyHeroTrialSite::OpenFinalDialog(ACharacter* Character)
{
	if (bCompleted || bDialogOpen || !Character)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;
	bAwaitingFinalChoice = true;

	if (UNartyInteractComponent* Interact = UNartyInteractComponent::EnsureOn(Character))
	{
		Interact->PushInteractTarget(this);
	}

	FText Title;
	FText Body;
	TArray<FText> Choices;

	switch (ActiveHero)
	{
	case ENartyHero::Soslan:
		Title = NSLOCTEXT("Narty", "Final_SoslanTitle", "\u0421\u0443\u0434 \u043c\u0451\u0440\u0442\u0432\u044b\u0445");
		Body = NSLOCTEXT("Narty", "Final_SoslanBody", "\u0422\u044b \u0432\u0438\u0434\u0435\u043b \u0446\u0435\u043d\u0443. \u0427\u0442\u043e \u0443\u043d\u0435\u0441\u0451\u0448\u044c \u0436\u0438\u0432\u044b\u043c?");
		Choices.Add(NSLOCTEXT("Narty", "Final_SoslanA", "\u041f\u0440\u0430\u0432\u0434\u0443 \u2014 \u0434\u0430\u0436\u0435 \u0435\u0441\u043b\u0438 \u043e\u043d\u0430 \u0436\u0436\u0451\u0442."));
		Choices.Add(NSLOCTEXT("Narty", "Final_SoslanB", "\u041c\u043e\u043b\u0447\u0430\u043d\u0438\u0435 \u2014 \u043f\u0443\u0441\u0442\u044c \u0436\u0438\u0432\u044b\u0435 \u0441\u0430\u043c\u0438 \u0440\u0435\u0448\u0430\u0442."));
		break;
	case ENartyHero::Batraz:
		Title = NSLOCTEXT("Narty", "Final_BatrazTitle", "\u042f\u0440\u043e\u0441\u0442\u044c");
		Body = NSLOCTEXT("Narty", "Final_BatrazBody", "\u0412\u0440\u0430\u0433\u0438 \u043f\u0430\u043b\u0438. \u041a\u0443\u0434\u0430 \u043d\u0430\u043f\u0440\u0430\u0432\u0438\u0442\u044c \u0443\u0434\u0430\u0440?");
		Choices.Add(NSLOCTEXT("Narty", "Final_BatrazA", "\u0421\u0434\u0435\u0440\u0436\u0430\u0442\u044c \u044f\u0440\u043e\u0441\u0442\u044c \u0440\u0430\u0434\u0438 \u0440\u043e\u0434\u0430."));
		Choices.Add(NSLOCTEXT("Narty", "Final_BatrazB", "\u0423\u0434\u0430\u0440\u0438\u0442\u044c \u043f\u043e \u043d\u0435\u0431\u0443 \u2014 \u043f\u0443\u0441\u0442\u044c \u0443\u0441\u043b\u044b\u0448\u0430\u0442."));
		break;
	case ENartyHero::Syrdon:
		Title = NSLOCTEXT("Narty", "Final_SyrdonTitle", "\u0421\u043b\u043e\u0432\u043e \u043d\u044b\u0445\u0430\u0441\u0443");
		Body = NSLOCTEXT("Narty", "Final_SyrdonBody", "\u041e\u0431\u0430 \u0448\u0451\u043f\u043e\u0442\u0430 \u0443\u0441\u043b\u044b\u0448\u0430\u043d\u044b. \u0427\u0442\u043e \u0441\u043a\u0430\u0436\u0435\u0448\u044c \u0441\u043e\u0432\u0435\u0442\u0443?");
		Choices.Add(NSLOCTEXT("Narty", "Final_SyrdonA", "\u0421\u0432\u0435\u0441\u0442\u0438 \u0440\u043e\u0434\u044b \u043e\u0434\u043d\u0438\u043c \u0441\u043b\u043e\u0432\u043e\u043c."));
		Choices.Add(NSLOCTEXT("Narty", "Final_SyrdonB", "\u041f\u0443\u0441\u0442\u044c \u0440\u0430\u0441\u043a\u043e\u043b \u0440\u0430\u0431\u043e\u0442\u0430\u0435\u0442 \u043d\u0430 \u0442\u0435\u0431\u044f."));
		break;
	default:
		Title = NSLOCTEXT("Narty", "Final_DefaultTitle", "\u0418\u0442\u043e\u0433");
		Body = NSLOCTEXT("Narty", "Final_DefaultBody", "\u0412\u044b\u0431\u0435\u0440\u0438.");
		Choices.Add(NSLOCTEXT("Narty", "Final_DefaultA", "A"));
		Choices.Add(NSLOCTEXT("Narty", "Final_DefaultB", "B"));
		break;
	}

	if (!FinalWidget)
	{
		FinalWidget = CreateWidget<UNartyChoiceDialogWidget>(PC, UNartyChoiceDialogWidget::StaticClass());
		if (FinalWidget)
		{
			FinalWidget->OnChoicePicked.AddDynamic(this, &ANartyHeroTrialSite::HandleFinalChoice);
			FinalWidget->OnClosed.AddDynamic(this, &ANartyHeroTrialSite::HandleFinalClosed);
		}
	}

	if (FinalWidget)
	{
		// Irreversible chapter beat — no "Позже" soft-lock.
		FinalWidget->Setup(Title, Body, Choices, false);
		if (!FinalWidget->IsInViewport())
		{
			FinalWidget->AddToViewport(1500);
		}
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (FinalWidget)
	{
		Mode.SetWidgetToFocus(FinalWidget->TakeWidget());
	}
	PC->SetInputMode(Mode);
	Character->DisableInput(PC);
	UNartyInteractComponent::NotifyPromptChanged(Character);
}

void ANartyHeroTrialSite::HandleFinalChoice(int32 ChoiceIndex)
{
	CloseAnyDialog();
	// Index 0 = noble/restrained path for all heroes
	CompleteTrial(ChoiceIndex == 0);
}

void ANartyHeroTrialSite::HandleFinalClosed()
{
	CloseAnyDialog();
	bAwaitingFinalChoice = true;
	GateLabel->SetText(NSLOCTEXT("Narty", "Trial_ChooseE", "E \u2014 \u0432\u044b\u0431\u043e\u0440"));
}

void ANartyHeroTrialSite::CompleteTrial(bool bNoblePath)
{
	if (bCompleted)
	{
		return;
	}

	bCompleted = true;
	bAwaitingFinalChoice = false;
	GateLabel->SetText(NSLOCTEXT("Narty", "Trial_Done", "\u041f\u0443\u0442\u044c \u043f\u0440\u043e\u0439\u0434\u0435\u043d"));

	if (ACharacter* Character = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		if (UNartyInteractComponent* Interact = Character->FindComponentByClass<UNartyInteractComponent>())
		{
			Interact->PopInteractTarget(this);
		}
	}

	if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->NotifyHeroTrialCompleted(bNoblePath);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: hero trial complete noble=%d"), bNoblePath ? 1 : 0);
}
