import json
import pathlib
import shutil
import unreal
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools

ProjectDirectory=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
BackupDirectory=ProjectDirectory/'Saved/Backups/BlueprintConfiguration'
VerificationDirectory=ProjectDirectory/'Saved/Verification/BlueprintConfiguration'
PrototypeClasses=['InteractionZoneComponent','WorkOperationComponent','WorkItem','ScenarioDirector','PlacementStation','VirtualRealityPawn','PrototypePlayerController','PrototypeGameMode']
FishingClasses=['VesselMovementComponent','FishSpeciesDefinition','PhysicalLever','TrawlerVessel','Trawl','FishingScenario','FishingSonar','FishingWinch','FishSchool','FishItem','FishContainerItem','KnifeItem','FishingStation','FishSortingConveyor','FishProcessingBench','FishFreezer','KnifeSharpeningStation']
LeverConfigurations={
    'VesselThrottleLever':('HullBlue','Ход'),
    'VesselSteeringLever':('Steel','Руль'),
    'TrawlControlLever':('SafetyOrange','Трал'),
    'CatchReleaseLever':('FloatYellow','Выгрузка'),
    'SonarScanLever':('NetGreen','Сканирование'),
    'ConveyorPowerLever':('CrateBlue','Конвейеры')
}
FishingVariants=list(LeverConfigurations)+['FishSchool_Cod','FishSchool_Haddock','FishSortingConveyor_Cod','FishSortingConveyor_Haddock','CoastalZone']
ActorBlueprintRoles={'ThrottleLever':'VesselThrottleLever','SteeringLever':'VesselSteeringLever','TrawlControlLever':'TrawlControlLever','CatchReleaseLever':'CatchReleaseLever','SonarScanLever':'SonarScanLever','ConveyorPowerLever':'ConveyorPowerLever','FishSchool_Cod':'FishSchool_Cod','FishSchool_Haddock':'FishSchool_Haddock','Sorting_Cod':'FishSortingConveyor_Cod','Sorting_Haddock':'FishSortingConveyor_Haddock'}


def Backup(Package):
    RelativePath=pathlib.Path('Content')/Package.removeprefix('/Game/')
    for Extension in ['.uasset','.umap']:
        Source=ProjectDirectory/RelativePath.with_suffix(Extension)
        Destination=BackupDirectory/RelativePath.with_suffix(Extension)
        if Source.exists() and not Destination.exists():
            Destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(Source,Destination)


def GetBlueprintClass(Name):
    Folder='/Game/Prototype/Blueprints'
    if Name in FishingClasses or Name in FishingVariants:
        Folder='/Game/Fishing/Blueprints'
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
    Component.set_static_mesh(unreal.load_asset('/Game/Fishing/Meshes/'+MeshName))
    Component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Component.set_collision_profile_name(Collision)
    Component.set_relative_location(unreal.Vector(*Position),False,False)
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)


def ConfigureCoastalZoneBlueprint():
    Path='/Game/Fishing/Blueprints/BP_CoastalZone'
    if unreal.EditorAssetLibrary.does_asset_exist(Path):
        return unreal.load_asset(Path)
    Factory=unreal.BlueprintFactory()
    Factory.set_editor_property('parent_class',unreal.Actor)
    Blueprint=unreal.AssetToolsHelpers.get_asset_tools().create_asset('BP_CoastalZone','/Game/Fishing/Blueprints',unreal.Blueprint,Factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    Defaults=unreal.get_default_object(Blueprint.generated_class())
    Defaults.set_editor_property('Tags',[unreal.Name('Shore')])
    Spline=ActorTools.add_component(Blueprint,unreal.SplineComponent.static_class(),'BoundarySpline')
    Spline.set_spline_points([unreal.Vector(-500,-500,0),unreal.Vector(500,-500,0),unreal.Vector(500,500,0),unreal.Vector(-500,500,0)],unreal.SplineCoordinateSpace.LOCAL,False)
    for PointIndex in range(Spline.get_number_of_spline_points()):
        Spline.set_spline_point_type(PointIndex,unreal.SplinePointType.LINEAR,False)
    Spline.set_closed_loop(True,True)
    Graph=BlueprintTools.get_graph(Blueprint,'EventGraph')
    for Node in list(BlueprintTools.find_nodes(Graph)):
        BlueprintTools.delete_node(Node)
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)
    return Blueprint


