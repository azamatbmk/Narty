#include "NartyMountainFireActor.h"

#include "NartyTrainingDummy.h"
#include "NartyForgeDialogWidget.h"
#include "NartyGameInstance.h"
#include "NartyInteractComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ANartyMountainFireActor::ANartyMountainFireActor()
{
	PrimaryActorTick.bCanEverTick = true;

	Pedestal = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pedestal"));
	SetRootComponent(Pedestal);

	FlameMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlameMesh"));
	FlameMesh->SetupAttachment(Pedestal);
	FlameMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FlameLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FlameLight"));
	FlameLight->SetupAttachment(Pedestal);
	FlameLight->SetIntensity(12000.f);
	FlameLight->SetLightColor(FLinearColor(1.f, 0.4f, 0.05f));
	FlameLight->SetAttenuationRadius(1600.f);
	FlameLight->SetRelativeLocation(FVector(0.f, 0.f, 160.f));

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Pedestal);
	Trigger->SetBoxExtent(FVector(160.f, 160.f, 120.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 80.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Pedestal);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 240.f));
	Label->SetRelativeRotation(FRotator(0.f, 180.f, 0.f));
	Label->SetWorldSize(36.f);
	Label->SetTextRenderColor(FColor(255, 160, 60));
	Label->SetText(NSLOCTEXT("Narty", "Fire_Label", "\u041e\u0433\u043e\u043d\u044c \u0433\u043e\u0440\u044b"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Pedestal->SetStaticMesh(CubeMesh.Object);
		Pedestal->SetWorldScale3D(FVector(1.6f, 1.6f, 0.5f));
		FlameMesh->SetStaticMesh(CubeMesh.Object);
		FlameMesh->SetRelativeLocation(FVector(0.f, 0.f, 90.f));
		FlameMesh->SetWorldScale3D(FVector(0.45f, 0.45f, 1.1f));
	}

	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

void ANartyMountainFireActor::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ANartyMountainFireActor::OnFireOverlap);
	Trigger->OnComponentEndOverlap.AddDynamic(this, &ANartyMountainFireActor::OnFireEndOverlap);
}

void ANartyMountainFireActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bFireTaken && FlameMesh)
	{
		const float Pulse = 0.45f + 0.08f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f);
		FlameMesh->SetWorldScale3D(FVector(Pulse, Pulse, 1.0f + Pulse));
	}
}

void ANartyMountainFireActor::ActivateForQuest()
{
	bQuestActive = true;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	FlameLight->SetVisibility(true);

	EnsureGuardiansSpawned();
	UE_LOG(LogTemp, Warning, TEXT("Narty: mountain fire activated"));
}

