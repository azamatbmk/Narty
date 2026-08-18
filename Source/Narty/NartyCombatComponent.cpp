#include "NartyCombatComponent.h"

#include "NartyHealthComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"
#include "UObject/ConstructorHelpers.h"

UNartyCombatComponent::UNartyCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

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
		World->GetTimerManager().ClearTimer(AbilityEndHandle);
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
		AbilityCooldown = 5.f;
		break;
	case ENartyHero::Batraz:
		AttackRange = 170.f;
		AttackRadius = 55.f;
		AttackDamage = bHasForgeWeapon ? 65.f : 45.f;
		AttackCooldown = 0.7f;
		AbilityCooldown = 8.f;
		break;
	case ENartyHero::Syrdon:
		AttackRange = 130.f;
		AttackRadius = 35.f;
		AttackDamage = bHasForgeWeapon ? 32.f : 20.f;
		AttackCooldown = 0.4f;
		AbilityCooldown = 9.f;
		break;
	default:
		break;
	}

	EnsureAttackInput();
	NotifyAbilityHud();
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

	if (!AbilityAction)
	{
		AbilityAction = NewObject<UInputAction>(this, TEXT("IA_NartyAbility"));
		AbilityAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!AttackMappingContext)
	{
		AttackMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_NartyAttack"));
		AttackMappingContext->MapKey(AttackAction, EKeys::LeftMouseButton);
		AttackMappingContext->MapKey(AttackAction, EKeys::LeftControl);
		AttackMappingContext->MapKey(AbilityAction, EKeys::Q);
		AttackMappingContext->MapKey(AbilityAction, EKeys::Gamepad_FaceButton_Left);
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

	if (bInputBound && bAbilityBound)
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
		if (!bInputBound && AttackAction)
		{
			EIC->BindAction(AttackAction, ETriggerEvent::Started, this, &UNartyCombatComponent::HandleAttackStarted);
			bInputBound = true;
		}
		if (!bAbilityBound && AbilityAction)
		{
			EIC->BindAction(AbilityAction, ETriggerEvent::Started, this, &UNartyCombatComponent::HandleAbilityStarted);
			bAbilityBound = true;
		}
	}
	else if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UNartyCombatComponent::EnsureAttackInput));
	}
}

void UNartyCombatComponent::CancelPendingStrike()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StrikeDelayHandle);
		World->GetTimerManager().ClearTimer(AbilityEndHandle);
	}
	EndSoslanDash();
	EndBatrazFortitude();
	EndSyrdonStealth();
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

	if (AActor* OwnerActor = GetOwner())
	{
		if (const UNartyHealthComponent* Health = OwnerActor->FindComponentByClass<UNartyHealthComponent>())
		{
			if (Health->IsDead())
			{
				return;
			}
		}
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
	const float Range = bHasForgeWeapon ? AttackRange + 30.f : AttackRange;
	DealStrikeDamage(AttackDamage, Range, AttackRadius);
}

void UNartyCombatComponent::DealStrikeDamage(float Damage, float Range, float Radius)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World || IsOwnerDead())
	{
		return;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector End = Start + OwnerActor->GetActorForwardVector() * Range;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(NartyStrike), false, OwnerActor);
	TArray<FHitResult> Hits;

	World->SweepMultiByChannel(
		Hits, Start, End, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(Radius), Params);

	TArray<FHitResult> WorldHits;
	World->SweepMultiByChannel(
		WorldHits, Start, End, FQuat::Identity, ECC_WorldDynamic,
		FCollisionShape::MakeSphere(Radius), Params);
	Hits.Append(WorldHits);

	const FColor DebugColor = bHasForgeWeapon ? FColor::Red : FColor::Orange;
#if ENABLE_DRAW_DEBUG
	DrawDebugSphere(World, End, Radius, 12, DebugColor, false, 0.2f);
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

		if (UNartyHealthComponent* HitHealth = HitActor->FindComponentByClass<UNartyHealthComponent>())
		{
			HitHealth->ApplyDamage(Damage, OwnerActor);
		}
	}
}

bool UNartyCombatComponent::IsOwnerDead() const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return true;
	}

	if (const UNartyHealthComponent* Health = OwnerActor->FindComponentByClass<UNartyHealthComponent>())
	{
		return Health->IsDead();
	}

	return false;
}

void UNartyCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDashing)
	{
		SweepDashHits();
	}

	NotifyAbilityHud();
}

void UNartyCombatComponent::HandleAbilityStarted()
{
	TryAbility();
}

void UNartyCombatComponent::TryAbility()
{
	UWorld* World = GetWorld();
	if (!World || IsOwnerDead())
	{
		return;
	}

	const float Now = World->GetTimeSeconds();
	if (Now - LastAbilityTime < AbilityCooldown)
	{
		return;
	}

	switch (Hero)
	{
	case ENartyHero::Soslan:
		ActivateSoslanDash();
		break;
	case ENartyHero::Batraz:
		ActivateBatrazFortitude();
		break;
	case ENartyHero::Syrdon:
		ActivateSyrdonStealth();
		break;
	default:
		return;
	}

	LastAbilityTime = Now;
	NotifyAbilityHud();
}

