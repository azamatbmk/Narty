#include "NartyGameInstance.h"

#include "NartyHeroSelectWidget.h"
#include "NartyObjectiveWidget.h"
#include "NartyEndingWidget.h"
#include "NartyPrototypeArena.h"
#include "NartyMountainFireActor.h"
#include "NartySettlementHearthActor.h"
#include "NartyUatsamongaCup.h"
#include "NartyHeroTrialSite.h"
#include "NartyFinaleSite.h"
#include "NartyCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UNartyGameInstance::OnStart()
{
	Super::OnStart();
	ResetCampaignState();

	if (UWorld* World = GetWorld())
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(World);
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyGameInstance::BootstrapPrototype));
	}
}

void UNartyGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);

	if (!bRestartPending)
	{
		return;
	}

	bRestartPending = false;
	PrototypeArena = nullptr;

	if (UWorld* World = GetWorld())
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(World);
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyGameInstance::BootstrapPrototype));
	}
}

void UNartyGameInstance::ResetCampaignState()
{
	SelectedHero = ENartyHero::None;
	QuestStage = ENartyQuestStage::GetWeapon;
	bHasMountainFire = false;
	bToldTruthAtCup = false;
	bClanFeudStarted = false;
	bNobleTrialPath = false;
	ChosenEnding = ENartyEnding::None;

	if (IsValid(HeroSelectWidget))
	{
		HeroSelectWidget->RemoveFromParent();
	}
	HeroSelectWidget = nullptr;

	if (IsValid(ObjectiveWidget))
	{
		ObjectiveWidget->RemoveFromParent();
	}
	ObjectiveWidget = nullptr;

	HideEndingScreen();
	PrototypeArena = nullptr;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HeroSelectRetryHandle);
		World->GetTimerManager().ClearTimer(ArenaRetryHandle);
		World->GetTimerManager().ClearTimer(ApplyHeroRetryHandle);
	}
}

void UNartyGameInstance::BootstrapPrototype()
{
	EnsureArenaAndTeleport();
	ShowHeroSelectMenu();
}

void UNartyGameInstance::EnsureArenaAndTeleport()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!PrototypeArena)
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(World);
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		World->GetTimerManager().SetTimer(
			ArenaRetryHandle,
			this,
			&UNartyGameInstance::EnsureArenaAndTeleport,
			0.1f,
			false);
		return;
	}

	if (PrototypeArena)
	{
		const FTransform Start = PrototypeArena->GetPlayerStartTransform();
		PC->GetPawn()->SetActorTransform(Start, false, nullptr, ETeleportType::TeleportPhysics);
		PC->SetControlRotation(Start.Rotator());
	}
}

void UNartyGameInstance::SetSelectedHero(ENartyHero Hero)
{
	SelectedHero = Hero;
}

FNartyHeroStats UNartyGameInstance::GetStatsForHero(ENartyHero Hero) const
{
	FNartyHeroStats Stats;

	switch (Hero)
	{
	case ENartyHero::Soslan:
		Stats.MaxWalkSpeed = 700.f;
		Stats.JumpZVelocity = 750.f;
		Stats.Tint = FLinearColor(1.f, 0.85f, 0.35f);
		Stats.DisplayName = NSLOCTEXT("Narty", "Hero_Soslan", "\u0421\u043e\u0441\u043b\u0430\u043d");
		break;

	case ENartyHero::Batraz:
		Stats.MaxWalkSpeed = 450.f;
		Stats.JumpZVelocity = 520.f;
		Stats.Tint = FLinearColor(0.55f, 0.6f, 0.7f);
		Stats.DisplayName = NSLOCTEXT("Narty", "Hero_Batraz", "\u0411\u0430\u0442\u0440\u0430\u0437");
		break;

	case ENartyHero::Syrdon:
		Stats.MaxWalkSpeed = 560.f;
		Stats.JumpZVelocity = 650.f;
		Stats.Tint = FLinearColor(0.45f, 0.75f, 0.55f);
		Stats.DisplayName = NSLOCTEXT("Narty", "Hero_Syrdon", "\u0421\u044b\u0440\u0434\u043e\u043d");
		break;

	default:
		Stats.DisplayName = NSLOCTEXT("Narty", "Hero_None", "None");
		break;
	}

	return Stats;
}

