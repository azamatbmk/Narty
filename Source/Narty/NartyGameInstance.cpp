#include "NartyGameInstance.h"

#include "NartyHeroSelectWidget.h"
#include "NartyObjectiveWidget.h"
#include "NartyEndingWidget.h"
#include "NartyHealthWidget.h"
#include "NartyPauseMenuWidget.h"
#include "NartySaveGame.h"
#include "NartyPauseComponent.h"
#include "NartyGorgeLayout.h"
#include "NartyHealthComponent.h"
#include "NartyMountainFireActor.h"
#include "NartySettlementHearthActor.h"
#include "NartyUatsamongaCup.h"
#include "NartyHeroTrialSite.h"
#include "NartyFinaleSite.h"
#include "NartyForgeActor.h"
#include "NartyCombatComponent.h"
#include "NartyInteractComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

const FString UNartyGameInstance::SaveSlotName = TEXT("NartyCampaign");
const int32 UNartyGameInstance::SaveUserIndex = 0;

void UNartyGameInstance::OnStart()
{
	Super::OnStart();
	ResetCampaignState();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyGameInstance::BootstrapGorge));
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyGameInstance::BootstrapGorge));
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

	if (IsValid(HealthWidget))
	{
		HealthWidget->RemoveFromParent();
	}
	HealthWidget = nullptr;
	bPlayerHealthBound = false;
	bPlayerAbilityBound = false;

	HideEndingScreen();
	HidePauseMenu();
	PauseMenuWidget = nullptr;
	bPauseMenuDelegatesBound = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HeroSelectRetryHandle);
		World->GetTimerManager().ClearTimer(PlayerReadyRetryHandle);
		World->GetTimerManager().ClearTimer(ApplyHeroRetryHandle);
		World->GetTimerManager().ClearTimer(RespawnHandle);
		World->GetTimerManager().ClearTimer(FallCatchHandle);
		World->GetTimerManager().ClearTimer(PauseStatusClearHandle);
	}

	bPauseMenuOpen = false;
	bPendingForgeWeapon = false;
	PendingLoadHealth = -1.f;
	bApplyPendingLoadHealth = false;
	bPendingPlayerTransform = false;
	PendingFireGuardiansAlive = -1;
	PendingTrialState = FNartyTrialSaveState();
	PendingPlayerLocation = FVector::ZeroVector;
	PendingPlayerRotation = FRotator::ZeroRotator;
}

void UNartyGameInstance::ResetLocalPlayerForNewRun()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	Pawn->SetActorTransform(
		NartyGorgeLayout::FindNykhasStart(World),
		false,
		nullptr,
		ETeleportType::ResetPhysics);

	if (UNartyCombatComponent* Combat = Pawn->FindComponentByClass<UNartyCombatComponent>())
	{
		Combat->ResetForNewRun();
	}
	if (UNartyHealthComponent* Health = Pawn->FindComponentByClass<UNartyHealthComponent>())
	{
		Health->ResetToFull(0.f);
	}
	if (UNartyInteractComponent* Interact = Pawn->FindComponentByClass<UNartyInteractComponent>())
	{
		Interact->ClearFocus();
	}

	Pawn->SetActorScale3D(FVector(1.f));
}

void UNartyGameInstance::BootstrapGorge()
{
	if (UWorld* World = GetWorld())
	{
		NartyGorgeLayout::SpawnMissingStoryActors(World);
		NartyGorgeLayout::EnsureSafetyGeometry(World);
		NartyGorgeLayout::ResetWorldForNewRun(World);
		NartyGorgeLayout::EnsureReadableWorldLighting(World);
	}

	ResetLocalPlayerForNewRun();
	EnsurePlayerReady();
	StartFallCatch();
	ShowHeroSelectMenu();
}

void UNartyGameInstance::EnsurePlayerReady()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->GetPawn())
	{
		World->GetTimerManager().SetTimer(
			PlayerReadyRetryHandle,
			this,
			&UNartyGameInstance::EnsurePlayerReady,
			0.1f,
			false);
		return;
	}

	if (ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
	{
		UNartyInteractComponent::EnsureOn(Character);
		UNartyPauseComponent::EnsureOn(Character);
	}

	StartFallCatch();
}

