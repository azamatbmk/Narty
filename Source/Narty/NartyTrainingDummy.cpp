#include "NartyTrainingDummy.h"

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
		Mesh->SetWorldScale3D(FVector(0.8f, 0.8f, 1.8f));
	}

	Health = MaxHealth;
}

float ANartyTrainingDummy::ReceiveStrike(float Damage, AActor* /*InstigatorActor*/)
{
	if (Health <= 0.f)
	{
		return 0.f;
	}

	Health = FMath::Max(0.f, Health - Damage);
	RefreshLabel();

	if (Health <= 0.f)
	{
		Mesh->SetVisibility(false);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HPLabel->SetText(NSLOCTEXT("Narty", "Dummy_Down", "X"));
		OnDefeated.Broadcast(this);
		UE_LOG(LogTemp, Log, TEXT("Narty: training dummy down"));
	}

	return Damage;
}

void ANartyTrainingDummy::SetMaxHealth(float InMaxHealth)
{
	MaxHealth = InMaxHealth;
	Health = InMaxHealth;
	if (HPLabel)
	{
		RefreshLabel();
	}
}

void ANartyTrainingDummy::RefreshLabel()
{
	if (HPLabel)
	{
		HPLabel->SetText(FText::AsNumber(FMath::RoundToInt(Health)));
	}
}

void ANartyTrainingDummy::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	RefreshLabel();
}