FNartyHeroStats UNartyGameInstance::GetSelectedHeroStats() const
{
	return GetStatsForHero(SelectedHero);
}

void UNartyGameInstance::ShowHeroSelectMenu()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		World->GetTimerManager().SetTimer(
			HeroSelectRetryHandle,
			this,
			&UNartyGameInstance::ShowHeroSelectMenu,
			0.1f,
			false);
		return;
	}

	if (!IsValid(HeroSelectWidget))
	{
		HeroSelectWidget = CreateWidget<UNartyHeroSelectWidget>(PC, UNartyHeroSelectWidget::StaticClass());
		if (HeroSelectWidget)
		{
			HeroSelectWidget->OnHeroChosen.AddDynamic(this, &UNartyGameInstance::HandleHeroChosen);
		}
	}

	if (IsValid(HeroSelectWidget) && !HeroSelectWidget->IsInViewport())
	{
		HeroSelectWidget->AddToViewport(1000);
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (IsValid(HeroSelectWidget))
	{
		InputMode.SetWidgetToFocus(HeroSelectWidget->TakeWidget());
	}
	PC->SetInputMode(InputMode);

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->DisableInput(PC);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: hero select menu shown"));
}

void UNartyGameInstance::HideHeroSelectMenu()
{
	if (IsValid(HeroSelectWidget))
	{
		HeroSelectWidget->RemoveFromParent();
	}
	HeroSelectWidget = nullptr;

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);

			if (APawn* Pawn = PC->GetPawn())
			{
				Pawn->EnableInput(PC);
			}
		}
	}
}

void UNartyGameInstance::HandleHeroChosen(ENartyHero Hero)
{
	SetSelectedHero(Hero);
	HideHeroSelectMenu();
	EnsureArenaAndTeleport();
	ApplySelectedHeroToLocalPawn();
}

void UNartyGameInstance::ApplySelectedHeroToLocalPawn()
{
	if (!HasSelectedHero())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	ACharacter* HeroCharacter = PC ? PC->GetCharacter() : nullptr;
	if (!HeroCharacter && PC)
	{
		HeroCharacter = Cast<ACharacter>(PC->GetPawn());
	}

	if (!HeroCharacter)
	{
		World->GetTimerManager().SetTimer(
			ApplyHeroRetryHandle,
			this,
			&UNartyGameInstance::ApplySelectedHeroToLocalPawn,
			0.1f,
			false);
		return;
	}

	ApplyHeroToCharacter(HeroCharacter, SelectedHero);
}

void UNartyGameInstance::ApplyHeroToCharacter(ACharacter* HeroCharacter, ENartyHero Hero)
{
	if (!HeroCharacter || Hero == ENartyHero::None)
	{
		return;
	}

	const FNartyHeroStats Stats = GetStatsForHero(Hero);

	if (UCharacterMovementComponent* Move = HeroCharacter->GetCharacterMovement())
	{
		Move->MaxWalkSpeed = Stats.MaxWalkSpeed;
		Move->JumpZVelocity = Stats.JumpZVelocity;
	}

	if (USkeletalMeshComponent* Mesh = HeroCharacter->GetMesh())
	{
		const int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 Index = 0; Index < NumMaterials; ++Index)
		{
			if (!Mesh->GetMaterial(Index))
			{
				continue;
			}

			if (UMaterialInstanceDynamic* Dyn = Mesh->CreateAndSetMaterialInstanceDynamic(Index))
			{
				static const FName ParamNames[] = {
					TEXT("Tint"),
					TEXT("Color"),
					TEXT("BaseColor"),
					TEXT("Paint Color")
				};

				for (const FName& ParamName : ParamNames)
				{
					Dyn->SetVectorParameterValue(ParamName, Stats.Tint);
				}
			}
		}
	}

	HeroCharacter->SetActorScale3D(
		Hero == ENartyHero::Batraz ? FVector(1.08f) :
		Hero == ENartyHero::Syrdon ? FVector(0.96f) :
		FVector(1.f));

	UNartyCombatComponent* Combat = HeroCharacter->FindComponentByClass<UNartyCombatComponent>();
	if (!Combat)
	{
		Combat = NewObject<UNartyCombatComponent>(HeroCharacter, TEXT("NartyCombat"));
		Combat->RegisterComponent();
		HeroCharacter->AddInstanceComponent(Combat);
	}
	Combat->SetHero(Hero);

	UE_LOG(LogTemp, Warning, TEXT("Narty: hero applied -> %s (speed=%.0f)"),
		*Stats.DisplayName.ToString(), Stats.MaxWalkSpeed);

	EnsureObjectiveWidget();
	UpdateObjectiveUI();
}