void UNartyGameInstance::StartFallCatch()
{
	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(FallCatchHandle))
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		FallCatchHandle,
		this,
		&UNartyGameInstance::CheckFallenOutOfWorld,
		0.2f,
		true);
}

void UNartyGameInstance::CheckFallenOutOfWorld()
{
	if (QuestStage == ENartyQuestStage::Completed)
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	if (Pawn->GetActorLocation().Z > -800.f)
	{
		return;
	}

	if (World->GetTimerManager().IsTimerActive(RespawnHandle))
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: fell out of gorge — return to nykhas"));
	RespawnPlayerAtNykhas();
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
		Stats.JumpZVelocity = 920.f;
		Stats.Tint = FLinearColor(1.f, 0.85f, 0.35f);
		Stats.MaxHealth = 100.f;
		Stats.DisplayName = NSLOCTEXT("Narty", "Hero_Soslan", "\u0421\u043e\u0441\u043b\u0430\u043d");
		break;

	case ENartyHero::Batraz:
		Stats.MaxWalkSpeed = 450.f;
		Stats.JumpZVelocity = 520.f;
		Stats.Tint = FLinearColor(0.55f, 0.6f, 0.7f);
		Stats.MaxHealth = 140.f;
		Stats.DisplayName = NSLOCTEXT("Narty", "Hero_Batraz", "\u0411\u0430\u0442\u0440\u0430\u0437");
		break;

	case ENartyHero::Syrdon:
		Stats.MaxWalkSpeed = 560.f;
		Stats.JumpZVelocity = 650.f;
		Stats.Tint = FLinearColor(0.45f, 0.75f, 0.55f);
		Stats.MaxHealth = 85.f;
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
			HeroSelectWidget->OnContinueRequested.AddDynamic(this, &UNartyGameInstance::HandleContinueCampaign);
		}
	}

	if (IsValid(HeroSelectWidget))
	{
		HeroSelectWidget->SetContinueVisible(HasSaveGame());
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
	EnsurePlayerReady();
	ApplySelectedHeroToLocalPawn();
}

void UNartyGameInstance::HandleContinueCampaign()
{
	if (!LoadCampaign())
	{
		DeleteSaveGame();
		if (IsValid(HeroSelectWidget))
		{
			HeroSelectWidget->SetContinueVisible(false);
		}
		UE_LOG(LogTemp, Warning, TEXT("Narty: continue failed — save removed"));
		return;
	}

	HideHeroSelectMenu();
	EnsurePlayerReady();
	ApplySelectedHeroToLocalPawn();
	SyncWorldToLoadedState();
	ApplyPendingPlayerTransform();

	if (QuestStage == ENartyQuestStage::Completed)
	{
		ShowEndingScreen(ChosenEnding);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: campaign loaded hero=%d stage=%d"),
		static_cast<int32>(SelectedHero), static_cast<int32>(QuestStage));
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

	if (!bPlayerAbilityBound)
	{
		Combat->OnAbilityChanged.AddDynamic(this, &UNartyGameInstance::HandleAbilityChanged);
		bPlayerAbilityBound = true;
	}
	Combat->NotifyAbilityHud();

	UNartyHealthComponent* Health = HeroCharacter->FindComponentByClass<UNartyHealthComponent>();
	const bool bCreatedHealth = (Health == nullptr);
	if (!Health)
	{
		Health = NewObject<UNartyHealthComponent>(HeroCharacter, TEXT("NartyHealth"));
		Health->RegisterComponent();
		HeroCharacter->AddInstanceComponent(Health);
	}
	Health->HitInvulnSeconds = 0.4f;
	Health->SetMaxHealth(Stats.MaxHealth, bCreatedHealth);

	if (bApplyPendingLoadHealth)
	{
		Health->SetHealth(PendingLoadHealth);
		bApplyPendingLoadHealth = false;
		PendingLoadHealth = -1.f;
	}

	if (!bPlayerHealthBound)
	{
		Health->OnDied.AddDynamic(this, &UNartyGameInstance::HandlePlayerDied);
		Health->OnHealthChanged.AddDynamic(this, &UNartyGameInstance::HandlePlayerHealthChanged);
		bPlayerHealthBound = true;
	}

	UNartyInteractComponent::EnsureOn(HeroCharacter);
	UNartyPauseComponent::EnsureOn(HeroCharacter);

	if (bPendingForgeWeapon)
	{
		if (UNartyCombatComponent* CombatComp = HeroCharacter->FindComponentByClass<UNartyCombatComponent>())
		{
			CombatComp->GrantForgeWeapon();
		}
		bPendingForgeWeapon = false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: hero applied -> %s (speed=%.0f hp=%.0f)"),
		*Stats.DisplayName.ToString(), Stats.MaxWalkSpeed, Stats.MaxHealth);

	EnsureObjectiveWidget();
	EnsureHealthWidget();
	UpdateObjectiveUI();
	HandlePlayerHealthChanged(Health->GetHealth(), Health->GetMaxHealth());
}