void ANartyMountainFireActor::EnsureGuardiansSpawned()
{
	UWorld* World = GetWorld();
	if (!World || Guardians.Num() > 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	const FVector Base = GetActorLocation();
	ANartyTrainingDummy* G1 = World->SpawnActor<ANartyTrainingDummy>(
		ANartyTrainingDummy::StaticClass(), Base + FVector(-180.f, -140.f, 50.f), FRotator::ZeroRotator, Params);
	ANartyTrainingDummy* G2 = World->SpawnActor<ANartyTrainingDummy>(
		ANartyTrainingDummy::StaticClass(), Base + FVector(-180.f, 140.f, 50.f), FRotator::ZeroRotator, Params);

	if (G1)
	{
		Guardians.Add(G1);
		G1->OnDefeated.AddDynamic(this, &ANartyMountainFireActor::OnGuardianDefeated);
	}
	if (G2)
	{
		Guardians.Add(G2);
		G2->OnDefeated.AddDynamic(this, &ANartyMountainFireActor::OnGuardianDefeated);
	}
}

int32 ANartyMountainFireActor::CaptureGuardiansAlive() const
{
	if (!bQuestActive || bFireTaken || Guardians.Num() == 0)
	{
		return -1;
	}

	int32 Alive = 0;
	for (const TObjectPtr<ANartyTrainingDummy>& Dummy : Guardians)
	{
		if (IsValid(Dummy) && Dummy->GetHealth() > 0.f)
		{
			++Alive;
		}
	}
	return Alive;
}

void ANartyMountainFireActor::RestoreFromSave(bool bActive, bool bTaken, int32 GuardiansAlive)
{
	if (bActive && !bTaken)
	{
		bQuestActive = true;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		if (FlameLight)
		{
			FlameLight->SetVisibility(true);
		}

		if (GuardiansAlive < 0)
		{
			EnsureGuardiansSpawned();
		}
		else if (GuardiansAlive == 0)
		{
			Label->SetText(NSLOCTEXT("Narty", "Fire_Ready", "\u041e\u0433\u043e\u043d\u044c \u0433\u043e\u0440\u044b"));
		}
		else
		{
			EnsureGuardiansSpawned();
			int32 AliveLeft = GuardiansAlive;
			for (int32 Index = Guardians.Num() - 1; Index >= 0; --Index)
			{
				ANartyTrainingDummy* Dummy = Guardians[Index].Get();
				if (!IsValid(Dummy))
				{
					continue;
				}

				if (AliveLeft > 0)
				{
					--AliveLeft;
				}
				else
				{
					Dummy->ReceiveStrike(99999.f, this);
				}
			}
		}
	}
	else if (bActive)
	{
		bQuestActive = true;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(true);
		if (FlameLight)
		{
			FlameLight->SetVisibility(true);
		}
	}

	if (bTaken)
	{
		bFireTaken = true;
		if (FlameMesh)
		{
			FlameMesh->SetVisibility(false);
		}
		if (FlameLight)
		{
			FlameLight->SetIntensity(1500.f);
			FlameLight->SetLightColor(FLinearColor(0.4f, 0.45f, 0.5f));
		}
		Label->SetText(NSLOCTEXT("Narty", "Fire_Taken", "\u041e\u0433\u043e\u043d\u044c \u0432\u0437\u044f\u0442"));
	}
}

void ANartyMountainFireActor::OnGuardianDefeated(AActor* /*DestroyedActor*/)
{
	if (AreGuardiansDefeated())
	{
		Label->SetText(NSLOCTEXT("Narty", "Fire_Ready", "\u041e\u0433\u043e\u043d\u044c \u0433\u043e\u0440\u044b"));
		RetryTakeForOverlappingPlayers();
	}
}

void ANartyMountainFireActor::RetryTakeForOverlappingPlayers()
{
	if (!Trigger || bFireTaken || !bQuestActive)
	{
		return;
	}

	TArray<AActor*> Overlapping;
	Trigger->GetOverlappingActors(Overlapping, ACharacter::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (ACharacter* Character = Cast<ACharacter>(Actor))
		{
			TryTakeFire(Character);
			if (bFireTaken)
			{
				return;
			}
		}
	}
}

void ANartyMountainFireActor::OnFireOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bQuestActive || bFireTaken)
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
		TryTakeFire(Character);
	}
}

void ANartyMountainFireActor::OnFireEndOverlap(
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

bool ANartyMountainFireActor::CanNartyInteract() const
{
	return bQuestActive && !bFireTaken && InsideCharacter.IsValid() && !bDialogOpen;
}

void ANartyMountainFireActor::TryNartyInteract(ACharacter* Character)
{
	if (!Character || !CanNartyInteract())
	{
		return;
	}
	TryTakeFire(Character);
}

void ANartyMountainFireActor::TryTakeFire(ACharacter* Character)
{
	if (!Character || bFireTaken || bDialogOpen)
	{
		return;
	}

	UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this));
	const ENartyHero Hero = GI ? GI->GetSelectedHero() : ENartyHero::None;

	if (Hero == ENartyHero::Syrdon)
	{
		OpenBargainDialog(Character);
		return;
	}

	if (Hero == ENartyHero::Batraz && !AreGuardiansDefeated())
	{
		Label->SetText(NSLOCTEXT("Narty", "Fire_Guards", "\u0421\u043d\u0430\u0447\u0430\u043b\u0430 \u0441\u0442\u0440\u0430\u0436\u0438"));
		UE_LOG(LogTemp, Warning, TEXT("Narty: Batraz must defeat mountain guardians first"));
		return;
	}

	// Soslan (and Batraz after clears): take the fire.
	GiveFireToPlayer(Character);
}