void UNartyGameInstance::EnsureObjectiveWidget()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	if (!IsValid(ObjectiveWidget))
	{
		ObjectiveWidget = CreateWidget<UNartyObjectiveWidget>(PC, UNartyObjectiveWidget::StaticClass());
	}

	if (IsValid(ObjectiveWidget) && !ObjectiveWidget->IsInViewport())
	{
		ObjectiveWidget->AddToViewport(50);
	}
}

FText UNartyGameInstance::GetHeroFireHint() const
{
	switch (SelectedHero)
	{
	case ENartyHero::Soslan:
		return NSLOCTEXT("Narty", "FireHint_Soslan",
			"\u0421\u043e\u0441\u043b\u0430\u043d: \u0431\u044b\u0441\u0442\u0440\u043e \u0432\u0437\u043e\u0439\u0434\u0438 \u043d\u0430 \u0443\u0441\u0442\u0443\u043f \u0438 \u0445\u0432\u0430\u0442\u0438 \u043e\u0433\u043e\u043d\u044c.");
	case ENartyHero::Batraz:
		return NSLOCTEXT("Narty", "FireHint_Batraz",
			"\u0411\u0430\u0442\u0440\u0430\u0437: \u0441\u0440\u0430\u0437\u0438 \u0441\u0442\u0440\u0430\u0436\u0435\u0439 \u0443 \u043e\u0433\u043d\u044f, \u0437\u0430\u0442\u0435\u043c \u0432\u0437\u044f\u0442\u044c \u0436\u0430\u0440.");
	case ENartyHero::Syrdon:
		return NSLOCTEXT("Narty", "FireHint_Syrdon",
			"\u0421\u044b\u0440\u0434\u043e\u043d: \u043f\u043e\u0434\u043e\u0439\u0434\u0438 \u043a \u043e\u0433\u043d\u044e \u0438 \u043e\u0431\u043c\u0435\u043d\u044f\u0439 \u0441\u043b\u043e\u0432\u043e\u043c.");
	default:
		return NSLOCTEXT("Narty", "FireHint_Default", "\u0414\u043e\u0431\u0443\u0434\u044c \u043e\u0433\u043e\u043d\u044c \u043d\u0430 \u0443\u0441\u0442\u0443\u043f\u0435.");
	}
}