void UNartyGameInstance::EnsureHealthWidget()
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

	if (!IsValid(HealthWidget))
	{
		HealthWidget = CreateWidget<UNartyHealthWidget>(PC, UNartyHealthWidget::StaticClass());
	}

	if (IsValid(HealthWidget) && !HealthWidget->IsInViewport())
	{
		HealthWidget->AddToViewport(40);
	}
}

void UNartyGameInstance::HandleAbilityChanged(FText AbilityLine)
{
	EnsureHealthWidget();
	if (HealthWidget)
	{
		HealthWidget->SetAbilityLine(AbilityLine);
	}
}

void UNartyGameInstance::HandlePlayerHealthChanged(float Health, float MaxHealth)
{
	EnsureHealthWidget();
	if (HealthWidget)
	{
		HealthWidget->SetHealth(Health, MaxHealth);
	}
}

void UNartyGameInstance::HandlePlayerDied(AActor* DeadActor, AActor* /*Killer*/)
{
	if (QuestStage == ENartyQuestStage::Completed)
	{
		return;
	}

	if (bPauseMenuOpen)
	{
		HidePauseMenu();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		APawn* Pawn = Cast<APawn>(DeadActor);
		if (!Pawn)
		{
			Pawn = PC->GetPawn();
		}
		if (Pawn)
		{
			Pawn->DisableInput(PC);
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
				{
					Move->StopMovementImmediately();
					Move->DisableMovement();
				}
			}

			if (UNartyCombatComponent* Combat = Pawn->FindComponentByClass<UNartyCombatComponent>())
			{
				Combat->CancelPendingStrike();
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: player died — respawn at nykhas"));
	World->GetTimerManager().SetTimer(
		RespawnHandle,
		this,
		&UNartyGameInstance::RespawnPlayerAtNykhas,
		0.45f,
		false);
}

void UNartyGameInstance::RespawnPlayerAtNykhas()
{
	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!World || !PC || !Pawn)
	{
		return;
	}

	const FTransform Start = NartyGorgeLayout::FindNykhasStart(World);
	Pawn->SetActorTransform(Start, false, nullptr, ETeleportType::TeleportPhysics);
	PC->SetControlRotation(Start.Rotator());

	if (ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->SetMovementMode(MOVE_Walking);
			if (HasSelectedHero())
			{
				const FNartyHeroStats Stats = GetSelectedHeroStats();
				Move->MaxWalkSpeed = Stats.MaxWalkSpeed;
				Move->JumpZVelocity = Stats.JumpZVelocity;
			}
		}
	}

	if (UNartyHealthComponent* Health = Pawn->FindComponentByClass<UNartyHealthComponent>())
	{
		Health->ResetToFull(1.6f);
	}

	Pawn->EnableInput(PC);
	EnsurePlayerReady();
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
					"\u0412\u044b\u0439\u0434\u0438 \u0432 \u043f\u0440\u043e\u0445\u043e\u0434 \u0441\u043f\u0440\u0430\u0432\u0430 \u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430. \u041f\u0440\u043e\u0439\u0434\u0438 \u0442\u0440\u0438 \u0432\u0438\u0434\u0435\u043d\u0438\u044f."));
			break;
		case ENartyHero::Batraz:
			ObjectiveWidget->SetObjective(
				NSLOCTEXT("Narty", "Obj_TrialBatrazTitle", "\u041a\u0440\u043e\u0432\u044c \u0440\u043e\u0434\u0430"),
				NSLOCTEXT("Narty", "Obj_TrialBatrazDetail",
					"\u0412\u044b\u0439\u0434\u0438 \u0432 \u043f\u0440\u043e\u0445\u043e\u0434 \u0441\u043f\u0440\u0430\u0432\u0430 \u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430. \u0421\u0440\u0430\u0437\u0438 \u0432\u0440\u0430\u0433\u043e\u0432 \u0440\u043e\u0434\u0430 \u0438 \u0440\u0435\u0448\u0438 \u0441\u0443\u0434\u044c\u0431\u0443 \u044f\u0440\u043e\u0441\u0442\u0438."));
			break;
		case ENartyHero::Syrdon:
			ObjectiveWidget->SetObjective(
				NSLOCTEXT("Narty", "Obj_TrialSyrdonTitle", "\u0428\u0451\u043f\u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430"),
				NSLOCTEXT("Narty", "Obj_TrialSyrdonDetail",
					"\u0412\u044b\u0439\u0434\u0438 \u0432 \u043f\u0440\u043e\u0445\u043e\u0434 \u0441\u043f\u0440\u0430\u0432\u0430 \u043e\u0442 \u043d\u044b\u0445\u0430\u0441\u0430. \u0412\u044b\u0441\u043b\u0443\u0448\u0430\u0439 \u043e\u0431\u0430 \u0433\u043e\u043b\u043e\u0441\u0430 \u0438 \u0432\u044b\u0431\u0435\u0440\u0438 \u0441\u043b\u043e\u0432\u043e \u0441\u043e\u0432\u0435\u0442\u0443."));
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

	if (ANartyMountainFireActor* Fire = NartyGorgeLayout::GetMountainFire(GetWorld()))
	{
		Fire->ActivateForQuest();
	}

	UpdateObjectiveUI();
	UE_LOG(LogTemp, Warning, TEXT("Narty: fire quest started"));
	TryAutoSaveCampaign();
}

