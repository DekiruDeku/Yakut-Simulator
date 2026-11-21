// Fill out your copyright notice in the Description page of Project Settings.


#include "NarrativeDialogueSettings.h"

UNarrativeDialogueSettings::UNarrativeDialogueSettings()
{
	bAllowAutoSkip = true;
	LettersPerSecondLineDuration = 25.f;
	MinDialogueTextDisplayTime = 2.f;
	DialogueLineAudioSilence = 0.5f;
}