void UNartyGameInstance::UpdateObjectiveUI()
{
	EnsureObjectiveWidget();
	if (!ObjectiveWidget)
	{
		return;
	}

	switch (QuestStage)
	{
	case ENartyQuestStage::GetWeapon:
		ObjectiveWidget->SetObjective(
			NSLOCTEXT("Narty", "Obj_WeaponTitle", "\u041a\u0443\u0440\u0434\u0430\u043b\u0430\u0433\u043e\u043d"),
			NSLOCTEXT("Narty", "Obj_WeaponDetail", "\u0414\u043e\u0439\u0434\u0438 \u0434\u043e \u043a\u0443\u0437\u043d\u0438\u0446\u044b \u0438 \u0432\u0437\u044f\u043c\u0438 \u043e\u0440\u0443\u0436\u0438\u0435."));
		break;
	case ENartyQuestStage::FetchFire:
		ObjectiveWidget->SetObjective(
			NSLOCTEXT("Narty", "Obj_FireTitle", "\u041e\u0433\u043e\u043d\u044c \u0441\u0435\u043b\u0435\u043d\u0438\u044f"),
			GetHeroFireHint());
		break;
	case ENartyQuestStage::ReturnFire:
		ObjectiveWidget->SetObjective(
			NSLOCTEXT("Narty", "Obj_ReturnTitle", "\u0412\u0435\u0440\u043d\u0438 \u043e\u0433\u043e\u043d\u044c"),
			NSLOCTEXT("Narty", "Obj_ReturnDetail",
				"\u041e\u0442\u043d\u0435\u0441\u0438 \u0436\u0430\u0440 \u043a \u043e\u0447\u0430\u0433\u0443 \u0441\u0435\u043b\u0435\u043d\u0438\u044f (\u0443 \u0432\u0445\u043e\u0434\u0430 \u0432 \u0443\u0449\u0435\u043b\u044c\u0435)."));
		break;
	case ENartyQuestStage::Uatsamonga:
		ObjectiveWidget->SetObjective(
			NSLOCTEXT("Narty", "Obj_CupTitle", "\u0423\u0430\u0446\u0430\u043c\u043e\u043d\u0433\u0430"),
			NSLOCTEXT("Narty", "Obj_CupDetail",
				"\u041f\u043e\u0434\u043e\u0439\u0434\u0438 \u043a \u0447\u0430\u0448\u0435 \u043d\u0430 \u043d\u044b\u0445\u0430\u0441\u0435 (\u0446\u0435\u043d\u0442\u0440 \u0443\u0449\u0435\u043b\u044c\u044f) \u0438 \u0441\u043a\u0430\u0436\u0438 \u043e \u043f\u043e\u0434\u0432\u0438\u0433\u0435."));
		break;
	case ENartyQuestStage::HeroTrial:
		switch (SelectedHero)
		{
		case ENartyHero::Soslan:
			ObjectiveWidget->SetObjective(
				NSLOCTEXT("Narty", "Obj_TrialSoslanTitle", "\u0426\u0430\u0440\u0441\u0442\u0432\u043e \u043c\u0451\u0440\u0442\u0432\u044b\u0445"),
				NSLOCTEXT("Narty", "Obj_TrialSoslanDetail",
					"\u0419\u0434\u0438 \u043a \u0431\u043e\u043a\u043e\u0432\u043e\u043c\u0443 \u043f\u0440\u043e\u0445\u043e\u0434\u0443 \u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430. \u041f\u0440\u043e\u0439\u0434\u0438 \u0442\u0440\u0438 \u0432\u0438\u0434\u0435\u043d\u0438\u044f."));
			break;
		case ENartyHero::Batraz:
			ObjectiveWidget->SetObjective(
				NSLOCTEXT("Narty", "Obj_TrialBatrazTitle", "\u041a\u0440\u043e\u0432\u044c \u0440\u043e\u0434\u0430"),
				NSLOCTEXT("Narty", "Obj_TrialBatrazDetail",
					"\u0419\u0434\u0438 \u043a \u0431\u043e\u043a\u043e\u0432\u043e\u043c\u0443 \u043f\u0440\u043e\u0445\u043e\u0434\u0443. \u0421\u0440\u0430\u0437\u0438 \u0432\u0440\u0430\u0433\u043e\u0432 \u0440\u043e\u0434\u0430 \u0438 \u0440\u0435\u0448\u0438 \u0441\u0443\u0434\u044c\u0431\u0443 \u044f\u0440\u043e\u0441\u0442\u0438."));
			break;
		case ENartyHero::Syrdon:
			ObjectiveWidget->SetObjective(
				NSLOCTEXT("Narty", "Obj_TrialSyrdonTitle", "\u0428\u0451\u043f\u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430"),
				NSLOCTEXT("Narty", "Obj_TrialSyrdonDetail",
					"\u0419\u0434\u0438 \u043a \u0431\u043e\u043a\u043e\u0432\u043e\u043c\u0443 \u043f\u0440\u043e\u0445\u043e\u0434\u0443. \u0412\u044b\u0441\u043b\u0443\u0448\u0430\u0439 \u043e\u0431\u0430 \u0433\u043e\u043b\u043e\u0441\u0430 \u0438 \u0432\u044b\u0431\u0435\u0440\u0438 \u0441\u043b\u043e\u0432\u043e \u0441\u043e\u0432\u0435\u0442\u0443."));
			break;
		default:
			ObjectiveWidget->SetObjective(
				NSLOCTEXT("Narty", "Obj_TrialTitle", "\u0418\u0441\u043f\u044b\u0442\u0430\u043d\u0438\u0435"),
				NSLOCTEXT("Narty", "Obj_TrialDetail", "\u0419\u0434\u0438 \u043a \u043f\u0440\u043e\u0445\u043e\u0434\u0443 \u0433\u0435\u0440\u043e\u044f."));
			break;
		}
		break;
	case ENartyQuestStage::Finale:
		ObjectiveWidget->SetObjective(
			NSLOCTEXT("Narty", "Obj_FinaleTitle", "\u041a\u043e\u043d\u0435\u0446 \u043d\u0430\u0440\u0442\u043e\u0432"),
			NSLOCTEXT("Narty", "Obj_FinaleDetail",
				"\u0419\u0434\u0438 \u043a \u0421\u0430\u0442\u0430\u043d\u0435 \u0443 \u0432\u0445\u043e\u0434\u0430 \u0432 \u0443\u0449\u0435\u043b\u044c\u0435 (\u043e\u043a\u043e\u043b\u043e \u043e\u0447\u0430\u0433\u0430) \u0438 \u0432\u044b\u0431\u0435\u0440\u0438 \u0441\u0443\u0434\u044c\u0431\u0443 \u0440\u043e\u0434\u0430."));
		break;
	case ENartyQuestStage::Completed:
	{
		FText Title;
		FText Detail;
		GetEndingTexts(ChosenEnding, Title, Detail);
		ObjectiveWidget->SetObjective(Title, Detail);
		break;
	}
	default:
		break;
	}
}