void UNartyGameInstance::NotifyFireTaken()
{
	bHasMountainFire = true;
	QuestStage = ENartyQuestStage::ReturnFire;

	if (ANartySettlementHearthActor* Hearth = NartyGorgeLayout::GetHearth(GetWorld()))
	{
		Hearth->ActivateReturnObjective();
	}

	UpdateObjectiveUI();
	UE_LOG(LogTemp, Warning, TEXT("Narty: fire taken — return to hearth"));
	TryAutoSaveCampaign();
}

void UNartyGameInstance::NotifyFireReturned()
{
	if (!bHasMountainFire || QuestStage != ENartyQuestStage::ReturnFire)
	{
		return;
	}

	bHasMountainFire = false;

	if (ANartySettlementHearthActor* Hearth = NartyGorgeLayout::GetHearth(GetWorld()))
	{
		Hearth->CompleteWithFire();
	}

	StartUatsamongaQuest();
	UE_LOG(LogTemp, Warning, TEXT("Narty: fire returned — starting Uatsamonga"));
	TryAutoSaveCampaign();
}

void UNartyGameInstance::StartUatsamongaQuest()
{
	QuestStage = ENartyQuestStage::Uatsamonga;

	if (ANartyUatsamongaCup* Cup = NartyGorgeLayout::GetUatsamongaCup(GetWorld()))
	{
		Cup->ActivateForQuest();
	}

	UpdateObjectiveUI();
	TryAutoSaveCampaign();
}

void UNartyGameInstance::NotifyUatsamongaResolved(bool bToldTruth)
{
	bToldTruthAtCup = bToldTruth;
	bClanFeudStarted = true;
	StartHeroTrial();
	UE_LOG(LogTemp, Warning, TEXT("Narty: Uatsamonga done, truth=%d — hero trial starts"), bToldTruth ? 1 : 0);
	TryAutoSaveCampaign();
}