FText UNartyCombatComponent::GetAbilityLine() const
{
	FText Name;
	switch (Hero)
	{
	case ENartyHero::Soslan:
		Name = NSLOCTEXT("Narty", "Ability_Soslan", "Q \u2014 \u0440\u044b\u0432\u043e\u043a-\u0436\u0430\u0440");
		break;
	case ENartyHero::Batraz:
		Name = NSLOCTEXT("Narty", "Ability_Batraz", "Q \u2014 \u0436\u0435\u043b\u0435\u0437\u043e");
		break;
	case ENartyHero::Syrdon:
		Name = NSLOCTEXT("Narty", "Ability_Syrdon", "Q \u2014 \u0442\u0435\u043d\u044c");
		break;
	default:
		return FText::GetEmpty();
	}

	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	const float Remaining = AbilityCooldown - (Now - LastAbilityTime);
	if (Remaining > 0.05f && LastAbilityTime > 0.f)
	{
		return FText::Format(
			NSLOCTEXT("Narty", "Ability_CD", "{0}  ({1}\u0441)"),
			Name,
			FText::AsNumber(FMath::CeilToInt(Remaining)));
	}

	if (bStealthed)
	{
		return NSLOCTEXT("Narty", "Ability_Hidden", "Q \u2014 \u0441\u043a\u0440\u044b\u0442");
	}
	if (bFortitude)
	{
		return NSLOCTEXT("Narty", "Ability_Iron", "Q \u2014 \u0441\u0442\u043e\u0439\u043a\u043e\u0441\u0442\u044c");
	}

	return Name;
}

void UNartyCombatComponent::NotifyAbilityHud()
{
	const FText Line = GetAbilityLine();
	if (Line.EqualTo(LastAbilityLine))
	{
		return;
	}
	LastAbilityLine = Line;
	OnAbilityChanged.Broadcast(Line);
}

void UNartyCombatComponent::ActivateSoslanDash()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	bDashing = true;
	DashHitActors.Reset();
	if (UNartyHealthComponent* Health = Character->FindComponentByClass<UNartyHealthComponent>())
	{
		Health->GrantIFrames(0.4f);
	}

	const FVector Launch = Character->GetActorForwardVector() * 2200.f + FVector(0.f, 0.f, 160.f);
	Character->LaunchCharacter(Launch, true, true);
	PlayAttackAnimation();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AbilityEndHandle,
			this,
			&UNartyCombatComponent::EndSoslanDash,
			0.35f,
			false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: Soslan dash"));
}

void UNartyCombatComponent::SweepDashHits()
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return;
	}

	const FVector Start = OwnerActor->GetActorLocation() + FVector(0.f, 0.f, 40.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NartyDash), false, OwnerActor);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps, Start, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(70.f), Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || DashHitActors.Contains(HitActor))
		{
			continue;
		}
		DashHitActors.Add(HitActor);
		if (UNartyHealthComponent* HitHealth = HitActor->FindComponentByClass<UNartyHealthComponent>())
		{
			HitHealth->ApplyDamage(AttackDamage * 1.15f, OwnerActor);
		}
	}
}

void UNartyCombatComponent::EndSoslanDash()
{
	bDashing = false;
	DashHitActors.Reset();
	NotifyAbilityHud();
}

void UNartyCombatComponent::ActivateBatrazFortitude()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	bFortitude = true;
	if (UNartyHealthComponent* Health = OwnerActor->FindComponentByClass<UNartyHealthComponent>())
	{
		Health->IncomingDamageScale = 0.4f;
	}

	PlayAttackAnimation();
	DealStrikeDamage(AttackDamage * 2.1f, (bHasForgeWeapon ? AttackRange + 50.f : AttackRange + 20.f), AttackRadius * 1.7f);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AbilityEndHandle,
			this,
			&UNartyCombatComponent::EndBatrazFortitude,
			3.2f,
			false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: Batraz fortitude"));
}

void UNartyCombatComponent::EndBatrazFortitude()
{
	bFortitude = false;
	if (AActor* OwnerActor = GetOwner())
	{
		if (UNartyHealthComponent* Health = OwnerActor->FindComponentByClass<UNartyHealthComponent>())
		{
			Health->IncomingDamageScale = 1.f;
		}
	}
	NotifyAbilityHud();
}

void UNartyCombatComponent::ActivateSyrdonStealth()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	bStealthed = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AbilityEndHandle,
			this,
			&UNartyCombatComponent::EndSyrdonStealth,
			4.5f,
			false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Narty: Syrdon stealth"));
}

void UNartyCombatComponent::EndSyrdonStealth()
{
	bStealthed = false;
	NotifyAbilityHud();
}