void UNartyGameInstance::GetEndingTexts(ENartyEnding Ending, FText& OutTitle, FText& OutBody) const
{
	OutTitle = NSLOCTEXT("Narty", "Obj_EndTitle", "\u041a\u043e\u043d\u0435\u0446");
	OutBody = NSLOCTEXT("Narty", "Obj_EndDefault", "\u041a\u0430\u043c\u043f\u0430\u043d\u0438\u044f \u0437\u0430\u0432\u0435\u0440\u0448\u0435\u043d\u0430.");

	switch (Ending)
	{
	case ENartyEnding::SoslanSacrifice:
		OutTitle = NSLOCTEXT("Narty", "Obj_EndSoslanATitle", "\u0421\u043e\u0441\u043b\u0430\u043d");
		OutBody = NSLOCTEXT("Narty", "Obj_EndSoslanA",
			"\u041a\u043e\u043b\u0435\u0441\u043e \u043f\u0440\u0438\u043d\u044f\u0442\u043e. \u0421\u0435\u043b\u0435\u043d\u0438\u0435 \u0441\u043f\u0430\u0441\u0435\u043d\u043e. \u0413\u0435\u0440\u043e\u0439 \u0443\u0445\u043e\u0434\u0438\u0442 \u0432 \u0441\u043b\u0430\u0432\u0443.");
		break;
	case ENartyEnding::SoslanShame:
		OutTitle = NSLOCTEXT("Narty", "Obj_EndSoslanBTitle", "\u0421\u043e\u0441\u043b\u0430\u043d");
		OutBody = NSLOCTEXT("Narty", "Obj_EndSoslanB",
			"\u041e\u043d \u0436\u0438\u0432. \u041d\u043e \u0440\u043e\u0434 \u043f\u043e\u043c\u043d\u0438\u0442 \u0443\u043a\u043b\u043e\u043d\u0435\u043d\u0438\u0435.");
		break;
	case ENartyEnding::BatrazMercy:
		OutTitle = NSLOCTEXT("Narty", "Obj_EndBatrazATitle", "\u0411\u0430\u0442\u0440\u0430\u0437");
		OutBody = NSLOCTEXT("Narty", "Obj_EndBatrazA",
			"\u041c\u0435\u0447 \u0443\u0448\u0451\u043b \u0432 \u043c\u043e\u0440\u0435. \u0420\u043e\u0434 \u043f\u043e\u043b\u0443\u0447\u0430\u0435\u0442 \u043e\u0442\u0441\u0440\u043e\u0447\u043a\u0443.");
		break;
	case ENartyEnding::BatrazStorm:
		OutTitle = NSLOCTEXT("Narty", "Obj_EndBatrazBTitle", "\u0411\u0430\u0442\u0440\u0430\u0437");
		OutBody = NSLOCTEXT("Narty", "Obj_EndBatrazB",
			"\u0423\u0434\u0430\u0440 \u043f\u043e \u043d\u0435\u0431\u0443. \u041c\u043e\u0440\u0435 \u0432\u0441\u043a\u0438\u043f\u0430\u0435\u0442.");
		break;
	case ENartyEnding::SyrdonPeace:
		OutTitle = NSLOCTEXT("Narty", "Obj_EndSyrdonATitle", "\u0421\u044b\u0440\u0434\u043e\u043d");
		OutBody = NSLOCTEXT("Narty", "Obj_EndSyrdonA",
			"\u041f\u0440\u0430\u0432\u0434\u0430 \u043d\u0430 \u043d\u044b\u0445\u0430\u0441\u0435. \u0425\u0440\u0443\u043f\u043a\u0438\u0439 \u043c\u0438\u0440.");
		break;
	case ENartyEnding::SyrdonLie:
		OutTitle = NSLOCTEXT("Narty", "Obj_EndSyrdonBTitle", "\u0421\u044b\u0440\u0434\u043e\u043d");
		OutBody = NSLOCTEXT("Narty", "Obj_EndSyrdonB",
			"\u0421\u043b\u0430\u0432\u0430 \u043d\u0430 \u043b\u0436\u0438. \u0427\u0430\u0448\u0430 \u0431\u043e\u043b\u044c\u0448\u0435 \u043d\u0435 \u0432\u0441\u043a\u0438\u043f\u0438\u0442.");
		break;
	default:
		break;
	}
}