void UNartyGameInstance::StartHeroTrial()
{
	QuestStage = ENartyQuestStage::HeroTrial;

	if (ANartyHeroTrialSite* Trial = NartyGorgeLayout::GetHeroTrialSite(GetWorld()))
	{
		Trial->ActivateForHero(SelectedHero);
	}

	UpdateObjectiveUI();
	TryAutoSaveCampaign();
}

void UNartyGameInstance::NotifyHeroTrialCompleted(bool bNoblePath)
{
	bNobleTrialPath = bNoblePath;
	StartFinale();
	UE_LOG(LogTemp, Warning, TEXT("Narty: hero trial done noble=%d — finale"), bNoblePath ? 1 : 0);
	TryAutoSaveCampaign();
}

void UNartyGameInstance::StartFinale()
{
	QuestStage = ENartyQuestStage::Finale;

	if (ANartyFinaleSite* Finale = NartyGorgeLayout::GetFinaleSite(GetWorld()))
	{
		Finale->ActivateFinale();
	}

	UpdateObjectiveUI();
	TryAutoSaveCampaign();
}

void UNartyGameInstance::NotifyFinaleCompleted(ENartyEnding Ending)
{
	ChosenEnding = Ending;
	QuestStage = ENartyQuestStage::Completed;
	UpdateObjectiveUI();
	ShowEndingScreen(Ending);
	UE_LOG(LogTemp, Warning, TEXT("Narty: finale completed ending=%d"), static_cast<int32>(Ending));
	TryAutoSaveCampaign();
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

bool UNartyGameInstance::HasSaveGame() const
{
	return UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex);
}

bool UNartyGameInstance::PopulateSaveGame(UNartySaveGame* Save) const
{
	if (!Save || !HasSelectedHero())
	{
		return false;
	}

	Save->SaveVersion = UNartySaveGame::CurrentVersion;
	Save->SelectedHero = SelectedHero;
	Save->QuestStage = QuestStage;
	Save->bHasMountainFire = bHasMountainFire;
	Save->bToldTruthAtCup = bToldTruthAtCup;
	Save->bClanFeudStarted = bClanFeudStarted;
	Save->bNobleTrialPath = bNobleTrialPath;
	Save->bHasForgeWeapon = false;
	Save->ChosenEnding = ChosenEnding;
	Save->PlayerHealth = 0.f;
	Save->bHasPlayerTransform = false;
	Save->FireGuardiansAlive = -1;
	Save->TrialState = FNartyTrialSaveState();

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (const UNartyCombatComponent* Combat = Pawn->FindComponentByClass<UNartyCombatComponent>())
				{
					Save->bHasForgeWeapon = Combat->HasForgeWeapon();
				}

				if (const UNartyHealthComponent* Health = Pawn->FindComponentByClass<UNartyHealthComponent>())
				{
					Save->PlayerHealth = Health->GetHealth();
				}

				Save->bHasPlayerTransform = true;
				Save->PlayerLocation = Pawn->GetActorLocation();
				Save->PlayerRotation = Pawn->GetActorRotation();
			}
		}

		if (ANartyMountainFireActor* Fire = NartyGorgeLayout::GetMountainFire(World))
		{
			Save->FireGuardiansAlive = Fire->CaptureGuardiansAlive();
		}

		if (QuestStage >= ENartyQuestStage::HeroTrial)
		{
			if (ANartyHeroTrialSite* Trial = NartyGorgeLayout::GetHeroTrialSite(World))
			{
				Trial->CaptureSaveState(Save->TrialState);
			}
		}
	}

	return true;
}

