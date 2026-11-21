// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

class FDialogueGraphConnectionDrawingPolicy : public FConnectionDrawingPolicy
{

public:
	FDialogueGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
		: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements)
	{
		TransitionColor = FLinearColor(0.92f, 0.72f, 0.2f, 1.f);
		TransitionTime = 1000.f;
		BacklinkImage = ArrowImage;
		ArrowImage = nullptr; // dont draw arrows
		
	}

	using FConnectionDrawingPolicy::ComputeSplineTangent;
	virtual FVector2f ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const override;
	void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params);
	using FConnectionDrawingPolicy::DrawConnection;
	virtual void DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params) override;

	const FSlateBrush* BacklinkImage;

	FLinearColor TransitionColor;
	float TransitionTime; //in ms
};