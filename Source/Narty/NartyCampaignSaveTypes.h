#pragma once

#include "CoreMinimal.h"
#include "NartyCampaignSaveTypes.generated.h"

USTRUCT()
struct FNartyTrialSaveState
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHasData = false;

	UPROPERTY()
	bool bEntered = false;

	UPROPERTY()
	bool bAwaitingFinalChoice = false;

	UPROPERTY()
	int32 VisionsSeen = 0;

	UPROPERTY()
	int32 SyrdonDeals = 0;

	UPROPERTY()
	TArray<float> DummyHealth;
};