bool UNartyGameInstance::ApplySaveGame(const UNartySaveGame* Save)
{
	if (!Save)
	{
		return false;
	}

	if (Save->SaveVersion != 1 && Save->SaveVersion != UNartySaveGame::CurrentVersion)
	{
		return false;
	}

	if (Save->SelectedHero == ENartyHero::None || Save->QuestStage == ENartyQuestStage::None)
	{
		return false;
	}

	static const ENartyQuestStage ValidStages[] = {
		ENartyQuestStage::GetWeapon,
		ENartyQuestStage::FetchFire,
		ENartyQuestStage::ReturnFire,
		ENartyQuestStage::Uatsamonga,
		ENartyQuestStage::HeroTrial,
		ENartyQuestStage::Finale,
		ENartyQuestStage::Completed
	};
	bool bValidStage = false;
	for (ENartyQuestStage Stage : ValidStages)
	{
		if (Save->QuestStage == Stage)
		{
			bValidStage = true;
			break;
		}
	}
	if (!bValidStage)
	{
		return false;
	}

	SelectedHero = Save->SelectedHero;
	QuestStage = Save->QuestStage;
	bHasMountainFire = Save->bHasMountainFire;
	bToldTruthAtCup = Save->bToldTruthAtCup;
	bClanFeudStarted = Save->bClanFeudStarted;
	bNobleTrialPath = Save->bNobleTrialPath;
	ChosenEnding = Save->ChosenEnding;
	bPendingForgeWeapon = Save->bHasForgeWeapon;
	PendingLoadHealth = Save->PlayerHealth;
	bApplyPendingLoadHealth = true;

	if (Save->SaveVersion >= 2)
	{
		bPendingPlayerTransform = Save->bHasPlayerTransform;
		PendingPlayerLocation = Save->PlayerLocation;
		PendingPlayerRotation = Save->PlayerRotation;
		PendingFireGuardiansAlive = Save->FireGuardiansAlive;
		PendingTrialState = Save->TrialState;
	}
	else
	{
		bPendingPlayerTransform = false;
		PendingFireGuardiansAlive = -1;
		PendingTrialState = FNartyTrialSaveState();
	}

	return true;
}

bool UNartyGameInstance::SaveCampaign()
{
	if (!HasSelectedHero())
	{
		return false;
	}

	UNartySaveGame* Save = Cast<UNartySaveGame>(
		UGameplayStatics::CreateSaveGameObject(UNartySaveGame::StaticClass()));
	if (!Save || !PopulateSaveGame(Save))
	{
		return false;
	}

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, SaveUserIndex);
	UE_LOG(LogTemp, Warning, TEXT("Narty: campaign save %s"), bSaved ? TEXT("ok") : TEXT("failed"));
	return bSaved;
}

bool UNartyGameInstance::LoadCampaign()
{
	if (!HasSaveGame())
	{
		return false;
	}

	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex);
	const UNartySaveGame* Save = Cast<UNartySaveGame>(Loaded);
	if (!Save || !ApplySaveGame(Save))
	{
		UE_LOG(LogTemp, Warning, TEXT("Narty: corrupt or incompatible save — ignoring slot"));
		return false;
	}
	return true;
}

void UNartyGameInstance::DeleteSaveGame()
{
	if (HasSaveGame())
	{
		UGameplayStatics::DeleteGameInSlot(SaveSlotName, SaveUserIndex);
	}
}

void UNartyGameInstance::TryAutoSaveCampaign()
{
	if (!HasSelectedHero() || QuestStage == ENartyQuestStage::None || QuestStage == ENartyQuestStage::GetWeapon)
	{
		return;
	}

	SaveCampaign();
}

