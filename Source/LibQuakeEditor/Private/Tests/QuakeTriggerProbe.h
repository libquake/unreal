#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "QuakeTriggerProbe.generated.h"

/** Counts OnTriggered calls in the tests. */
UCLASS()
class UQuakeTriggerProbe : public UObject
{
	GENERATED_BODY()

public:
	int32 Count = 0;

	UFUNCTION()
	void OnTriggered(AActor* Activator) { Count++; }
};
