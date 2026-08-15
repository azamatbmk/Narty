#pragma once

#include "CoreMinimal.h"
#include "NartyQuestTypes.generated.h"

UENUM(BlueprintType)
enum class ENartyQuestStage : uint8
{
	None UMETA(DisplayName = "None"),
	GetWeapon UMETA(DisplayName = "GetWeapon"),
	FetchFire UMETA(DisplayName = "FetchFire"),
	ReturnFire UMETA(DisplayName = "ReturnFire"),
	Uatsamonga UMETA(DisplayName = "Uatsamonga"),
	HeroTrial UMETA(DisplayName = "HeroTrial"),
	Finale UMETA(DisplayName = "Finale"),
	Completed UMETA(DisplayName = "Completed")
};

UENUM(BlueprintType)
enum class ENartyEnding : uint8
{
	None UMETA(DisplayName = "None"),
	SoslanSacrifice UMETA(DisplayName = "SoslanSacrifice"),
	SoslanShame UMETA(DisplayName = "SoslanShame"),
	BatrazMercy UMETA(DisplayName = "BatrazMercy"),
	BatrazStorm UMETA(DisplayName = "BatrazStorm"),
	SyrdonPeace UMETA(DisplayName = "SyrdonPeace"),
	SyrdonLie UMETA(DisplayName = "SyrdonLie")
};