void UNartyGameInstance::ApplyPendingPlayerTransform()
{
	if (!bPendingPlayerTransform)
	{
		return;
	}

	bPendingPlayerTransform = false;

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	Pawn->SetActorLocation(PendingPlayerLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Pawn->SetActorRotation(PendingPlayerRotation, ETeleportType::TeleportPhysics);
	PC->SetControlRotation(PendingPlayerRotation);
}

void UNartyGameInstance::SyncWorldToLoadedState()
{
	UWorld* World = GetWorld();
	if (!World || !HasSelectedHero())
	{
		return;
	}

	bool bForgeDone = QuestStage != ENartyQuestStage::GetWeapon;
	if (!bForgeDone)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (const UNartyCombatComponent* Combat = Pawn->FindComponentByClass<UNartyCombatComponent>())
				{
					bForgeDone = Combat->HasForgeWeapon();
				}
			}
		}
	}

	if (ANartyForgeActor* Forge = NartyGorgeLayout::GetForge(World))
	{
		if (bForgeDone)
		{
			Forge->MarkWeaponGranted();
		}
	}

	if (ANartyMountainFireActor* Fire = NartyGorgeLayout::GetMountainFire(World))
	{
		const bool bTaken = QuestStage > ENartyQuestStage::FetchFire
			|| (QuestStage == ENartyQuestStage::ReturnFire && bHasMountainFire);
		const bool bActive = QuestStage == ENartyQuestStage::FetchFire
			|| (QuestStage == ENartyQuestStage::ReturnFire && bHasMountainFire);
		Fire->RestoreFromSave(bActive, bTaken, PendingFireGuardiansAlive);
	}

	if (ANartySettlementHearthActor* Hearth = NartyGorgeLayout::GetHearth(World))
	{
		const bool bReturn = QuestStage == ENartyQuestStage::ReturnFire && bHasMountainFire;
		const bool bDone = QuestStage > ENartyQuestStage::ReturnFire;
		Hearth->RestoreFromSave(bReturn, bDone);
	}

	if (ANartyUatsamongaCup* Cup = NartyGorgeLayout::GetUatsamongaCup(World))
	{
		const bool bActive = QuestStage == ENartyQuestStage::Uatsamonga;
		const bool bJudged = QuestStage > ENartyQuestStage::Uatsamonga || bClanFeudStarted;
		Cup->RestoreFromSave(bActive, bJudged, bToldTruthAtCup);
	}

	if (ANartyHeroTrialSite* Trial = NartyGorgeLayout::GetHeroTrialSite(World))
	{
		const bool bActive = QuestStage == ENartyQuestStage::HeroTrial;
		const bool bDone = QuestStage > ENartyQuestStage::HeroTrial;
		if (bActive || bDone)
		{
			Trial->RestoreFromSave(SelectedHero, bActive, bDone, PendingTrialState);
		}
	}

	if (ANartyFinaleSite* Finale = NartyGorgeLayout::GetFinaleSite(World))
	{
		const bool bActive = QuestStage == ENartyQuestStage::Finale;
		const bool bDone = QuestStage == ENartyQuestStage::Completed;
		if (bActive || bDone)
		{
			Finale->RestoreFromSave(bActive, bDone);
		}
	}

	UpdateObjectiveUI();
}