void UNartyGameInstance::StartFireQuest()
{
	QuestStage = ENartyQuestStage::FetchFire;
	bHasMountainFire = false;

	if (!PrototypeArena)
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(GetWorld());
	}

	if (PrototypeArena)
	{
		if (ANartyMountainFireActor* Fire = PrototypeArena->GetMountainFire())
		{
			Fire->ActivateForQuest();
		}
	}

	UpdateObjectiveUI();
	UE_LOG(LogTemp, Warning, TEXT("Narty: fire quest started"));
}

void UNartyGameInstance::NotifyFireTaken()
{
	bHasMountainFire = true;
	QuestStage = ENartyQuestStage::ReturnFire;

	if (PrototypeArena)
	{
		if (ANartySettlementHearthActor* Hearth = PrototypeArena->GetHearth())
		{
			Hearth->ActivateReturnObjective();
		}
	}

	UpdateObjectiveUI();
	UE_LOG(LogTemp, Warning, TEXT("Narty: fire taken — return to hearth"));
}

void UNartyGameInstance::NotifyFireReturned()
{
	if (!bHasMountainFire || QuestStage != ENartyQuestStage::ReturnFire)
	{
		return;
	}

	bHasMountainFire = false;

	if (PrototypeArena)
	{
		if (ANartySettlementHearthActor* Hearth = PrototypeArena->GetHearth())
		{
			Hearth->CompleteWithFire();
		}
	}

	StartUatsamongaQuest();
	UE_LOG(LogTemp, Warning, TEXT("Narty: fire returned — starting Uatsamonga"));
}

