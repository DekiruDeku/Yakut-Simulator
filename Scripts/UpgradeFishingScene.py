import pathlib
import shutil
import unreal
import sys

ProjectDirectory=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory/'Scripts'))
from ConfigureProjectBlueprints import GetConfiguredClass,GetBlueprintClass
from ConfigureFishingOcean import ConfigureFishingOcean
ActorSubsystem=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LevelSubsystem=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
AssetTools=unreal.AssetToolsHelpers.get_asset_tools()
BackupDirectory=ProjectDirectory/'Saved/Backups/FishingScenario'
BackupDirectory.mkdir(parents=True,exist_ok=True)
MapPackage=ProjectDirectory/'Content/Prototype/Maps/L_Fishing.umap'
BackupPackage=BackupDirectory/'L_Fishing.umap'
if not BackupPackage.exists():
    shutil.copy2(MapPackage,BackupPackage)
LevelSubsystem.load_level('/Game/Prototype/Maps/L_Fishing')
World=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
Actors=list(ActorSubsystem.get_all_level_actors())
Vessels=[Actor for Actor in Actors if isinstance(Actor,unreal.TrawlerVessel)]
if len(Vessels)!=1:
    raise RuntimeError('Exactly one fishing vessel is required')
Vessel,=Vessels
Throttle=Vessel.throttle_lever
Steering=Vessel.steering_lever
for Actor in Actors:
    if Actor in [Vessel,Throttle,Steering]:
        continue
    if isinstance(Actor,(unreal.StaticMeshActor,unreal.WorkItem,unreal.FishingStation,unreal.FishingScenario,unreal.FishingWinch,unreal.Trawl,unreal.FishSchool,unreal.FishingSonar,unreal.TextRenderActor)):
        ActorSubsystem.destroy_actor(Actor)


def Spawn(ActorClass,Name,Position,Rotation=None,Attach=True):
    if Rotation is None:
        Rotation=unreal.Rotator()
    Actor=ActorSubsystem.spawn_actor_from_class(GetConfiguredClass(ActorClass,Name),unreal.Vector(*Position),Rotation)
    Actor.set_actor_label(Name)
    if Attach:
        if isinstance(Actor,unreal.StaticMeshActor):
            Actor.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
        if isinstance(Actor,unreal.TextRenderActor):
            Actor.get_component_by_class(unreal.TextRenderComponent).set_mobility(unreal.ComponentMobility.MOVABLE)
        Actor.attach_to_actor(Vessel,'',unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,unreal.AttachmentRule.KEEP_WORLD,False)
        Vessel.hull.ignore_actor_when_moving(Actor,True)
    return Actor


def Mesh(Name):
    return unreal.load_asset('/Game/Fishing/Meshes/'+Name)


def Label(Text,Position,Size=9):
    Actor=Spawn(unreal.TextRenderActor,'Label_'+Text,Position,unreal.Rotator(yaw=180))
    Actor.get_component_by_class(unreal.TextRenderComponent).set_text(unreal.Text(Text))
    Actor.get_component_by_class(unreal.TextRenderComponent).set_world_size(Size)
    Actor.get_component_by_class(unreal.TextRenderComponent).set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    Actor.get_component_by_class(unreal.TextRenderComponent).set_text_render_color(unreal.Color(220,230,225,255))
    return Actor


def Lever(Name,Position,Rotation=None):
    Result=Spawn(unreal.PhysicalLever,Name,Position,Rotation)
    return Result


Vessel.hull.set_collision_response_to_channel(unreal.CollisionChannel.ECC_VISIBILITY,unreal.CollisionResponseType.ECR_IGNORE)
Vessel.set_actor_location(unreal.Vector(350,0,-45),False,False)

Ocean=ConfigureFishingOcean(SaveMap=False)

for Actor in ActorSubsystem.get_all_level_actors():
    if isinstance(Actor,unreal.DirectionalLight):
        Actor.set_actor_rotation(unreal.Rotator(pitch=-35,yaw=-38),False)
        Actor.light_component.set_editor_property('intensity',45000)
        Actor.light_component.set_editor_property('atmosphere_sun_light',True)
        Actor.light_component.set_editor_property('light_source_angle',3)
    if isinstance(Actor,unreal.SkyLight):
        Actor.light_component.set_editor_property('intensity',1)
        Actor.light_component.set_editor_property('real_time_capture',True)
    if isinstance(Actor,unreal.PostProcessVolume):
        Settings=Actor.get_editor_property('settings')
        for Property,Value in {
            'override_auto_exposure_method':True,'auto_exposure_method':unreal.AutoExposureMethod.AEM_MANUAL,
            'override_auto_exposure_apply_physical_camera_exposure':True,'auto_exposure_apply_physical_camera_exposure':True,
            'override_camera_iso':True,'camera_iso':100,
            'override_camera_shutter_speed':True,'camera_shutter_speed':125,
            'override_depth_of_field_fstop':True,'depth_of_field_fstop':8,
            'override_auto_exposure_bias':True,'auto_exposure_bias':0,
            'override_motion_blur_amount':True,'motion_blur_amount':0,
            'override_bloom_intensity':True,'bloom_intensity':0.08,
        }.items():
            Settings.set_editor_property(Property,Value)
        Actor.set_editor_property('settings',Settings)

