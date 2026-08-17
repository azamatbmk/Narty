#include "NartyGorgeLayout.h"

#include "NartyForgeActor.h"
#include "NartyTrainingDummy.h"
#include "NartyMountainFireActor.h"
#include "NartySettlementHearthActor.h"
#include "NartyUatsamongaCup.h"
#include "NartyHeroTrialSite.h"
#include "NartyFinaleSite.h"
#include "NartyUaigEnemy.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"

namespace NartyGorgeLayout
{
	template <typename T>
	T* FindOrSpawnAt(UWorld* World, const FVector& Location, const FRotator& Rotation)
	{
		if (T* Existing = FindNearest<T>(World, Location, StorySnapRadius))
		{
			Existing->Tags.AddUnique(StoryTag);
			return Existing;
		}

		if (!World)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		T* Spawned = World->SpawnActor<T>(T::StaticClass(), Location, Rotation, Params);
		if (Spawned)
		{
			Spawned->Tags.AddUnique(StoryTag);
			UE_LOG(LogTemp, Warning, TEXT("Narty: spawned missing %s (place it in Lvl_Gorge to edit)"),
				*T::StaticClass()->GetName());
		}
		return Spawned;
	}

	void SpawnMissingStoryActors(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		FindOrSpawnAt<ANartyForgeActor>(World, Origin + FVector(1100.f, 0.f, 40.f), FRotator(0.f, 180.f, 0.f));
		FindOrSpawnAt<ANartyTrainingDummy>(World, Origin + FVector(200.f, -120.f, 90.f), FRotator::ZeroRotator);
		FindOrSpawnAt<ANartyTrainingDummy>(World, Origin + FVector(450.f, 140.f, 90.f), FRotator::ZeroRotator);
		FindOrSpawnAt<ANartySettlementHearthActor>(World, Origin + FVector(-750.f, 180.f, 40.f), FRotator(0.f, -90.f, 0.f));
		FindOrSpawnAt<ANartyMountainFireActor>(World, Origin + FVector(1580.f, 0.f, 230.f), FRotator(0.f, 180.f, 0.f));
		FindOrSpawnAt<ANartyUatsamongaCup>(World, Origin + FVector(-200.f, 0.f, 50.f), FRotator(0.f, 180.f, 0.f));
		FindOrSpawnAt<ANartyHeroTrialSite>(World, Origin + FVector(-200.f, 1100.f, 0.f), FRotator(0.f, -90.f, 0.f));
		FindOrSpawnAt<ANartyFinaleSite>(World, Origin + FVector(-1100.f, -250.f, 40.f), FRotator(0.f, 90.f, 0.f));
		FindOrSpawnAt<ANartyUaigEnemy>(World, Origin + FVector(1250.f, 80.f, 140.f), FRotator(0.f, 180.f, 0.f));
	}

	FTransform FindNykhasStart(UWorld* World)
	{
		const FTransform Fallback = PlayerStart();
		if (!World)
		{
			return Fallback;
		}

		APlayerStart* Named = nullptr;
		APlayerStart* Only = nullptr;
		int32 Count = 0;

		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			APlayerStart* Start = *It;
			if (!Start)
			{
				continue;
			}

			++Count;
			Only = Start;

			const bool bTagged = Start->ActorHasTag(PlayerStartTag);
			const bool bNamed = Start->GetActorNameOrLabel().Contains(TEXT("Narty"));
			if (bTagged || bNamed)
			{
				Named = Start;
			}
		}

		if (Named)
		{
			return Named->GetActorTransform();
		}

		if (Count == 1 && Only)
		{
			return Only->GetActorTransform();
		}

		if (Count > 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("Narty: %d PlayerStarts and none tagged Narty — using canonical nykhas"), Count);
		}

		return Fallback;
	}

	void EnsureReadableWorldLighting(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		ADirectionalLight* Sun = FindFirst<ADirectionalLight>(World);
		if (!Sun)
		{
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Sun = World->SpawnActor<ADirectionalLight>(
				ADirectionalLight::StaticClass(),
				FVector(0.f, 0.f, 800.f),
				FRotator(-42.f, 35.f, 0.f),
				Params);
			if (Sun)
			{
				Sun->SetActorLabel(TEXT("Sun_Narty"));
			}
		}

		if (Sun)
		{
			if (UDirectionalLightComponent* Light = Sun->GetComponent())
			{
				Light->SetIntensity(12.f);
				Light->SetLightColor(FLinearColor(1.f, 0.94f, 0.82f));
				Light->SetTemperature(5500.f);
				Light->SetUseTemperature(true);
				Light->SetCastShadows(true);
			}
			Sun->SetActorRotation(FRotator(-42.f, 35.f, 0.f));
		}

		if (ASkyLight* Sky = FindFirst<ASkyLight>(World))
		{
			if (USkyLightComponent* SkyComp = Sky->GetLightComponent())
			{
				SkyComp->SetIntensity(1.25f);
				SkyComp->SetMobility(EComponentMobility::Movable);
				SkyComp->SetLightColor(FLinearColor(0.55f, 0.62f, 0.75f));
				SkyComp->RecaptureSky();
			}
		}

		if (AExponentialHeightFog* Fog = FindFirst<AExponentialHeightFog>(World))
		{
			if (UExponentialHeightFogComponent* FogComp = Fog->GetComponent())
			{
				FogComp->SetFogDensity(0.012f);
				FogComp->SetFogHeightFalloff(0.12f);
				FogComp->SetFogInscatteringColor(FLinearColor(0.45f, 0.5f, 0.58f));
				FogComp->SetStartDistance(600.f);
			}
		}
	}

	ANartyForgeActor* GetForge(UWorld* World)
	{
		return FindPreferred<ANartyForgeActor>(World, Origin + FVector(1100.f, 0.f, 40.f));
	}

	ANartyMountainFireActor* GetMountainFire(UWorld* World)
	{
		return FindPreferred<ANartyMountainFireActor>(World, Origin + FVector(1580.f, 0.f, 230.f));
	}

	ANartySettlementHearthActor* GetHearth(UWorld* World)
	{
		return FindPreferred<ANartySettlementHearthActor>(World, Origin + FVector(-750.f, 180.f, 40.f));
	}

	ANartyUatsamongaCup* GetUatsamongaCup(UWorld* World)
	{
		return FindPreferred<ANartyUatsamongaCup>(World, Origin + FVector(-200.f, 0.f, 50.f));
	}

	ANartyHeroTrialSite* GetHeroTrialSite(UWorld* World)
	{
		return FindPreferred<ANartyHeroTrialSite>(World, Origin + FVector(-200.f, 1100.f, 0.f));
	}

	ANartyFinaleSite* GetFinaleSite(UWorld* World)
	{
		return FindPreferred<ANartyFinaleSite>(World, Origin + FVector(-1100.f, -250.f, 40.f));
	}
}