bool UNartyGameInstance::CanOpenPauseMenu() const
{
	if (!HasSelectedHero() || bPauseMenuOpen)
	{
		return false;
	}

	if (IsValid(HeroSelectWidget) && HeroSelectWidget->IsInViewport())
	{
		return false;
	}

	if (IsValid(EndingWidget) && EndingWidget->IsInViewport())
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (const UNartyHealthComponent* Health = Pawn->FindComponentByClass<UNartyHealthComponent>())
				{
					if (Health->IsDead())
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

void UNartyGameInstance::TogglePauseMenu()
{
	const double Now = FPlatformTime::Seconds();
	if (Now - LastPauseToggleSeconds < 0.2)
	{
		return;
	}
	LastPauseToggleSeconds = Now;

	if (bPauseMenuOpen)
	{
		HidePauseMenu();
	}
	else if (CanOpenPauseMenu())
	{
		ShowPauseMenu();
	}
}

void UNartyGameInstance::ShowPauseMenu()
{
	if (bPauseMenuOpen || !CanOpenPauseMenu())
	{
		return;
	}

	UWorld* World = GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return;
	}

	if (!IsValid(PauseMenuWidget))
	{
		PauseMenuWidget = CreateWidget<UNartyPauseMenuWidget>(PC, UNartyPauseMenuWidget::StaticClass());
		if (PauseMenuWidget && !bPauseMenuDelegatesBound)
		{
			PauseMenuWidget->OnResume.AddDynamic(this, &UNartyGameInstance::HandlePauseResume);
			PauseMenuWidget->OnSave.AddDynamic(this, &UNartyGameInstance::HandlePauseSave);
			PauseMenuWidget->OnNewGame.AddDynamic(this, &UNartyGameInstance::HandlePauseNewGame);
			PauseMenuWidget->OnQuit.AddDynamic(this, &UNartyGameInstance::HandlePauseQuit);
			bPauseMenuDelegatesBound = true;
		}
	}

	if (!IsValid(PauseMenuWidget))
	{
		return;
	}

	PauseMenuWidget->SetStatusText(FText::GetEmpty());
	if (!PauseMenuWidget->IsInViewport())
	{
		PauseMenuWidget->AddToViewport(1500);
	}

	bPauseMenuOpen = true;
	LastPauseToggleSeconds = FPlatformTime::Seconds();

	PC->SetTickableWhenPaused(true);

	if (World)
	{
		UGameplayStatics::SetGamePaused(World, true);
	}

	PC->bShowMouseCursor = true;
	FInputModeUIOnly Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetWidgetToFocus(PauseMenuWidget->TakeWidget());
	PC->SetInputMode(Mode);

	if (APawn* Pawn = PC->GetPawn())
	{
		Pawn->DisableInput(PC);
	}
}

void UNartyGameInstance::HidePauseMenu()
{
	if (IsValid(PauseMenuWidget))
	{
		PauseMenuWidget->RemoveFromParent();
		PauseMenuWidget->SetStatusText(FText::GetEmpty());
	}

	bPauseMenuOpen = false;
	LastPauseToggleSeconds = FPlatformTime::Seconds();

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
		World->GetTimerManager().ClearTimer(PauseStatusClearHandle);

		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (IsValid(EndingWidget) && EndingWidget->IsInViewport())
			{
				return;
			}

			if (IsValid(HeroSelectWidget) && HeroSelectWidget->IsInViewport())
			{
				return;
			}

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

void UNartyGameInstance::HandlePauseResume()
{
	HidePauseMenu();
}

void UNartyGameInstance::HandlePauseSave()
{
	const bool bSaved = SaveCampaign();
	if (IsValid(PauseMenuWidget))
	{
		PauseMenuWidget->SetStatusText(bSaved
			? NSLOCTEXT("Narty", "Pause_Saved", "\u0421\u043e\u0445\u0440\u0430\u043d\u0435\u043d\u043e")
			: NSLOCTEXT("Narty", "Pause_SaveFailed", "\u041d\u0435 \u0443\u0434\u0430\u043b\u043e\u0441\u044c \u0441\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c"));

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				PauseStatusClearHandle,
				FTimerDelegate::CreateWeakLambda(PauseMenuWidget, [Widget = PauseMenuWidget]()
				{
					if (IsValid(Widget))
					{
						Widget->SetStatusText(FText::GetEmpty());
					}
				}),
				2.f,
				false);
		}
	}
}

void UNartyGameInstance::HandlePauseNewGame()
{
	HidePauseMenu();
	DeleteSaveGame();
	RestartCampaign();
}

void UNartyGameInstance::HandlePauseQuit()
{
	HidePauseMenu();

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
		}
	}
}

void UNartyGameInstance::HandlePlayAgain()
{
	DeleteSaveGame();
	RestartCampaign();
}

void UNartyGameInstance::RestartCampaign()
{
	// Always restore input first — OpenLevel may fail or World may be missing.
	HideEndingScreen();
	HidePauseMenu();
	PauseMenuWidget = nullptr;
	bPauseMenuDelegatesBound = false;
	ResetCampaignState();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("Narty: RestartCampaign aborted — no world"));
		return;
	}

	UGameplayStatics::SetGamePaused(World, false);

	// PIE often does not actually reload the same map. Reset in place instead.
	if (World->GetMapName().Contains(TEXT("Lvl_Gorge")))
	{
		NartyGorgeLayout::SpawnMissingStoryActors(World);
		NartyGorgeLayout::EnsureSafetyGeometry(World);
		NartyGorgeLayout::ResetWorldForNewRun(World);
		NartyGorgeLayout::EnsureReadableWorldLighting(World);
		ResetLocalPlayerForNewRun();
		EnsurePlayerReady();
		StartFallCatch();
		ShowHeroSelectMenu();
		UE_LOG(LogTemp, Warning, TEXT("Narty: campaign restarted in place"));
		return;
	}

	bRestartPending = true;
	UGameplayStatics::OpenLevel(
		World,
		FName(TEXT("/Game/Narty/Lvl_Gorge")),
		true,
		TEXT("game=/Script/Narty.NartyGameMode"));
}