Scenario=Spawn(unreal.FishingScenario,'FishingScenario',(70,-265,165),unreal.Rotator(yaw=90))
Scenario.set_editor_property('vessel',Vessel)
Sonar=Spawn(unreal.FishingSonar,'FishingSonar',(70,-265,100))
Sonar.set_editor_property('vessel',Vessel)
Sonar.set_editor_property('water_height',Ocean.get_actor_location().z)
Scenario.set_editor_property('sonar',Sonar)
Trawl=Spawn(unreal.Trawl,'FishingTrawl',(-350,0,155),Attach=False)
Trawl.set_editor_property('vessel',Vessel)
Trawl.set_editor_property('scenario',Scenario)
Trawl.set_editor_property('water_height',Ocean.get_actor_location().z)
Scenario.set_editor_property('trawl',Trawl)
Winch=Spawn(unreal.FishingWinch,'FishingWinch',(-80,-170,0))
Winch.set_editor_property('trawl',Trawl)
Scenario.set_editor_property('winch',Winch)
Throttle.set_actor_location(unreal.Vector(65,-125,55),False,False)
Steering.set_actor_location(unreal.Vector(65,-190,55),False,False)
WinchControl=Lever('TrawlControlLever',(-80,-70,55),unreal.Rotator(yaw=-90))
Winch.set_editor_property('control_lever',WinchControl)
UnloadControl=Lever('CatchReleaseLever',(-200,-70,55),unreal.Rotator(yaw=-90))
Winch.set_editor_property('unload_lever',UnloadControl)
ScanControl=Lever('SonarScanLever',(65,-310,55),unreal.Rotator(yaw=-90))
Sonar.set_editor_property('scan_lever',ScanControl)
Label('ХОД',(33,-125,88),7)
Label('РУЛЬ',(33,-190,88),7)
Label('ТРАЛ',(-112,-70,88),7)
Label('ВЫГРУЗКА',(-232,-70,88),7)

Definitions=[]
for Name,Title,FishMesh,Location in [
    ('Cod','Треска','SM_FishCod',(-1500,0,-400)),
    ('Haddock','Пикша','SM_FishHaddock',(4000,1100,-420)),
]:
    Path='/Game/Fishing/Data/DA_'+Name
    Definition=unreal.load_asset(Path)
    if not Definition:
        Factory=unreal.DataAssetFactory()
        Factory.set_editor_property('data_asset_class',GetBlueprintClass('FishSpeciesDefinition'))
        Definition=AssetTools.create_asset('DA_'+Name,'/Game/Fishing/Data',unreal.FishSpeciesDefinition,Factory)
        Definition.set_editor_property('species_identifier',Name)
        Definition.set_editor_property('display_name',unreal.Text(Title))
        Definition.set_editor_property('mesh',Mesh(FishMesh))
        Definition.set_editor_property('material',unreal.load_asset('/Game/Fishing/Materials/M_FishSilver'))
        unreal.EditorAssetLibrary.save_loaded_asset(Definition)
    Definitions.append(Definition)
    School=Spawn(unreal.FishSchool,'FishSchool_'+Name,Location,Attach=False)
    School.set_editor_property('species',Definition)

Conveyors=[]
ConveyorPower=Lever('ConveyorPowerLever',(200,310,55),unreal.Rotator(yaw=-90))
for Definition,Position in zip(Definitions,[(260,190,90),(440,190,90)]):
    Conveyor=Spawn(unreal.FishSortingConveyor,'Sorting_'+str(Definition.species_identifier),Position,unreal.Rotator(yaw=180))
    Conveyor.set_editor_property('accepted_species',Definition)
    Conveyor.set_editor_property('scenario',Scenario)
    Conveyor.set_editor_property('power_lever',ConveyorPower)
    Conveyors.append(Conveyor)
    Label(str(Definition.display_name),(Position[0]-80,Position[1],122),10)
Scenario.set_editor_property('sorting_conveyors',Conveyors)
Bench=Spawn(unreal.FishProcessingBench,'FishProcessingBench',(110,0,90))
Bench.set_editor_property('scenario',Scenario)
Scenario.set_editor_property('processing_bench',Bench)
Stone=Spawn(unreal.KnifeSharpeningStation,'SharpeningStone',(155,-20,97))
Stone.set_editor_property('scenario',Scenario)
Scenario.set_editor_property('sharpening_stone',Stone)
Knife=Spawn(unreal.KnifeItem,'FishingKnife',(85,-15,100),Attach=False)
Scenario.set_editor_property('knife',Knife)
Container=Spawn(unreal.FishContainerItem,'PackingContainer',(245,-15,24),Attach=False)
Scenario.set_editor_property('packing_container',Container)
Freezer=Spawn(unreal.FishFreezer,'FishFreezer',(350,-275,0))
Freezer.set_editor_property('scenario',Scenario)
Scenario.set_editor_property('freezer',Freezer)
Label('ОБРАБОТКА',(25,0,115),9)
Label('МОРОЗИЛЬНИК',(270,-275,105),8)

for Position in [(1800,-1600,-70),(3400,1200,-70),(-2000,2300,-70)]:
    Buoy=Spawn(unreal.StaticMeshActor,'NavigationBuoy',Position,Attach=False)
    Buoy.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'))
    Buoy.static_mesh_component.set_material(0,unreal.load_asset('/Game/Fishing/Materials/M_SafetyOrange'))
    Buoy.static_mesh_component.set_collision_profile_name('NoCollision')
    Buoy.set_actor_scale3d(unreal.Vector(.6,.6,1.5))

LevelSubsystem.save_current_level()
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-1600,-1900,900),unreal.Rotator(pitch=-18,yaw=41))
print('Full fishing scene and equipment references saved')
