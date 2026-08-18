#include "NartySettlementHearthActor.h"

#include "NartyGameInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ANartySettlementHearthActor::ANartySettlementHearthActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Bowl = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bowl"));
	SetRootComponent(Bowl);

	HearthLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("HearthLight"));
	HearthLight->SetupAttachment(Bowl);
	HearthLight->SetIntensity(1200.f);
	HearthLight->SetLightColor(FLinearColor(0.35f, 0.4f, 0.45f));
	HearthLight->SetAttenuationRadius(900.f);
	HearthLight->SetRelativeLocation(FVector(0.f, 0.f, 120.f));

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Bowl);
	Trigger->SetBoxExtent(FVector(160.f, 160.f, 100.f));
	Trigger->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Bowl);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	Label->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	Label->SetWorldSize(34.f);
	Label->SetTextRenderColor(FColor(180, 190, 200));
	Label->SetText(NSLOCTEXT("Narty", "Hearth_Label", "\u041e\u0447\u0430\u0433 \u0441\u0435\u043b\u0435\u043d\u0438\u044f"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Bowl->SetStaticMesh(CubeMesh.Object);
		Bowl->SetWorldScale3D(FVector(1.8f, 1.8f, 0.45f));
	}
}

void ANartySettlementHearthActor::BeginPlay()
{
	Super::BeginPlay();
	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ANartySettlementHearthActor::OnHearthOverlap);
}

void ANartySettlementHearthActor::ActivateReturnObjective()
{
	bReturnActive = true;
	Label->SetText(NSLOCTEXT("Narty", "Hearth_Return", "\u0412\u0435\u0440\u043d\u0438 \u043e\u0433\u043e\u043d\u044c"));
	Label->SetTextRenderColor(FColor(255, 200, 80));
	HearthLight->SetIntensity(4000.f);
	HearthLight->SetLightColor(FLinearColor(1.f, 0.55f, 0.15f));
}

void ANartySettlementHearthActor::CompleteWithFire()
{
	if (bCompleted)
	{
		return;
	}

	bCompleted = true;
	bReturnActive = false;
	Label->SetText(NSLOCTEXT("Narty", "Hearth_Lit", "\u041e\u0433\u043e\u043d\u044c \u0441\u0435\u043b\u0435\u043d\u0438\u044f"));
	Label->SetTextRenderColor(FColor(255, 220, 120));
	HearthLight->SetIntensity(16000.f);
	HearthLight->SetLightColor(FLinearColor(1.f, 0.45f, 0.08f));
	HearthLight->SetAttenuationRadius(2200.f);
}

void ANartySettlementHearthActor::RestoreFromSave(bool bReturnActive, bool bCompleted)
{
	if (bCompleted)
	{
		CompleteWithFire();
	}
	else if (bReturnActive)
	{
		ActivateReturnObjective();
	}
}

void ANartySettlementHearthActor::OnHearthOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bReturnActive || bCompleted)
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

	if (UNartyGameInstance* GI = Cast<UNartyGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		if (GI->HasMountainFire())
		{
			GI->NotifyFireReturned();
		}
	}
}
