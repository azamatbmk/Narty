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
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

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

	void SpawnSafetyBlock(UWorld* World, const FName& Label, const FVector& Location, const FVector& Scale)
	{
		if (!World)
		{
			return;
		}

		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (*It && It->ActorHasTag(TEXT("NartySafety"))
				&& FVector::DistSquared(It->GetActorLocation(), Location) < 2500.f)
			{
				return;
			}
		}

		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Cube)
		{
			return;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
		if (!Block)
		{
			return;
		}

		Block->Tags.AddUnique(TEXT("NartySafety"));
		Block->SetActorScale3D(Scale);
		UE_LOG(LogTemp, Log, TEXT("Narty: safety floor %s"), *Label.ToString());
		if (UStaticMeshComponent* Mesh = Block->GetStaticMeshComponent())
		{
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(Cube);
			Mesh->SetCollisionProfileName(TEXT("BlockAll"));
			if (UMaterialInstanceDynamic* Dyn = Mesh->CreateAndSetMaterialInstanceDynamic(0))
			{
				const FLinearColor Tint(0.16f, 0.15f, 0.13f);
				Dyn->SetVectorParameterValue(TEXT("Color"), Tint);
				Dyn->SetVectorParameterValue(TEXT("BaseColor"), Tint);
			}
		}
	}

	bool HasMeshLabel(UWorld* World, const TCHAR* Token)
	{
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (*It && It->GetActorNameOrLabel().Contains(Token))
			{
				return true;
			}
		}
		return false;
	}

	void EnsureUaigs(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		TArray<ANartyUaigEnemy*> All;
		for (TActorIterator<ANartyUaigEnemy> It(World); It; ++It)
		{
			if (*It)
			{
				All.Add(*It);
			}
		}

		const FVector Homes[] = { UaigHomeA(), UaigHomeB() };
		constexpr int32 Wanted = 2;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (int32 Index = 0; Index < Wanted; ++Index)
		{
			if (Index < All.Num())
			{
				All[Index]->Tags.AddUnique(StoryTag);
				All[Index]->ReviveAt(Homes[Index]);
			}
			else if (ANartyUaigEnemy* Spawned = World->SpawnActor<ANartyUaigEnemy>(
				ANartyUaigEnemy::StaticClass(), Homes[Index], FRotator(0.f, 180.f, 0.f), Params))
			{
				Spawned->Tags.AddUnique(StoryTag);
				UE_LOG(LogTemp, Warning, TEXT("Narty: spawned missing uaig %d"), Index);
			}
		}

		for (int32 Index = Wanted; Index < All.Num(); ++Index)
		{
			All[Index]->Destroy();
		}
	}

	void EnsureSafetyGeometry(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		if (AWorldSettings* Settings = World->GetWorldSettings())
		{
			Settings->KillZ = -20000.f;
		}

		TArray<AActor*> Stale;
		for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
		{
			if (!*It)
			{
				continue;
			}

			if (It->ActorHasTag(TEXT("NartySafety")))
			{
				Stale.Add(*It);
			}
		}
		for (AActor* Actor : Stale)
		{
			Actor->Destroy();
		}

		// Side steps onto the fire ledge if the map was never re-populated.
		if (!HasMeshLabel(World, TEXT("Grey_FireStepR1")))
		{
			SpawnSafetyBlock(World, TEXT("Safety_FireStepR1"), Origin + FVector(1380.f, 280.f, 20.f), FVector(5.f, 4.f, 0.6f));
			SpawnSafetyBlock(World, TEXT("Safety_FireStepR2"), Origin + FVector(1520.f, 280.f, 80.f), FVector(5.f, 4.f, 0.6f));
			SpawnSafetyBlock(World, TEXT("Safety_FireStepR3"), Origin + FVector(1640.f, 220.f, 130.f), FVector(5.f, 4.f, 0.6f));
			SpawnSafetyBlock(World, TEXT("Safety_FireStepL1"), Origin + FVector(1380.f, -280.f, 20.f), FVector(5.f, 4.f, 0.6f));
			SpawnSafetyBlock(World, TEXT("Safety_FireStepL2"), Origin + FVector(1520.f, -280.f, 80.f), FVector(5.f, 4.f, 0.6f));
			SpawnSafetyBlock(World, TEXT("Safety_FireStepL3"), Origin + FVector(1640.f, -220.f, 130.f), FVector(5.f, 4.f, 0.6f));
		}

		// Keep hearth / trial paths walkable without deleting canyon walls.
		if (!HasMeshLabel(World, TEXT("Grey_HearthApron")))
		{
			SpawnSafetyBlock(World, TEXT("Safety_HearthApron"), Origin + FVector(-600.f, 500.f, -50.f), FVector(20.f, 20.f, 1.f));
		}
		if (!HasMeshLabel(World, TEXT("Grey_TrialPathFloor")))
		{
			SpawnSafetyBlock(World, TEXT("Safety_TrialPath"), Origin + FVector(-200.f, 1000.f, -50.f), FVector(16.f, 22.f, 1.f));
		}
		if (!HasMeshLabel(World, TEXT("Grey_NykhasJoin")))
		{
			SpawnSafetyBlock(World, TEXT("Safety_NykhasJoin"), Origin + FVector(-350.f, 250.f, -50.f), FVector(16.f, 12.f, 1.f));
		}
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
		EnsureUaigs(World);
	}

	void ResetWorldForNewRun(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		if (ANartyForgeActor* Forge = GetForge(World))
		{
			Forge->ResetForNewRun();
		}
		if (ANartyMountainFireActor* Fire = GetMountainFire(World))
		{
			Fire->ResetForNewRun();
		}
		if (ANartySettlementHearthActor* Hearth = GetHearth(World))
		{
			Hearth->ResetForNewRun();
		}
		if (ANartyUatsamongaCup* Cup = GetUatsamongaCup(World))
		{
			Cup->ResetForNewRun();
		}
		if (ANartyHeroTrialSite* Trial = GetHeroTrialSite(World))
		{
			Trial->ResetForNewRun();
		}
		if (ANartyFinaleSite* Finale = GetFinaleSite(World))
		{
			Finale->ResetForNewRun();
		}

		for (TActorIterator<ANartyTrainingDummy> It(World); It; ++It)
		{
			if (*It)
			{
				(*It)->Revive();
			}
		}

		EnsureUaigs(World);
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