void UNartyGameInstance::StartUatsamongaQuest()
{
	QuestStage = ENartyQuestStage::Uatsamonga;

	if (!PrototypeArena)
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(GetWorld());
	}

	if (PrototypeArena)
	{
		if (ANartyUatsamongaCup* Cup = PrototypeArena->GetUatsamongaCup())
		{
			Cup->ActivateForQuest();
		}
	}

	UpdateObjectiveUI();
}

void UNartyGameInstance::NotifyUatsamongaResolved(bool bToldTruth)
{
	bToldTruthAtCup = bToldTruth;
	bClanFeudStarted = true;
	StartHeroTrial();
	UE_LOG(LogTemp, Warning, TEXT("Narty: Uatsamonga done, truth=%d — hero trial starts"), bToldTruth ? 1 : 0);
}

void UNartyGameInstance::StartHeroTrial()
{
	QuestStage = ENartyQuestStage::HeroTrial;

	if (!PrototypeArena)
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(GetWorld());
	}

	if (PrototypeArena)
	{
		if (ANartyHeroTrialSite* Trial = PrototypeArena->GetHeroTrialSite())
		{
			Trial->ActivateForHero(SelectedHero);
		}
	}

	UpdateObjectiveUI();
}

void UNartyGameInstance::NotifyHeroTrialCompleted(bool bNoblePath)
{
	bNobleTrialPath = bNoblePath;
	StartFinale();
	UE_LOG(LogTemp, Warning, TEXT("Narty: hero trial done noble=%d — finale"), bNoblePath ? 1 : 0);
}

void UNartyGameInstance::StartFinale()
{
	QuestStage = ENartyQuestStage::Finale;

	if (!PrototypeArena)
	{
		PrototypeArena = ANartyPrototypeArena::EnsureInWorld(GetWorld());
	}

	if (PrototypeArena)
	{
		if (ANartyFinaleSite* Finale = PrototypeArena->GetFinaleSite())
		{
			Finale->ActivateFinale();
		}
	}

	UpdateObjectiveUI();
}

void UNartyGameInstance::NotifyFinaleCompleted(ENartyEnding Ending)
{
	ChosenEnding = Ending;
	QuestStage = ENartyQuestStage::Completed;
	UpdateObjectiveUI();
	ShowEndingScreen(Ending);
	UE_LOG(LogTemp, Warning, TEXT("Narty: finale completed ending=%d"), static_cast<int32>(Ending));
}

void UNartyGameInstance::ShowEndingScreen(ENartyEnding Ending)
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	FText Title;
	FText Body;
	GetEndingTexts(Ending, Title, Body);

	if (!IsValid(EndingWidget))
	{
		EndingWidget = CreateWidget<UNartyEndingWidget>(PC, UNartyEndingWidget::StaticClass());
		if (EndingWidget)
		{
			EndingWidget->OnPlayAgain.AddDynamic(this, &UNartyGameInstance::HandlePlayAgain);
		}
	}

	if (!IsValid(EndingWidget))
	{
		return;
	}

	EndingWidget->SetEndingTexts(Title, Body);
	if (!EndingWidget->IsInViewport())
	{
		EndingWidget->AddToViewport(2000);
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetWidgetToFocus(EndingWidget->TakeWidget());
	PC->SetInputMode(Mode);

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->DisableInput(PC);
	}
}

void UNartyGameInstance::HideEndingScreen()
{
	if (IsValid(EndingWidget))
	{
		EndingWidget->RemoveFromParent();
	}
	EndingWidget = nullptr;

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);

			if (APawn* Pawn = PC->GetPawn())
			{
				Pawn->EnableInput(PC);
			}
		}
	}
}

void UNartyGameInstance::HandlePlayAgain()
{
	RestartCampaign();
}

void UNartyGameInstance::RestartCampaign()
{
	// Always restore input first — OpenLevel may fail or World may be missing.
	HideEndingScreen();
	ResetCampaignState();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Narty: RestartCampaign aborted — no world"));
		return;
	}

	bRestartPending = true;
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(World, true);
	UGameplayStatics::OpenLevel(World, FName(*LevelName));
}
