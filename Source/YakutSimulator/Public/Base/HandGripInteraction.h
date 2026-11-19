#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HandGripInteraction.generated.h"

class AVirtualRealityPawn;

enum class EHandGripResult : uint8
{
    Ignored,
    Started,
    Blocked
};

UINTERFACE(MinimalAPI)
class UHandGripInteraction : public UInterface
{
    GENERATED_BODY()
};

class YAKUTSIMULATOR_API IHandGripInteraction
{
    GENERATED_BODY()

public:
    virtual EHandGripResult TryBeginHandGrip(AVirtualRealityPawn* Pawn, bool bLeftHand) = 0;
    virtual void EndHandGrip(AVirtualRealityPawn* Pawn, bool bLeftHand, bool bCancelled) = 0;
};
