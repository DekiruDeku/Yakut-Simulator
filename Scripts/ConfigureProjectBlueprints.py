import json
import pathlib
import shutil
import unreal
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools

ProjectDirectory=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
BackupDirectory=ProjectDirectory/'Saved/Backups/BlueprintConfiguration'
VerificationDirectory=ProjectDirectory/'Saved/Verification/BlueprintConfiguration'
BaseClasses=['InteractionZoneComponent','WorkOperationComponent','WorkItem','ScenarioDirector','PlacementStation','VirtualRealityPawn','MainPlayerController','MainGameMode','MainGameInstance','MainHUD','MainSettingsSave','PhysicalLever','WorkstationScenario']
FishingClasses=['VesselMovementComponent','FishSpeciesDefinition','TrawlerVessel','FishingWinch','FishItem','KnifeItem','FishingStation','FishSortingConveyor','FishProcessingBench']
WorkstationClasses=['TrawlWinchScenario','FishSortingScenario','FishCuttingScenario','FishReceivingBin']
FishingVariants=['FishSortingConveyor_Cod','FishSortingConveyor_Haddock']
ActorBlueprintRoles={'Sorting_Cod':'FishSortingConveyor_Cod','Sorting_Haddock':'FishSortingConveyor_Haddock'}


def Backup(Package):
    RelativePath=pathlib.Path('Content')/Package.removeprefix('/Game/')
    for Extension in ['.uasset','.umap']:
        Source=ProjectDirectory/RelativePath.with_suffix(Extension)
        Destination=BackupDirectory/RelativePath.with_suffix(Extension)
        if Source.exists() and not Destination.exists():
            Destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(Source,Destination)


def GetBlueprintClass(Name):
    Folder='/Game/Base/Blueprints'
    if Name in FishingClasses or Name in FishingVariants:
        Folder='/Game/Minigames/Fishing/Trawling/Blueprints'
    if Name in WorkstationClasses:
        Folder='/Game/Minigames/Fishing/Workstations/Blueprints'
    Blueprint=unreal.load_asset(Folder+'/BP_'+Name)
    if not Blueprint:
        raise RuntimeError('ConfigureProjectBlueprints must create the requested class first: '+Name)
    return Blueprint.generated_class()


def GetConfiguredClass(ActorClass,ActorName=''):
    if isinstance(ActorClass,unreal.Class):
        return ActorClass
    NativeClass=ActorClass.static_class()
    if NativeClass.get_path_name().startswith('/Script/YakutSimulator.'):
        return GetBlueprintClass(ActorBlueprintRoles.get(ActorName,NativeClass.get_name()))
    return ActorClass


def RestoreComponentConstructorDefaults(Blueprint):
    Properties={
        'WorkOperationComponent':['RequiredAction','RequiredItemType','RequiredCount','bUseExplicitResults'],
        'InteractionZoneComponent':['AcceptedItemType','bSnapOnPlacement','bAcceptReleasedItemsOnOverlap','BoxExtent','RelativeLocation','RelativeRotation','RelativeScale3D'],
        'VesselMovementComponent':['MaximumSpeed','Acceleration','TurnRate']
    }
    NativeDefaults=unreal.get_default_object(getattr(unreal,Blueprint.get_name().removeprefix('BP_')))
    Defaults=unreal.get_default_object(Blueprint.generated_class())
    Components={Component.get_name():Component for Component in Defaults.get_components_by_class(unreal.ActorComponent)}
    for Source in NativeDefaults.get_components_by_class(unreal.ActorComponent):
        Names=Properties.get(Source.get_class().get_name(),[])
        for Property in Names:
            Components[Source.get_name()].set_editor_property(Property,Source.get_editor_property(Property))


def ConfigureComponentClasses(Blueprints):
    Subsystem=unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    Library=unreal.SubobjectDataBlueprintFunctionLibrary
    ComponentClasses={Name:Blueprints[Name].generated_class() for Name in ['InteractionZoneComponent','WorkOperationComponent','VesselMovementComponent']}
    for Blueprint in Blueprints.values():
        if not isinstance(unreal.get_default_object(Blueprint.generated_class()),unreal.Actor):
            continue
        Changed=False
        while True:
            Selected=None
            for Subobject in Subsystem.k2_gather_subobject_data_for_blueprint(Blueprint):
                Component=Library.get_associated_object(Library.get_data(Subobject))
                if Component and Component.get_class().get_path_name().startswith('/Script/YakutSimulator.') and Component.get_class().get_name() in ComponentClasses:
                    Selected=(Subobject,Component.get_class().get_name())
                    break
            if Selected is None:
                break
            Subobject,ComponentClass=Selected
            Backup(Blueprint.get_path_name().split('.')[0])
            if not Subsystem.change_subobject_class(Subobject,ComponentClasses[ComponentClass]):
                raise RuntimeError('Native component Blueprint override failed: '+Blueprint.get_name())
            Changed=True
        if Changed:
            RestoreComponentConstructorDefaults(Blueprint)
            unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
            unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)


def AddVisual(Blueprint,Name,MeshName,Position,Collision):
    ComponentPath=Blueprint.generated_class().get_path_name()+':'+Name+'_GEN_VARIABLE'
    if unreal.find_object(None,ComponentPath):
        return
    Backup(Blueprint.get_path_name().split('.')[0])
    Component=ActorTools.add_component(Blueprint,unreal.StaticMeshComponent.static_class(),Name)
    Component.set_static_mesh(unreal.load_asset('/Game/Minigames/Fishing/Trawling/Meshes/'+MeshName))
    Component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Component.set_collision_profile_name(Collision)
    Component.set_relative_location(unreal.Vector(*Position),False,False)
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)



def ConfigureProjectBlueprints():
    from BuildStationaryWorkstations import ConfigureWorkstationBlueprints
    return ConfigureWorkstationBlueprints()


if __name__=='__main__':
    ConfigureProjectBlueprints()
