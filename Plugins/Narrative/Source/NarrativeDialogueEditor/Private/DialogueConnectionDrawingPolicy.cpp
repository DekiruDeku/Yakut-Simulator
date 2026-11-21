// Copyright Narrative Tools 2022. 

#include "DialogueConnectionDrawingPolicy.h"
#include "QuestSM.h"
#include "DialogueGraphNode_NPC.h"
#include <EdGraph/EdGraphNode.h>

FVector2f FDialogueGraphConnectionDrawingPolicy::ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const
{
	return FConnectionDrawingPolicy::ComputeSplineTangent(Start, End);
}

void FDialogueGraphConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{
	Params.WireColor = FLinearColor::White;
	Params.WireThickness = 2.f;

	UEdGraphNode* OutNode = OutputPin ? OutputPin->GetOwningNode() : nullptr;
	UEdGraphNode* InNode = InputPin ? InputPin->GetOwningNode() : nullptr;

	if (OutNode && InNode)
	{
		if (OutNode->NodePosX > InNode->NodePosX)
		{
			Params.WireColor.A = 0.3f;
		}
	}
}

void FDialogueGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params)
{
	if (Start.X > End.X)
	{
		FVector2f Halfway = (Start + End) * 0.5f;
		Halfway.Y -= 300.f * ZoomFactor;

		DrawConnection(LayerId, Halfway, Start, Params);
		DrawConnection(LayerId, End, Halfway, Params);

		if (BacklinkImage != nullptr)
		{
			FVector2f ArrowPoint = Halfway - ArrowRadius;

			FSlateDrawElement::MakeRotatedBox(
				DrawElementsList,
				ArrowLayerID,
				FPaintGeometry(FVector2f(ArrowPoint), FVector2f(BacklinkImage->ImageSize) * ZoomFactor, ZoomFactor),
				BacklinkImage,
				ESlateDrawEffect::None,
				PI,
				TOptional<FVector2f>(),
				FSlateDrawElement::RelativeToElement,
				Params.WireColor
			);
		}
	}
	else
	{
		FConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
	}
}