bool ANartyMountainFireActor::AreGuardiansDefeated() const
{
	if (Guardians.Num() == 0)
	{
		return true;
	}

	for (const TObjectPtr<ANartyTrainingDummy>& Dummy : Guardians)
	{
		if (IsValid(Dummy) && Dummy->GetHealth() > 0.f)
		{
			return false;
		}
	}
	return true;
}

void ANartyMountainFireActor::OpenBargainDialog(ACharacter* Character)
{
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!PC || bDialogOpen || bFireTaken)
	{
		return;
	}

	InteractingCharacter = Character;
	bDialogOpen = true;

	if (!BargainWidget)
	{
		BargainWidget = CreateWidget<UNartyForgeDialogWidget>(PC, UNartyForgeDialogWidget::StaticClass());
		if (BargainWidget)
		{
			BargainWidget->OnAccepted.AddDynamic(this, &ANartyMountainFireActor::HandleBargainAccepted);
			BargainWidget->OnClosed.AddDynamic(this, &ANartyMountainFireActor::HandleBargainClosed);
		}
	}

	if (BargainWidget)
	{
		BargainWidget->SetDialogTexts(
			NSLOCTEXT("Narty", "Fire_BargainTitle", "\u0421\u0442\u0440\u0430\u0436 \u043e\u0433\u043d\u044f"),
			NSLOCTEXT("Narty", "Fire_BargainBody",
				"\u0421\u044b\u0440\u0434\u043e\u043d...\n\u0422\u044b \u043c\u043e\u0436\u0435\u0448\u044c \u0432\u0437\u044f\u0442\u044c \u043e\u0433\u043e\u043d\u044c \u0441\u0438\u043b\u043e\u0439.\n\u0418\u043b\u0438 \u043e\u0431\u043c\u0435\u043d\u044f\u0442\u044c \u0441\u043b\u043e\u0432\u043e \u043d\u0430 \u0436\u0430\u0440."),
			NSLOCTEXT("Narty", "Fire_BargainAccept", "\u041e\u0431\u043c\u0435\u043d\u044f\u0442\u044c"));

		if (!BargainWidget->IsInViewport())
		{
			BargainWidget->AddToViewport(1200);
		}
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (BargainWidget)
	{
		InputMode.SetWidgetToFocus(BargainWidget->TakeWidget());
	}
	PC->SetInputMode(InputMode);
	Character->DisableInput(PC);
	UNartyInteractComponent::NotifyPromptChanged(Character);
}

void ANartyMountainFireActor::HandleBargainAccepted()
{
	if (ACharacter* Character = InteractingCharacter.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			if (BargainWidget)
			{
				BargainWidget->RemoveFromParent();
			}
			PC->bShowMouseCursor = false;
			FInputModeGameOnly Mode;
			PC->SetInputMode(Mode);
			Character->EnableInput(PC);
		}
		GiveFireToPlayer(Character);
	}
	bDialogOpen = false;
	UNartyInteractComponent::NotifyPromptChanged(InteractingCharacter.Get());
}

void ANartyMountainFireActor::HandleBargainClosed()
{
	if (ACharacter* Character = InteractingCharacter.Get())
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			if (BargainWidget)
			{
				BargainWidget->RemoveFromParent();
			}
			PC->bShowMouseCursor = false;
			FInputModeGameOnly Mode;
			PC->SetInputMode(Mode);
			Character->EnableInput(PC);
		}
	}
	bDialogOpen = false;
	UNartyInteractComponent::NotifyPromptChanged(InteractingCharacter.Get());
}

void ANartyMountainFireActor::GiveFireToPlayer(ACharacter* Character)
{
	if (bFireTaken)
	{
		return;
	}

	bFireTaken = true;
	FlameMesh->SetVisibility(false);
	FlameLight->SetIntensity(1500.f);
	FlameLight->SetLightColor(FLinearColor(0.4f, 0.45f, 0.5f));
	Label->SetText(NSLOCTEXT("Narty", "Fire_Taken", "\u041e\u0433\u043e\u043d\u044c \u0432\u0437\u044f\u0442"));

	if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->NotifyFireTaken();
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: mountain fire taken by %s"), *GetNameSafe(Character));
}
