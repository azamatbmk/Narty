#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"

class ANartyMountainFireActor;
class ANartySettlementHearthActor;
class ANartyUatsamongaCup;
class ANartyHeroTrialSite;
class ANartyFinaleSite;
class ANartyForgeActor;

/** Canonical greybox poses (arena origin 0,0,100). Story actors live in Lvl_Gorge. */
namespace NartyGorgeLayout
{
	inline const FVector Origin(0.f, 0.f, 100.f);
	inline const FName StoryTag(TEXT("NartyStory"));
	inline const FName PlayerStartTag(TEXT("NartyPlayerStart"));
	inline constexpr float StorySnapRadius = 280.f;

	inline FTransform PlayerStart()
	{
		return FTransform(FRotator::ZeroRotator, Origin + FVector(-900.f, 0.f, 120.f));
	}

	template <typename T>
	T* FindFirst(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<T> It(World); It; ++It)
		{
			return *It;
		}

		return nullptr;
	}

	template <typename T>
	T* FindNearest(UWorld* World, const FVector& Location, float MaxDistance)
	{
		if (!World)
		{
			return nullptr;
		}

		T* Best = nullptr;
		float BestDistSq = MaxDistance * MaxDistance;
		for (TActorIterator<T> It(World); It; ++It)
		{
			T* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), Location);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Best = Actor;
			}
		}

		return Best;
	}

	template <typename T>
	T* FindPreferred(UWorld* World, const FVector& PreferredLocation)
	{
		if (!World)
		{
			return nullptr;
		}

		T* TaggedClosest = nullptr;
		T* AnyClosest = nullptr;
		float TaggedDistSq = TNumericLimits<float>::Max();
		float AnyDistSq = TNumericLimits<float>::Max();
		int32 Count = 0;

		for (TActorIterator<T> It(World); It; ++It)
		{
			T* Actor = *It;
			if (!Actor)
			{
				continue;
			}

			++Count;
			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), PreferredLocation);
			if (DistSq < AnyDistSq)
			{
				AnyDistSq = DistSq;
				AnyClosest = Actor;
			}

			if (Actor->ActorHasTag(StoryTag) && DistSq < TaggedDistSq)
			{
				TaggedDistSq = DistSq;
				TaggedClosest = Actor;
			}
		}

		if (Count > 1)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Narty: %d actors of %s — using nearest to canonical pose"),
				Count, *T::StaticClass()->GetName());
		}

		return TaggedClosest ? TaggedClosest : AnyClosest;
	}

	void SpawnMissingStoryActors(UWorld* World);
	void EnsureReadableWorldLighting(UWorld* World);
	FTransform FindNykhasStart(UWorld* World);

	ANartyForgeActor* GetForge(UWorld* World);
	ANartyMountainFireActor* GetMountainFire(UWorld* World);
	ANartySettlementHearthActor* GetHearth(UWorld* World);
	ANartyUatsamongaCup* GetUatsamongaCup(UWorld* World);
	ANartyHeroTrialSite* GetHeroTrialSite(UWorld* World);
	ANartyFinaleSite* GetFinaleSite(UWorld* World);
}
