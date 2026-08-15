#include "NartyCombatComponent.h"

#include "NartyTrainingDummy.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

UNartyCombatComponent::UNartyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		WeaponMeshAsset = CubeMesh.Object;
	}
}

void UNartyCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	static const TCHAR* AttackPaths[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"),
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03")
	};

	for (const TCHAR* Path : AttackPaths)
	{
		if (UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, Path))
		{
			AttackMontages.Add(Montage);
		}
		else if (UAnimSequence* Seq = LoadObject<UAnimSequence>(nullptr, Path))
		{
			AttackSequences.Add(Seq);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: loaded attack montages=%d sequences=%d"),
		AttackMontages.Num(), AttackSequences.Num());

	EnsureAttackInput();
}

void UNartyCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StrikeDelayHandle);
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	RemoveAttackMapping();
	bInputBound = false;
	Super::EndPlay(EndPlayReason);
}

void UNartyCombatComponent::RemoveAttackMapping()
{
	if (!bMappingAdded)
	{
		BoundLocalPlayer = nullptr;
		return;
	}

	if (ULocalPlayer* LP = BoundLocalPlayer.Get())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
		{
			if (AttackMappingContext)
			{
				Subsystem->RemoveMappingContext(AttackMappingContext);
			}
		}
	}

	BoundLocalPlayer = nullptr;
	bMappingAdded = false;
}

void UNartyCombatComponent::SetHero(ENartyHero InHero)
{
	Hero = InHero;

	switch (Hero)
	{
	case ENartyHero::Soslan:
		AttackRange = 150.f;
		AttackRadius = 40.f;
		AttackDamage = bHasForgeWeapon ? 40.f : 28.f;
		AttackCooldown = 0.32f;
		break;
	case ENartyHero::Batraz:
		AttackRange = 170.f;
		AttackRadius = 55.f;
		AttackDamage = bHasForgeWeapon ? 65.f : 45.f;
		AttackCooldown = 0.7f;
		break;
	case ENartyHero::Syrdon:
		AttackRange = 130.f;
		AttackRadius = 35.f;
		AttackDamage = bHasForgeWeapon ? 32.f : 20.f;
		AttackCooldown = 0.4f;
		break;
	default:
		break;
	}

	EnsureAttackInput();
}

void UNartyCombatComponent::GrantForgeWeapon()
{
	bHasForgeWeapon = true;
	SetHero(Hero);
	EnsureWeaponMesh();
	UE_LOG(LogTemp, Warning, TEXT("Narty: forge weapon granted (damage=%.0f)"), AttackDamage);
}

void UNartyCombatComponent::EnsureWeaponMesh()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || WeaponMesh)
	{
		return;
	}

	USkeletalMeshComponent* CharMesh = OwnerCharacter->GetMesh();
	if (!CharMesh)
	{
		return;
	}

	WeaponMesh = NewObject<UStaticMeshComponent>(OwnerCharacter, TEXT("NartyWeaponBlade"));
	WeaponMesh->SetupAttachment(CharMesh, TEXT("hand_r"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->RegisterComponent();
	OwnerCharacter->AddInstanceComponent(WeaponMesh);

	if (WeaponMeshAsset)
	{
		WeaponMesh->SetStaticMesh(WeaponMeshAsset);
	}

	WeaponMesh->SetRelativeLocation(FVector(8.f, 0.f, 0.f));
	WeaponMesh->SetRelativeRotation(FRotator(0.f, 0.f, 10.f));
	WeaponMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.55f));

	if (UMaterialInstanceDynamic* Dyn = WeaponMesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		const FLinearColor Metal =
			Hero == ENartyHero::Soslan ? FLinearColor(1.f, 0.75f, 0.2f) :
			Hero == ENartyHero::Batraz ? FLinearColor(0.55f, 0.6f, 0.7f) :
			FLinearColor(0.4f, 0.8f, 0.5f);
		Dyn->SetVectorParameterValue(TEXT("Color"), Metal);
		Dyn->SetVectorParameterValue(TEXT("BaseColor"), Metal);
	}
}