def ConfigureProjectBlueprints():
    AssetTools=unreal.AssetToolsHelpers.get_asset_tools()
    Blueprints={}
    Created=set()
    for Names,Folder in [(PrototypeClasses,'/Game/Prototype/Blueprints'),(FishingClasses,'/Game/Fishing/Blueprints')]:
        for Name in Names:
            Path=Folder+'/BP_'+Name
            Blueprint=unreal.load_asset(Path)
            if not Blueprint:
                Factory=unreal.BlueprintFactory()
                Factory.set_editor_property('parent_class',getattr(unreal,Name))
                Blueprint=AssetTools.create_asset('BP_'+Name,Folder,unreal.Blueprint,Factory)
                if not Blueprint:
                    raise RuntimeError('Blueprint creation failed: '+Path)
                Created.add(Name)
                unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
            Blueprints[Name]=Blueprint

    ConfigureComponentClasses(Blueprints)
    Defaults={Name:unreal.get_default_object(Blueprint.generated_class()) for Name,Blueprint in Blueprints.items()}
    if 'VirtualRealityPawn' in Created:
        Pawn=Defaults['VirtualRealityPawn']
        Pawn.set_editor_property('interaction_mapping',unreal.load_asset('/Game/Prototype/Input/IMC_Interaction'))
        for Property,Asset in {
            'grab_left_action':'IA_GrabLeft','grab_right_action':'IA_GrabRight',
            'use_left_action':'IA_UseLeft','use_right_action':'IA_UseRight',
            'teleport_action':'IA_Teleport','snap_turn_action':'IA_SnapTurn',
            'toggle_menu_action':'IA_ToggleMenu','cancel_action':'IA_Cancel',
            'desktop_move_action':'IA_DesktopMove','desktop_look_action':'IA_DesktopLook',
            'teleport_hold_action':'IA_TeleportHold'
        }.items():
            Pawn.set_editor_property(Property,unreal.load_asset('/Game/Prototype/Input/'+Asset))
    if 'PrototypePlayerController' in Created:
        Defaults['PrototypePlayerController'].set_editor_property('main_menu_class',unreal.load_asset('/Game/Prototype/UI/WBP_MainMenu').generated_class())
    if 'PrototypeGameMode' in Created:
        GameMode=Defaults['PrototypeGameMode']
        GameMode.set_editor_property('default_pawn_class',Blueprints['VirtualRealityPawn'].generated_class())
        GameMode.set_editor_property('player_controller_class',Blueprints['PrototypePlayerController'].generated_class())
        Entries=[]
        for Name,Title in [('L_Fishing','Рыболовный траулер'),('L_Clinic','Кабинет терапевта')]:
            Entry=unreal.ScenarioMapEntry()
            Entry.set_editor_property('display_name',unreal.Text(Title))
            Entry.set_editor_property('map',unreal.load_asset('/Game/Prototype/Maps/'+Name))
            Entries.append(Entry)
        GameMode.set_editor_property('scenario_maps',Entries)
        GameMode.set_editor_property('main_menu_map',unreal.load_asset('/Game/Prototype/Maps/L_MainMenu'))
    if 'Trawl' in Created:
        Defaults['Trawl'].set_editor_property('fish_item_class',Blueprints['FishItem'].generated_class())
        Defaults['Trawl'].port_cable.set_material(0,unreal.load_asset('/Game/Fishing/Materials/M_Rope'))
        Defaults['Trawl'].starboard_cable.set_material(0,unreal.load_asset('/Game/Fishing/Materials/M_Rope'))
    if 'TrawlerVessel' in Created:
        Defaults['TrawlerVessel'].hull.set_box_extent(unreal.Vector(900,400,45),False)

    MeshAssignments={
        'Trawl':{'net':'SM_TrawlNet'},
        'FishingWinch':{'foundation':'SM_WinchBase','drum':'SM_WinchDrum'},
        'KnifeItem':{'body':'SM_FishingKnife'},
        'FishContainerItem':{'body':'SM_FishingCrate'},
        'FishSortingConveyor':{'surface':'SM_FishingConveyor'},
        'FishProcessingBench':{'surface':'SM_ProcessingBench'},
        'FishFreezer':{'surface':'SM_FishingFreezer'},
        'KnifeSharpeningStation':{'surface':'SM_SharpeningStone'}
    }
    for Name,Assignments in MeshAssignments.items():
        if Name not in Created:
            continue
        for Property,Mesh in Assignments.items():
            Component=Defaults[Name].get_editor_property(Property)
            Component.set_static_mesh(unreal.load_asset('/Game/Fishing/Meshes/'+Mesh))
            Component.set_relative_scale3d(unreal.Vector(1,1,1))
        if Name=='FishingWinch':
            Defaults[Name].drum.set_relative_location(unreal.Vector(0,0,72),False,False)
        if Name=='KnifeItem':
            Defaults[Name].body.set_mass_override_in_kg('',.35,True)
        if Name=='FishContainerItem':
            Defaults[Name].body.set_mass_override_in_kg('',5,True)
        if Name=='FishFreezer':
            Defaults[Name].intake.set_relative_location(unreal.Vector(0,0,35),False,False)
        if Name=='KnifeSharpeningStation':
            Defaults[Name].intake.set_box_extent(unreal.Vector(25,10,3),False)
            Defaults[Name].intake.set_relative_location(unreal.Vector(0,0,5),False,False)

    for Name in Created:
        unreal.BlueprintEditorLibrary.compile_blueprint(Blueprints[Name])
        unreal.EditorAssetLibrary.save_loaded_asset(Blueprints[Name],False)
    for Name,(Color,Title) in LeverConfigurations.items():
        Path='/Game/Fishing/Blueprints/BP_'+Name
        if unreal.EditorAssetLibrary.does_asset_exist(Path):
            continue
        Factory=unreal.BlueprintFactory()
        Factory.set_editor_property('parent_class',Blueprints['PhysicalLever'].generated_class())
        Blueprint=AssetTools.create_asset('BP_'+Name,'/Game/Fishing/Blueprints',unreal.Blueprint,Factory)
        unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
        Lever=unreal.get_default_object(Blueprint.generated_class())
        Lever.foundation.set_material(0,unreal.load_asset('/Game/Fishing/Materials/M_DarkSteel'))
        Lever.body.set_material(0,unreal.load_asset('/Game/Fishing/Materials/M_Steel'))
        Lever.knob.set_material(0,unreal.load_asset('/Game/Fishing/Materials/M_'+Color))
        Lever.set_editor_property('display_name',unreal.Text(Title))
        unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)
    for SpeciesName in ['Cod','Haddock']:
        Species=unreal.load_asset('/Game/Fishing/Data/DA_'+SpeciesName)
        for Base,Property in [('FishSchool','species'),('FishSortingConveyor','accepted_species')]:
            Name=Base+'_'+SpeciesName
            Path='/Game/Fishing/Blueprints/BP_'+Name
            if unreal.EditorAssetLibrary.does_asset_exist(Path):
                continue
            Factory=unreal.BlueprintFactory()
            Factory.set_editor_property('parent_class',Blueprints[Base].generated_class())
            Blueprint=AssetTools.create_asset('BP_'+Name,'/Game/Fishing/Blueprints',unreal.Blueprint,Factory)
            unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
            unreal.get_default_object(Blueprint.generated_class()).set_editor_property(Property,Species)
            if Base=='FishSchool':
                unreal.get_default_object(Blueprint.generated_class()).set_editor_property('radius',1400)
            unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
            unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)

    for Name,MeshName,Position,Collision in [
        ('HullVisual','SM_TrawlerHull',(0,0,0),'NoCollision'),
        ('DeckVisual','SM_TrawlerDeck',(0,0,0),'BlockAllDynamic'),
        ('WheelhouseVisual','SM_Wheelhouse',(0,0,0),'BlockAllDynamic'),
        ('WheelhouseGlassVisual','SM_WheelhouseGlass',(0,0,0),'BlockAllDynamic'),
        ('SternFrameVisual','SM_SternFrame',(0,0,0),'NoCollision'),
        ('ControlConsoleVisual','SM_ControlConsole',(-285,-160,95),'BlockAllDynamic')
    ]:
        AddVisual(Blueprints['TrawlerVessel'],Name,MeshName,Position,Collision)
    AddVisual(Blueprints['FishingScenario'],'DisplayHousing','SM_SonarDisplay',(0,0,0),'BlockAllDynamic')
    ConfigureCoastalZoneBlueprint()
    VerificationDirectory.mkdir(parents=True,exist_ok=True)
    (VerificationDirectory/'Blueprints.json').write_text(json.dumps({Name:Blueprint.get_path_name() for Name,Blueprint in Blueprints.items()},indent=2),encoding='utf-8')
    print('Project Blueprint classes configured:',len(Blueprints))
    return Blueprints


if __name__=='__main__':
    ConfigureProjectBlueprints()
