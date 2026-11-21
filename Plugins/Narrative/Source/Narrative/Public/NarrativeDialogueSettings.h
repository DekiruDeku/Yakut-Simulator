// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NarrativeDialogueSettings.generated.h"

/**
 * Runtime dialogue settings for narrative 
 */
UCLASS(config = Engine, defaultconfig)
class NARRATIVE_API UNarrativeDialogueSettings : public UObject
{
	GENERATED_BODY()
	
public:

	UNarrativeDialogueSettings();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, config, Category = "Dialogue Settings")
	bool bAllowAutoSkip = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, config, Category = "Dialogue Settings", meta = (ClampMin = 0.01))
	float DialogueLineAudioSilence;

	//How long should the text be displayed for at a minimum? Since default letters per minute is 25 this prevents a reply like "no" from being played too quickly
	UPROPERTY(BlueprintReadWrite, EditAnywhere, config, Category = "Dialogue Settings", meta = (ClampMin=0.01))
	float MinDialogueTextDisplayTime;

	//If a dialogue doesn't have audio supplied, how long should the text be displayed on the screen for? Lower letters per minute means player gets more time 
	UPROPERTY(BlueprintReadWrite, EditAnywhere, config, Category = "Dialogue Settings", meta = (ClampMin = 1))
	float LettersPerSecondLineDuration;

};