void UNartyCombatComponent::EnsureAttackInput()
{
	if (!AttackAction)
	{
		AttackAction = NewObject<UInputAction>(this, TEXT("IA_NartyAttack"));
		AttackAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!AttackMappingContext)
	{
		AttackMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_NartyAttack"));
		AttackMappingContext->MapKey(AttackAction, EKeys::LeftMouseButton);
		AttackMappingContext->MapKey(AttackAction, EKeys::LeftControl);
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	APlayerController* PC = OwnerCharacter ? Cast<APlayerController>(OwnerCharacter->GetController()) : nullptr;
	if (!PC)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UNartyCombatComponent::EnsureAttackInput));
		}
		return;
	}

	if (!bMappingAdded)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				Subsystem->AddMappingContext(AttackMappingContext, 1);
				BoundLocalPlayer = LP;
				bMappingAdded = true;
			}
		}
	}

	if (bInputBound || !AttackAction)
	{
		return;
	}

	UInputComponent* IC = OwnerCharacter->InputComponent;
	if (!IC)
	{
		IC = PC->InputComponent;
	}

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(IC))
	{
		EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &UNartyCombatComponent::HandleAttackStarted);
		bInputBound = true;
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyCombatComponent::EnsureAttackInput));
	}
}

void UNartyCombatComponent::HandleAttackStarted()
{
	TryAttack();
}

void UNartyCombatComponent::TryAttack()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastAttackTime < AttackCooldown)
	{
		return;
	}
	LastAttackTime = Now;

	PlayAttackAnimation();

	const float Delay = bHasForgeWeapon ? 0.18f : 0.12f;
	World->GetTimerManager().SetTimer(
		StrikeDelayHandle,
		this,
		&UNartyCombatComponent::PerformStrike,
		Delay,
		false);
}

void UNartyCombatComponent::PlayAttackAnimation()
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	if (AttackMontages.Num() > 0)
	{
		UAnimMontage* Montage = AttackMontages[AttackComboIndex % AttackMontages.Num()];
		AttackComboIndex++;
		if (Montage)
		{
			OwnerCharacter->PlayAnimMontage(Montage, bHasForgeWeapon ? 1.05f : 1.2f);
			return;
		}
	}

	if (AttackSequences.Num() > 0)
	{
		if (USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh())
		{
			if (UAnimInstance* Anim = Mesh->GetAnimInstance())
			{
				UAnimSequence* Seq = AttackSequences[AttackComboIndex % AttackSequences.Num()];
				AttackComboIndex++;
				if (Seq)
				{
					Anim->PlaySlotAnimationAsDynamicMontage(
						Seq, TEXT("DefaultSlot"), 0.08f, 0.12f, bHasForgeWeapon ? 1.05f : 1.2f);
				}
			}
		}
	}
}

void UNartyCombatComponent::PerformStrike()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	const float Range = bHasForgeWeapon ? AttackRange + 30.f : AttackRange;
	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector End = Start + OwnerActor->GetActorForwardVector() * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NartyStrike), false, OwnerActor);
	TArray<FHitResult> Hits;

	World->SweepMultiByChannel(
		Hits, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AttackRadius), Params);

	TArray<FHitResult> WorldHits;
	World->SweepMultiByChannel(
		WorldHits, Start, End, FQuat::Identity, ECC_WorldDynamic,
		FCollisionShape::MakeSphere(AttackRadius), Params);
	Hits.Append(WorldHits);

	const FColor DebugColor = bHasForgeWeapon ? FColor::Red : FColor::Orange;
#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(World, End, AttackRadius, 12, DebugColor, false, 0.2f);
	DrawDebugLine(World, Start, End, DebugColor, false, 0.2f, 0, 2.f);
#endif

	TSet<AActor*> Damaged;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || Damaged.Contains(HitActor))
		{
			continue;
		}
		Damaged.Add(HitActor);

		if (ANartyTrainingDummy* Dummy = Cast<ANartyTrainingDummy>(HitActor))
		{
			Dummy->ReceiveStrike(AttackDamage, OwnerActor);
		}
	}
}
