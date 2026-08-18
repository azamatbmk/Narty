#include "NartyUaigEnemy.h"

#include "NartyHealthComponent.h"
#include "NartyCombatComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "UObject/ConstructorHelpers.h"

ANartyUaigEnemy::ANartyUaigEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("Pawn"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(false);
	Mesh->CanCharacterStepUpOn = ECB_No;

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Mesh);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(32.f);
	Label->SetTextRenderColor(FColor(180, 160, 140));

	Health = CreateDefaultSubobject<UNartyHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 220.f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetWorldScale3D(FVector(1.15f, 1.15f, 2.6f));
	}
}

void ANartyUaigEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (UMaterialInstanceDynamic* Dyn = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		const FLinearColor Stone(0.38f, 0.28f, 0.22f);
		Dyn->SetVectorParameterValue(TEXT("Color"), Stone);
		Dyn->SetVectorParameterValue(TEXT("BaseColor"), Stone);
	}

	Health->OnHealthChanged.AddDynamic(this, &ANartyUaigEnemy::HandleHealthChanged);
	Health->OnDied.AddDynamic(this, &ANartyUaigEnemy::HandleDied);
	RefreshLabel();
}

void ANartyUaigEnemy::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Health || Health->IsDead())
	{
		return;
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return;
	}

	if (const UNartyHealthComponent* PlayerHealth = Player->FindComponentByClass<UNartyHealthComponent>())
	{
		if (PlayerHealth->IsDead())
		{
			return;
		}
	}

	FVector ToPlayer = Player->GetActorLocation() - GetActorLocation();
	ToPlayer.Z = 0.f;
	const float Distance = ToPlayer.Size();
	if (Distance > AggroRange || Distance < 1.f)
	{
		return;
	}

	if (const UNartyCombatComponent* Combat = Player->FindComponentByClass<UNartyCombatComponent>())
	{
		if (Combat->IsStealthed() && Distance > 110.f)
		{
			return;
		}
	}

	const FRotator Face = ToPlayer.Rotation();
	SetActorRotation(FRotator(0.f, Face.Yaw, 0.f));

	if (Distance > AttackRange)
	{
		const FVector Step = ToPlayer.GetSafeNormal() * WalkSpeed * DeltaSeconds;
		SetActorLocation(GetActorLocation() + Step, false);
	}
	else
	{
		TrySmash(Player);
	}
}

bool ANartyUaigEnemy::HasLineOfSightTo(const AActor* Target) const
{
	const UWorld* World = GetWorld();
	if (!World || !Target)
	{
		return false;
	}

	const FVector Start = GetActorLocation() + FVector(0.f, 0.f, 80.f);
	const FVector End = Target->GetActorLocation() + FVector(0.f, 0.f, 60.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NartyUaigLos), false, this);
	Params.AddIgnoredActor(Target);

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return Hit.GetActor() == Target;
	}

	return true;
}

void ANartyUaigEnemy::TrySmash(AActor* Target)
{
	UWorld* World = GetWorld();
	if (!World || !Target || !Health)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastSmashTime < AttackCooldown)
	{
		return;
	}
	LastSmashTime = Now;

	if (UNartyHealthComponent* TargetHealth = Target->FindComponentByClass<UNartyHealthComponent>())
	{
		TargetHealth->ApplyDamage(AttackDamage, this);
	}
}

void ANartyUaigEnemy::HandleHealthChanged(float InHealth, float /*InMaxHealth*/)
{
	if (Health && !Health->IsDead())
	{
		RefreshLabel();
	}
}

void ANartyUaigEnemy::ReviveAt(const FVector& Location)
{
	LastSmashTime = -100.f;
	SetActorLocation(Location, false, nullptr, ETeleportType::ResetPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	if (Mesh)
	{
		Mesh->SetVisibility(true, true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (Health)
	{
		Health->ResetToFull(0.f);
	}
	RefreshLabel();
}

void ANartyUaigEnemy::HandleDied(AActor* /*DeadActor*/, AActor* /*Killer*/)
{
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetVisibility(false, true);
	Label->SetText(NSLOCTEXT("Narty", "Uaig_Down", "\u0423\u0430\u0438\u0433 \u043f\u0430\u043b"));
	SetActorTickEnabled(false);
	UE_LOG(LogTemp, Warning, TEXT("Narty: uaig defeated"));
}

void ANartyUaigEnemy::RefreshLabel()
{
	if (!Label || !Health)
	{
		return;
	}

	Label->SetText(FText::Format(
		NSLOCTEXT("Narty", "Uaig_HP", "\u0423\u0430\u0438\u0433  {0}"),
		FText::AsNumber(FMath::RoundToInt(Health->GetHealth()))));
}
