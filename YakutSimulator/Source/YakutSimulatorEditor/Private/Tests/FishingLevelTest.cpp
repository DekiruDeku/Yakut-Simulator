#if WITH_DEV_AUTOMATION_TESTS
#include "MiniGameTestFixture.h"
#include "Tests/AutomationEditorCommon.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "InputAction.h"
#include "Editor.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

namespace MiniGameTests
{
class FPlaythrough : public IAutomationLatentCommand
{
    FAutomationTestBase* Test;
    double Started=FPlatformTime::Seconds();
    AFishingMiniGameScenario* Game=nullptr;
    APawn* Pawn=nullptr;
    UXRInteractorComponent* Input=nullptr;
    USceneComponent* Hands[2]={nullptr,nullptr};
    UCameraComponent* Camera=nullptr;
    AFishingFish* Fish=nullptr;
    int32 Phase=0;
    bool bHaul=false;
    float WaitUntil=0;
    bool bFeedbackCaptured=false;
    bool Check(bool Value,const TCHAR* Message){return Test->TestTrue(Message,Value);}
    bool Press(FName Id)
    {
        for (TActorIterator<AMiniGameStation> It(Game->GetWorld());It;++It)
            if (It->Button->bEnabled && It->Button->ActionId==Id)
            {
                Hands[0]->SetWorldLocation(It->PressZone->GetComponentLocation());
                if(!Check(Input->FindTarget(Input->Context(false),EInteractionAction::Use)==It->Button,TEXT("Authored button is the selected target")))return false;
                Input->Use(false);return true;
            }
        return Check(false,TEXT("Required button exists"));
    }
    bool Grab(AGrabbableItem* Item,int32 I)
    {
        Input->ReleaseGrip(I!=0);Hands[I]->SetWorldLocation(Item->GetActorLocation());Input->Grip(I!=0);
        return Check(Item->Grab->GetHoldingHand()==Hands[I],TEXT("Production interactor grabs requested item"));
    }
    void Drop(AGrabbableItem* Item,FVector Location,int32 I)
    {Hands[I]->AddWorldOffset(Location-Item->GetActorLocation());Input->ReleaseGrip(I!=0);}
    bool Lever()
    {
        auto* Lever=Game->WinchStation->Lever.Get();Input->ReleaseGrip(false);Hands[0]->SetWorldLocation(Game->WinchStation->Zone->GetComponentLocation());Input->Grip(false);
        if(!Check(Lever->GetHoldingHand()==Hands[0],TEXT("Production interactor grabs lever")))return false;
        Hands[0]->AddWorldOffset(Lever->GetComponentTransform().TransformVectorNoScale(Lever->LocalAxis.GetSafeNormal()*Lever->Travel*.5f));
        return true;
    }
    bool Workplace(USceneComponent* Anchor)
    {
        const FVector Offset=Camera->GetComponentLocation()-Anchor->GetComponentLocation();
        return Check(FVector2D(Offset.X,Offset.Y).Size()<.1f,TEXT("Travel compensates horizontal HMD offset")) &&
            Check(FMath::Abs(Pawn->GetActorLocation().Z-Anchor->GetComponentLocation().Z)<.1f,TEXT("Travel preserves floor anchor"));
    }
public:
    explicit FPlaythrough(FAutomationTestBase* In):Test(In){}
    virtual bool Update() override
    {
        UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
        if(FPlatformTime::Seconds()-Started>240){Test->AddError(FString::Printf(TEXT("Saved level timed out: phase=%d objective=%s"),Phase,Game?*Game->ObjectiveId.ToString():TEXT("none")));return true;}
        if(!World)return false;
        if(Phase==0)
        {
            if(World->GetTimeSeconds()<.5f)return false;
            for(TActorIterator<AFishingMiniGameScenario> It(World);It;++It)Game=*It;
            Pawn=UGameplayStatics::GetPlayerPawn(World,0);
            if(!Check(Game&&Pawn,TEXT("Saved map creates scenario and pawn")))return true;
            Input=Pawn->FindComponentByClass<UXRInteractorComponent>();Camera=Pawn->FindComponentByClass<UCameraComponent>();
            if(!Check(Input&&Camera,TEXT("Common pawn owns input and camera")))return true;
            Hands[0]=Input->Context(false).Hand;Hands[1]=Input->Context(true).Hand;
            if(!Check(Hands[0]&&Hands[1],TEXT("Explicit hand bindings resolve")))return true;
            // shortcut: synthetic poses bypass hardware tracking; physical headset calibration is a separate check.
            Input->bRequireTracking=false;
            for(auto* Hand:Hands)Hand->SetComponentTickEnabled(false);
            Camera->SetRelativeLocation(FVector(25,-15,165));
            auto* Enhanced=Cast<UEnhancedInputComponent>(Pawn->InputComponent);int32 Bindings=0;bool Duplicated=false;
            if(Enhanced)for(const auto& Binding:Enhanced->GetActionEventBindings())
            {
                const auto* Action=Binding->GetAction();
                if(!Action)continue;
                const auto* Profile=Input->Profile.Get();
                if(Action==Profile->LeftGrab||Action==Profile->LeftRelease||Action==Profile->RightGrab||Action==Profile->RightRelease||Action==Profile->LeftUse||Action==Profile->RightUse)
                {if(Binding->IsBoundToObject(Input))++Bindings;else Duplicated=true;}
            }
            if(!Check(Bindings==6&&!Duplicated,TEXT("One owner for all six gameplay actions")))return true;
            if(!Check(Game->FishingDefinition()->Catch.FishCount==8,TEXT("Default authored batch has eight fish")))return true;
            if(!Press(Game->FishingDefinition()->EquipAction)||!Press(Game->FishingDefinition()->StopAction)||!Press(Game->FishingDefinition()->StartAction)||!Lever())return true;
            Phase=1;Test->AddInfo(TEXT("Saved level: deployment, trawling and hauling through common lever"));
        }
        else if(Phase==1)
        {
            if(Game->Operation==EFishingOperation::Haul&&!bHaul){if(!Lever())return true;bHaul=true;}
            if(Game->ObjectiveId==TEXT("GoSorting"))
            {
                Input->ReleaseAll();if(!Press(Game->FishingDefinition()->SortingAction))return true;
                Phase=2;WaitUntil=World->GetTimeSeconds()+.4f;
            }
        }
        else if(Phase==2&&World->GetTimeSeconds()>WaitUntil)
        {
            if(!Workplace(Game->SortingPlayerPoint))return true;
            if(Game->ObjectiveId==TEXT("GoProcessing"))
            {
                if(!Press(Game->FishingDefinition()->ProcessingAction))return true;
                Phase=3;WaitUntil=World->GetTimeSeconds()+.4f;Test->AddInfo(TEXT("Saved level: all eight fish sorted by release into receivers"));
            }
            else for(AFishingFish* Item:Game->Conveyor->Fish)if(Item->State==EFishWorkState::Conveyor||Item->State==EFishWorkState::Loose)
            {
                for(const auto& Bin:Game->SortingBins)if(Bin.Species==Item->Species)
                {if(!Grab(Item,0))return true;Drop(Item,Bin.Station->GetActorLocation(),0);if(!Check(Item->bSorted,TEXT("Receiver accepts matching species")))return true;break;}
                break;
            }
        }
        else if(Phase==3&&World->GetTimeSeconds()>WaitUntil)
        {
            if(Game->Status==EMiniGameStatus::Completed)
            {
                if(!Check(Game->Processing->Delivered==8&&Game->Score()==800,TEXT("Eight-fish shift completes with configured score")))return true;
                if(!Press(Game->FishingDefinition()->RestartAction))return true;
                Check(Game->ObjectiveId==TEXT("Equip")&&Game->Conveyor->Fish.IsEmpty(),TEXT("New shift resets the saved level"));return true;
            }
            if(!Workplace(Game->ProcessingPlayerPoint))return true;
            Fish=nullptr;for(AFishingFish* Item:Game->Conveyor->Fish)if(Item->State==EFishWorkState::Loose){Fish=Item;break;}
            if(!Check(Fish!=nullptr,TEXT("Processing supplies the next fish")))return true;
            if(!Grab(Fish,0))return true;Drop(Fish,Game->ProcessingStation->GetActorLocation(),0);
            if(!Check(Game->Processing->BoardFish==Fish,TEXT("Release loads authored work board")))return true;
            if(!Grab(Game->Processing->Knife,1)||!Grab(Fish,0))return true;
            Phase=4;
        }
        else if(Phase==4)
        {
            if(bFeedbackCaptured && World->GetTimeSeconds()<WaitUntil)return false;
            if(!bFeedbackCaptured && Fish->State==EFishWorkState::OnBoard && Fish->NextCutPoint>0)
            {
                const auto Panel=Game->GetPresentation(NAME_None,Game);
                if(!Check(Panel.Metrics.Num()==2 && Panel.Metrics[0].Value>0 && Panel.Metrics[0].Value<Panel.Metrics[0].Maximum,
                    TEXT("Partial cutting progress is visible on the actual level panel")))return true;
                if(!Check(Fish->IsBraced() && Fish->BraceIndicator->IsVisible() && Game->Processing->Knife->ToolTipVisual->IsVisible(),
                    TEXT("Actual level shows the brace and knife-tip markers")))return true;
                Camera->SetRelativeRotation(FRotator(-30,0,0));
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("ProcessingFeedback/processing-in-level.png"),false,false);
                Test->AddInfo(TEXT("Captured partial-cut feedback from the real game viewport"));
                bFeedbackCaptured=true;WaitUntil=World->GetTimeSeconds()+.5f;return false;
            }
            if(Fish->State==EFishWorkState::Processed)
            {
                Input->ReleaseAll();if(!Grab(Fish,0))return true;Drop(Fish,Game->DeliveryStation->GetActorLocation(),0);
                if(!Check(Fish->State==EFishWorkState::Delivered,TEXT("Processed fish delivered through receiver")))return true;
                Phase=3;WaitUntil=World->GetTimeSeconds()+.05f;
            }
            else
            {
                const auto Points=Fish->GetCutPoints();
                if(!Check(Points.IsValidIndex(Fish->NextCutPoint),TEXT("Authored cut checkpoint exists")))return true;
                const FVector Target=Fish->GetActorTransform().TransformPosition(Points[Fish->NextCutPoint]);
                Hands[1]->AddWorldOffset(Target-Game->Processing->Knife->ToolTip->GetComponentLocation());
            }
        }
        return false;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingSavedLevel,"Yakut.MiniGames.Fishing.SavedLevelPlaythrough",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFishingSavedLevel::RunTest(const FString&)
{
    if(!GEditor||GEditor->PlayWorld){AddError(TEXT("Stop Play before running the level test."));return false;}
    UWorld* Original=GEditor->GetEditorWorldContext().World();
    if(!Original||Original->GetPackage()->IsDirty()){AddError(TEXT("Save the current level before testing."));return false;}
    const FString OriginalMap=Original->GetPackage()->GetName();const FString Map=TEXT("/Game/MiniGames/Fishing/Maps/L_Fishing");
    FAutomationEditorCommonUtils::LoadMap(Map);
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(MiniGameTests::FPlaythrough(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    if(OriginalMap!=Map)ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(OriginalMap));
    return true;
}
#endif
