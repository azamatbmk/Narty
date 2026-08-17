#pragma once

#include "CoreMinimal.h"
#include "NartyHeroTypes.generated.h"

UENUM(BlueprintType)
enum class ENartyHero : uint8
{
	None UMETA(DisplayName = "None"),
	Soslan UMETA(DisplayName = "Soslan"),
	Batraz UMETA(DisplayName = "Batraz"),
	Syrdon UMETA(DisplayName = "Syrdon")
};

USTRUCT(BlueprintType)
struct FNartyHeroStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float JumpZVelocity = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;
};
