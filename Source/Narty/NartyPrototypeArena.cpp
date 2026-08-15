#include "NartyPrototypeArena.h"

#include "NartyForgeActor.h"
#include "NartyTrainingDummy.h"
#include "NartyMountainFireActor.h"
#include "NartySettlementHearthActor.h"
#include "NartyUatsamongaCup.h"
#include "NartyHeroTrialSite.h"
#include "NartyFinaleSite.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ANartyPrototypeArena::ANartyPrototypeArena()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CubeMesh = CubeFinder.Object;
	}

	SetActorLocation(FVector(5000.f, 0.f, 100.f));
}

ANartyPrototypeArena* ANartyPrototypeArena::EnsureInWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ANartyPrototypeArena> It(World); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ANartyPrototypeArena>(
		ANartyPrototypeArena::StaticClass(), FVector(5000.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
}

FTransform ANartyPrototypeArena::GetPlayerStartTransform() const
{
	const FVector Loc = GetActorLocation() + FVector(-900.f, 0.f, 120.f);
	return FTransform(FRotator(0.f, 0.f, 0.f), Loc);
}

void ANartyPrototypeArena::BeginPlay()
{
	Super::BeginPlay();
	BuildGeometry();
}

void ANartyPrototypeArena::BuildGeometry()
{
	if (bBuilt || !CubeMesh)
	{
		return;
	}
	bBuilt = true;

	AddBlock(Root, TEXT("Floor"), FVector(0.f, 0.f, -50.f), FVector(34.f, 8.f, 1.f), FLinearColor(0.18f, 0.16f, 0.14f));
	AddBlock(Root, TEXT("WallLeft"), FVector(0.f, -450.f, 400.f), FVector(34.f, 1.5f, 10.f), FLinearColor(0.25f, 0.22f, 0.2f));
	AddBlock(Root, TEXT("WallRight"), FVector(0.f, 450.f, 400.f), FVector(34.f, 1.5f, 10.f), FLinearColor(0.25f, 0.22f, 0.2f));
	AddBlock(Root, TEXT("WallBack"), FVector(-1400.f, 0.f, 400.f), FVector(1.5f, 10.f, 10.f), FLinearColor(0.2f, 0.18f, 0.16f));

	// Split front wall — path to mountain ledge
	AddBlock(Root, TEXT("WallFrontL"), FVector(1600.f, -280.f, 350.f), FVector(1.2f, 4.f, 8.f), FLinearColor(0.22f, 0.2f, 0.17f));
	AddBlock(Root, TEXT("WallFrontR"), FVector(1600.f, 280.f, 350.f), FVector(1.2f, 4.f, 8.f), FLinearColor(0.22f, 0.2f, 0.17f));

	AddBlock(Root, TEXT("RockA"), FVector(-400.f, -280.f, 80.f), FVector(2.f, 2.f, 3.f), FLinearColor(0.3f, 0.27f, 0.24f));
	AddBlock(Root, TEXT("RockB"), FVector(200.f, 300.f, 60.f), FVector(2.5f, 1.8f, 2.2f), FLinearColor(0.28f, 0.25f, 0.22f));
	AddBlock(Root, TEXT("RockC"), FVector(700.f, -260.f, 70.f), FVector(1.8f, 2.2f, 2.8f), FLinearColor(0.32f, 0.28f, 0.24f));

	// Stairs / ledge to mountain fire
	AddBlock(Root, TEXT("Step1"), FVector(1200.f, 0.f, 20.f), FVector(2.5f, 3.f, 0.6f), FLinearColor(0.24f, 0.21f, 0.18f));
	AddBlock(Root, TEXT("Step2"), FVector(1320.f, 0.f, 70.f), FVector(2.5f, 3.f, 0.6f), FLinearColor(0.24f, 0.21f, 0.18f));
	AddBlock(Root, TEXT("Step3"), FVector(1440.f, 0.f, 120.f), FVector(2.5f, 3.f, 0.6f), FLinearColor(0.24f, 0.21f, 0.18f));
	AddBlock(Root, TEXT("Ledge"), FVector(1580.f, 0.f, 160.f), FVector(4.f, 4.f, 0.7f), FLinearColor(0.2f, 0.18f, 0.15f));

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	const FVector ForgeLoc = GetActorLocation() + FVector(1100.f, 0.f, 40.f);
	World->SpawnActor<ANartyForgeActor>(ANartyForgeActor::StaticClass(), ForgeLoc, FRotator(0.f, 180.f, 0.f), Params);

	const FVector DummyA = GetActorLocation() + FVector(200.f, -120.f, 90.f);
	const FVector DummyB = GetActorLocation() + FVector(450.f, 140.f, 90.f);
	World->SpawnActor<ANartyTrainingDummy>(ANartyTrainingDummy::StaticClass(), DummyA, FRotator::ZeroRotator, Params);
	World->SpawnActor<ANartyTrainingDummy>(ANartyTrainingDummy::StaticClass(), DummyB, FRotator::ZeroRotator, Params);

	const FVector HearthLoc = GetActorLocation() + FVector(-750.f, 180.f, 40.f);
	Hearth = World->SpawnActor<ANartySettlementHearthActor>(
		ANartySettlementHearthActor::StaticClass(), HearthLoc, FRotator(0.f, -90.f, 0.f), Params);

	const FVector FireLoc = GetActorLocation() + FVector(1580.f, 0.f, 230.f);
	MountainFire = World->SpawnActor<ANartyMountainFireActor>(
		ANartyMountainFireActor::StaticClass(), FireLoc, FRotator(0.f, 180.f, 0.f), Params);

	// Nykhas / feast table in the gorge center
	AddBlock(Root, TEXT("NykhasFloor"), FVector(-200.f, 0.f, -20.f), FVector(6.f, 6.f, 0.4f), FLinearColor(0.16f, 0.14f, 0.12f));
	const FVector CupLoc = GetActorLocation() + FVector(-200.f, 0.f, 50.f);
	UatsamongaCup = World->SpawnActor<ANartyUatsamongaCup>(
		ANartyUatsamongaCup::StaticClass(), CupLoc, FRotator(0.f, 180.f, 0.f), Params);

	const FVector TrialLoc = GetActorLocation() + FVector(-200.f, 1100.f, 0.f);
	HeroTrialSite = World->SpawnActor<ANartyHeroTrialSite>(
		ANartyHeroTrialSite::StaticClass(), TrialLoc, FRotator(0.f, -90.f, 0.f), Params);

	// Satana sanctum near settlement entrance / hearth
	const FVector FinaleLoc = GetActorLocation() + FVector(-1100.f, -250.f, 40.f);
	FinaleSite = World->SpawnActor<ANartyFinaleSite>(
		ANartyFinaleSite::StaticClass(), FinaleLoc, FRotator(0.f, 90.f, 0.f), Params);

	UE_LOG(LogTemp, Warning, TEXT("Narty: prototype gorge arena built (full campaign skeleton)"));
}

UStaticMeshComponent* ANartyPrototypeArena::AddBlock(
	USceneComponent* Parent,
	const FName& Name,
	const FVector& RelativeLocation,
	const FVector& Scale,
	const FLinearColor& Color)
{
	UStaticMeshComponent* Block = NewObject<UStaticMeshComponent>(this, Name);
	Block->SetupAttachment(Parent);
	Block->SetStaticMesh(CubeMesh);
	Block->SetRelativeLocation(RelativeLocation);
	Block->SetWorldScale3D(Scale);
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
