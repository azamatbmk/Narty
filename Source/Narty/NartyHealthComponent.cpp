#include "NartyHealthComponent.h"

#include "Engine/World.h"

UNartyHealthComponent::UNartyHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNartyHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Health <= 0.f || Health > MaxHealth)
	{
		Health = MaxHealth;
	}
	bDead = false;
}

bool UNartyHealthComponent::IsInvulnerable() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < InvulnerableUntil;
}

void UNartyHealthComponent::SetMaxHealth(float InMaxHealth, bool bFill)
{
	MaxHealth = FMath::Max(1.f, InMaxHealth);
	if (bFill)
	{
		Health = MaxHealth;
		bDead = false;
	}
	else
	{
		Health = FMath::Clamp(Health, 0.f, MaxHealth);
	}
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UNartyHealthComponent::ResetToFull(float InvulnerableSeconds)
{
	bDead = false;
	Health = MaxHealth;
	if (const UWorld* World = GetWorld())
	{
		InvulnerableUntil = World->GetTimeSeconds() + FMath::Max(0.f, InvulnerableSeconds);
	}
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

float UNartyHealthComponent::ApplyDamage(float Damage, AActor* InstigatorActor)
{
	if (bDead || Damage <= 0.f || IsInvulnerable())
	{
		return 0.f;
	}

	Health = FMath::Max(0.f, Health - Damage);
	if (const UWorld* World = GetWorld())
	{
		InvulnerableUntil = World->GetTimeSeconds() + HitInvulnSeconds;
	}

	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (Health <= 0.f)
	{
		bDead = true;
		OnDied.Broadcast(GetOwner(), InstigatorActor);
	}

	return Damage;
}
