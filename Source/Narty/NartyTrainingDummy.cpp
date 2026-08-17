#include "NartyTrainingDummy.h"

#include "NartyHealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

ANartyTrainingDummy::ANartyTrainingDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh->SetSimulatePhysics(false);

	HPLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HPLabel"));
	HPLabel->SetupAttachment(Mesh);
	HPLabel->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	HPLabel->SetHorizontalAlignment(EHTA_Center);
	HPLabel->SetWorldSize(28.f);
	HPLabel->SetTextRenderColor(FColor::White);

	Health = CreateDefaultSubobject<UNartyHealthComponent>(TEXT("Health"));
	Health->MaxHealth = 100.f;
	Health->HitInvulnSeconds = 0.08f;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetWorldScale3D(FVector(0.8f, 0.8f, 1.8f));
	}
}

float ANartyTrainingDummy::GetHealth() const
{
	return Health ? Health->GetHealth() : 0.f;
}

float ANartyTrainingDummy::ReceiveStrike(float Damage, AActor* InstigatorActor)
{
	return Health ? Health->ApplyDamage(Damage, InstigatorActor) : 0.f;
}

void ANartyTrainingDummy::SetMaxHealth(float InMaxHealth)
{
	if (Health)
	{
		Health->SetMaxHealth(InMaxHealth, true);
	}
}

void ANartyTrainingDummy::RefreshLabel()
{
	if (HPLabel && Health)
	{
		HPLabel->SetText(FText::AsNumber(FMath::RoundToInt(Health->GetHealth())));
	}
}

void ANartyTrainingDummy::HandleHealthChanged(float /*InHealth*/, float /*InMaxHealth*/)
{
	RefreshLabel();
}

void ANartyTrainingDummy::HandleDied(AActor* /*DeadActor*/, AActor* /*Killer*/)
{
	Mesh->SetVisibility(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HPLabel->SetText(NSLOCTEXT("Narty", "Dummy_Down", "X"));
	OnDefeated.Broadcast(this);
	UE_LOG(LogTemp, Log, TEXT("Narty: training dummy down"));
}

void ANartyTrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	Health->OnHealthChanged.AddDynamic(this, &ANartyTrainingDummy::HandleHealthChanged);
	Health->OnDied.AddDynamic(this, &ANartyTrainingDummy::HandleDied);
	RefreshLabel();
}
