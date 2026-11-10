import importlib.util
import json
import pathlib
import shutil
import sys
import unreal
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
from ConfigureProjectBlueprints import ConfigureComponentClasses

Actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
Levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
Assets = unreal.AssetToolsHelpers.get_asset_tools()
BlueprintFolder = '/Game/Workstations/Blueprints'
InterfaceFolder = '/Game/Workstations/UI'
BackupDirectory = ProjectDirectory / 'Saved/Backups/StationaryWorkstations'
ReportDirectory = ProjectDirectory / 'Saved/Verification/StationaryWorkstations'


def Backup(Asset):
    Package = Asset.get_path_name().split('.')[0]
    Relative = pathlib.Path('Content') / Package.removeprefix('/Game/')
    for Extension in ['.uasset', '.umap']:
        Source = ProjectDirectory / Relative.with_suffix(Extension)
        Destination = BackupDirectory / Relative.with_suffix(Extension)
        if Source.exists() and not Destination.exists():
            Destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(Source, Destination)


def Load(Name, Folder='/Game/Fishing/Blueprints'):
    Asset = unreal.load_asset(Folder + '/' + Name)
    if not Asset:
        raise RuntimeError('Required asset is missing: ' + Folder + '/' + Name)
    return Asset


def CreateBlueprint(Name, Parent):
    Blueprint = unreal.load_asset(BlueprintFolder + '/' + Name)
    if Blueprint:
        Backup(Blueprint)
        return Blueprint
    Factory = unreal.BlueprintFactory()
    Factory.set_editor_property('parent_class', Parent)
    Blueprint = Assets.create_asset(Name, BlueprintFolder, unreal.Blueprint, Factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    return Blueprint


def SaveBlueprint(Blueprint):
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(Blueprint, False)


def ConfigureWorkstationBlueprints():
    Blueprints = {Name: CreateBlueprint('BP_' + Name, getattr(unreal, Name).static_class()) for Name in ['WorkstationScenario', 'TrawlWinchScenario', 'FishProcessingScenario', 'FishReceivingBin']}
    Components = {Name: Load('BP_' + Name, '/Game/Base/Blueprints') for Name in ['InteractionZoneComponent', 'WorkOperationComponent']}
    Components['VesselMovementComponent'] = Load('BP_VesselMovementComponent')
    ConfigureComponentClasses(dict(Blueprints, **Components))
    Lever = CreateBlueprint('BP_WorkstationLever', Load('BP_PhysicalLever').generated_class())
    Defaults = unreal.get_default_object(Lever.generated_class())
    Defaults.body.set_relative_location(unreal.Vector(0, 0, 20), False, False)
    Defaults.foundation.set_relative_scale3d(unreal.Vector(.13, .13, .045))
    Defaults.body.set_relative_scale3d(unreal.Vector(.035, .035, .28))
    Defaults.knob.set_relative_scale3d(unreal.Vector(2.2, 2.2, .275))
    Defaults.foundation.set_material(0, Load('M_DarkSteel', '/Game/Fishing/Materials'))
    Defaults.body.set_material(0, Load('M_Steel', '/Game/Fishing/Materials'))
    Defaults.knob.set_material(0, Load('M_FloatYellow', '/Game/Fishing/Materials'))
    SaveBlueprint(Lever)
    Winch = CreateBlueprint('BP_IndependentFishingWinch', Load('BP_FishingWinch').generated_class())
    Defaults = unreal.get_default_object(Winch.generated_class())
    Defaults.set_editor_property('CableSpeed', 150)
    SaveBlueprint(Winch)
    for Name, Kind, Color, Scale in [('ProcessingWaste', 'Offcut', 'FishSilver', (.14, .09, .04)), ('ConveyorDebris', 'Debris', 'Rope', (.12, .12, .12))]:
        Blueprint = CreateBlueprint('BP_' + Name, Load('BP_WorkItem', '/Game/Base/Blueprints').generated_class())
        Defaults = unreal.get_default_object(Blueprint.generated_class())
        Defaults.set_editor_property('ItemType', unreal.Name(Kind))
        DisplayName = 'Посторонний предмет'
        if Kind == 'Offcut':
            DisplayName = 'Отходы обработки'
        Defaults.set_editor_property('DisplayName', unreal.Text(DisplayName))
        Defaults.body.set_relative_scale3d(unreal.Vector(*Scale))
        Defaults.body.set_material(0, Load('M_' + Color, '/Game/Fishing/Materials'))
        SaveBlueprint(Blueprint)
    Fish = Load('BP_FishItem')
    Backup(Fish)
    unreal.get_default_object(Fish.generated_class()).set_editor_property('WasteItemClass', Load('BP_ProcessingWaste', BlueprintFolder).generated_class())
    SaveBlueprint(Fish)
    for Name in ['BP_FishingStation', 'BP_FishSortingConveyor', 'BP_FishSortingConveyor_Cod', 'BP_FishSortingConveyor_Haddock', 'BP_FishProcessingBench', 'BP_KnifeItem']:
        Blueprint = Load(Name)
        Backup(Blueprint)
        SaveBlueprint(Blueprint)
    Bin = unreal.get_default_object(Blueprints['FishReceivingBin'].generated_class())
    Bin.surface.set_static_mesh(Load('SM_FishingCrate', '/Game/Fishing/Meshes'))
    Bin.surface.set_relative_scale3d(unreal.Vector(.8, .8, .8))
    WinchScenario = unreal.get_default_object(Blueprints['TrawlWinchScenario'].generated_class())
    WinchScenario.trawl_visual.set_static_mesh(Load('SM_TrawlNet', '/Game/Fishing/Meshes'))
    WinchScenario.trawl_visual.set_relative_location(unreal.Vector(850, 0, -30), False, False)
    for Cable in [WinchScenario.port_cable_visual, WinchScenario.starboard_cable_visual]:
        Cable.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'))
        Cable.set_material(0, Load('M_Rope', '/Game/Fishing/Materials'))
    WinchScenario.port_fairlead.set_relative_location(unreal.Vector(510, -210, 110), False, False)
    WinchScenario.starboard_fairlead.set_relative_location(unreal.Vector(510, 210, 110), False, False)
    Processing = unreal.get_default_object(Blueprints['FishProcessingScenario'].generated_class())
    Processing.set_editor_property('FishItemClass', Fish.generated_class())
    Processing.set_editor_property('DebrisItemClass', Load('BP_ConveyorDebris', BlueprintFolder).generated_class())
    Processing.set_editor_property('FishSpecies', [Load('DA_Cod', '/Game/Fishing/Data'), Load('DA_Haddock', '/Game/Fishing/Data')])
    Processing.conveyor_surface.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    from CreateWorkstationMaterials import CreateWorkstationMaterials
    Processing.conveyor_surface.set_material(0, CreateWorkstationMaterials())
    Processing.conveyor_surface.set_relative_location(unreal.Vector(82, 0, 91), False, False)
    Processing.conveyor_surface.set_relative_scale3d(unreal.Vector(.55, 3.2, .08))
    Processing.conveyor_zone.set_relative_location(unreal.Vector(82, 0, 110), False, False)
    Processing.feed_point.set_relative_location(unreal.Vector(82, -85, 115), False, False)
    Processing.water_stream.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'))
    Processing.water_stream.set_material(0, Load('M_WheelhouseGlass', '/Game/Fishing/Materials'))
    Processing.water_stream.set_relative_location(unreal.Vector(45, 0, 135), False, False)
    Processing.water_stream.set_relative_scale3d(unreal.Vector(.025, .025, .5))
    for Blueprint in Blueprints.values():
        SaveBlueprint(Blueprint)
    Pawn = Load('BP_VirtualRealityPawn', '/Game/Base/Blueprints')
    Backup(Pawn)
    unreal.get_default_object(Pawn.generated_class()).set_editor_property('bLocomotionEnabled', False)
    SaveBlueprint(Pawn)
    Vessel = Load('BP_TrawlerVessel')
    Backup(Vessel)
    unreal.get_default_object(Vessel.generated_class()).movement.set_editor_property('bMovementEnabled', False)
    SaveBlueprint(Vessel)
    return Blueprints


def Spawn(Class, Name, Position, Rotation=(0, 0, 0)):
    Actor = Actors.spawn_actor_from_class(Class, unreal.Vector(*Position), unreal.Rotator(pitch=Rotation[0], yaw=Rotation[1], roll=Rotation[2]))
    Actor.set_actor_label(Name)
    return Actor


def Shape(Name, Position, Scale, Material='Steel', Mesh='/Engine/BasicShapes/Cube', Rotation=(0, 0, 0), Collision=True):
    Actor = Spawn(unreal.StaticMeshActor, Name, Position, Rotation)
    Component = Actor.static_mesh_component
    Component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Component.set_static_mesh(unreal.load_asset(Mesh))
    Component.set_material(0, Load('M_' + Material, '/Game/Fishing/Materials'))
    CollisionProfile = 'NoCollision'
    if Collision:
        CollisionProfile = 'BlockAllDynamic'
    Component.set_collision_profile_name(CollisionProfile)
    Actor.set_actor_scale3d(unreal.Vector(*Scale))
    return Actor


def Label(Name, Text, Position, Size=8, Rotation=(0, 180, 0), Color=(210, 220, 210, 255)):
    Text = {'ЛЕВЫЙ БАРАБАН': 'PORT DRUM', 'ПРАВЫЙ БАРАБАН': 'STARBOARD DRUM', 'НАТЯЖЕНИЕ Л': 'PORT TENSION', 'НАТЯЖЕНИЕ П': 'STBD TENSION', 'АВАРИЙНЫЙ СТОП': 'EMERGENCY STOP', 'ПОДТВЕРДИТЬ': 'CONFIRM', 'ЛЕВЫЙ ВАЕР': 'PORT WARP', 'ПРАВЫЙ ВАЕР': 'STARBOARD WARP', 'СКОРОСТЬ ЛЕНТЫ': 'BELT SPEED', 'ПОДАЧА РЫБЫ': 'FISH FEED', 'ВОДА': 'WATER', 'ЗАМЕНА ТАРЫ': 'CHANGE BINS', 'ПРОДУКЦИЯ': 'PRODUCT', 'ОТХОДЫ': 'WASTE', 'Треска': 'COD', 'Пикша': 'HADDOCK', 'УЧАСТОК ПЕРВИЧНОЙ ОБРАБОТКИ': 'FISH PROCESSING'}.get(Text, Text)
    Actor = Spawn(unreal.TextRenderActor, Name, Position, Rotation)
    Actor.text_render.set_text(Text)
    Actor.text_render.set_world_size(Size)
    Actor.text_render.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    Actor.text_render.set_text_render_color(unreal.Color(*Color))
    return Actor


def Control(Name, Title, Position, Color='FloatYellow'):
    Actor = Spawn(Load('BP_WorkstationLever', BlueprintFolder).generated_class(), Name, Position, (0, 90, 0))
    Actor.set_editor_property('DisplayName', unreal.Text(Title))
    Actor.knob.set_material(0, Load('M_' + Color, '/Game/Fishing/Materials'))
    Label(Name + '_Label', Title, (Position[0] - 7, Position[1], Position[2] - 2), 5)
    return Actor


def PrepareMap(Name):
    Path = '/Game/Maps/' + Name
    if unreal.EditorAssetLibrary.does_asset_exist(Path):
        raise RuntimeError('The workstation map already exists; preserve edits: ' + Path)
    if not Levels.new_level(Path):
        raise RuntimeError('Could not create workstation map: ' + Path)
    World = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    World.get_world_settings().set_editor_property('default_game_mode', Load('BP_PrototypeGameMode', '/Game/Base/Blueprints').generated_class())
    Spawn(unreal.PlayerStart, 'StationaryPlayerStart', (0, 0, 0))
    Exposure = Spawn(unreal.PostProcessVolume, 'WorkstationExposure', (0, 0, 150))
    Exposure.set_editor_property('unbound', True)
    Settings = Exposure.get_editor_property('settings')
    for Property, Value in {'bOverride_AutoExposureMethod': True, 'AutoExposureMethod': unreal.AutoExposureMethod.AEM_MANUAL, 'bOverride_AutoExposureApplyPhysicalCameraExposure': True, 'AutoExposureApplyPhysicalCameraExposure': False, 'bOverride_AutoExposureBias': True, 'AutoExposureBias': 0.0}.items():
        Settings.set_editor_property(Property, Value)
    Exposure.set_editor_property('settings', Settings)
    return World


def BuildWinchScene():
    World = PrepareMap('L_WinchStation')
    Shape('StationaryAftDeck', (100, 0, -15), (13, 9, .3), 'DeckDark')
    Shape('AftHull', (100, 0, -80), (13, 9, 1.1), 'HullBlue')
    for Side in [-1, 1]:
        Shape('Bulwark', (100, Side * 440, 45), (13, .15, 1), 'Ivory')
        for Height in [95, 120]:
            Shape('SafetyRail', (100, Side * 440, Height), (13, .04, .04), 'Steel')
        for Position in [-450, -150, 150, 450, 730]:
            Shape('RailUpright', (Position, Side * 440, 85), (.05, .05, .7), 'Steel')
        Shape('CableWarningStripe', (400, Side * 200, .5), (4, .12, .01), 'FloatYellow', Collision=False)
    Shape('WinchConsoleHousing', (70, 0, 90), (1, 1, 1), 'HullBlue', '/Game/Fishing/Meshes/SM_ControlConsole', (0, 90, 0))
    Scenario = Spawn(Load('BP_TrawlWinchScenario', BlueprintFolder).generated_class(), 'WinchWorkstation', (0, 0, 0))
    PortControl = Control('PortDrumControl', 'ЛЕВЫЙ БАРАБАН', (65, -24, 102))
    StarboardControl = Control('StarboardDrumControl', 'ПРАВЫЙ БАРАБАН', (65, 24, 102))
    Scenario.set_editor_property('PortTensionControl', Control('PortTensionControl', 'НАТЯЖЕНИЕ Л', (36, -48, 102), 'CrateBlue'))
    Scenario.set_editor_property('StarboardTensionControl', Control('StarboardTensionControl', 'НАТЯЖЕНИЕ П', (36, 48, 102), 'CrateBlue'))
    Scenario.set_editor_property('EmergencyControl', Control('EmergencyStop', 'АВАРИЙНЫЙ СТОП', (20, 0, 102), 'SafetyOrange'))
    Scenario.set_editor_property('CommandControl', Control('CommandConfirmation', 'ПОДТВЕРДИТЬ', (92, 0, 102), 'NetGreen'))
    for Name, Position, ControlActor, Property in [('PortWinch', (420, -235, 0), PortControl, 'PortWinch'), ('StarboardWinch', (420, 235, 0), StarboardControl, 'StarboardWinch')]:
        Winch = Spawn(Load('BP_IndependentFishingWinch', BlueprintFolder).generated_class(), Name, Position)
        Winch.set_editor_property('ControlLever', ControlActor)
        Scenario.set_editor_property(Property, Winch)
    Shape('AftGantry', (1260, 0, 0), (1, 1, 1), 'HullBlue', '/Game/Fishing/Meshes/SM_SternFrame')
    Scenario.console_display.set_relative_location(unreal.Vector(130, -105, 178), False, False)
    Scenario.console_display.set_relative_rotation(unreal.Rotator(yaw=155), False, False)
    Scenario.console_display.set_relative_scale3d(unreal.Vector(.055, .055, .055))
    Label('PortWinchSign', 'ЛЕВЫЙ ВАЕР', (350, -235, 150), 14)
    Label('StarboardWinchSign', 'ПРАВЫЙ ВАЕР', (350, 235, 150), 14)
    Sky = Shape('MaritimeSky', (0, 0, 0), (1000, 1000, 1000), 'MaritimeSky', '/Engine/EngineSky/SM_SkySphere', Collision=False)
    Sky.static_mesh_component.set_cast_shadow(False)
    Light = Spawn(unreal.DirectionalLight, 'DeckSun', (0, 0, 600), (-30, -50, 0))
    Light.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Light.light_component.set_intensity(3.0)
    Fill = Spawn(unreal.SkyLight, 'DeckFill', (0, 0, 400))
    Fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Fill.light_component.set_intensity(.7)
    Ocean = Spawn(unreal.WaterBodyOcean, 'WaterBodyOcean', (0, 0, -120))
    Ocean.set_editor_property('WaterWaves', None)
    Zone = Spawn(unreal.WaterZone, 'WaterZone', (0, 0, -120))
    Zone.set_editor_property('ZoneExtent', unreal.Vector2D(80000, 80000))
    Zone.water_mesh.set_editor_property('FarDistanceMaterial', unreal.load_asset('/Water/Materials/WaterSurface/LODs/Water_Material_Ocean_LOD'))
    Zone.water_mesh.set_editor_property('FarDistanceMeshExtent', 1000000.0)
    Ocean.get_water_body_component().set_editor_property('bAffectsLandscape', False)
    Ocean.get_water_body_component().set_editor_property('OceanExtents', unreal.Vector2D(80000, 80000))
    Ocean.get_water_body_component().set_editor_property('CollisionExtents', unreal.Vector(40000, 40000, 1000))
    Ocean.get_water_body_component().set_collision_profile_name('WaterBodyCollision')
    Spline = Ocean.get_water_spline()
    Spline.set_spline_points([unreal.Vector(-41000, -41000, 0), unreal.Vector(-40000, -41000, 0), unreal.Vector(-40000, -40000, 0), unreal.Vector(-41000, -40000, 0)], unreal.SplineCoordinateSpace.LOCAL, False)
    Spline.set_closed_loop(True, True)
    Spline.k2_synchronize_and_broadcast_data_change()
    Ocean.get_water_body_component().set_water_zone_override(Zone)
    Ocean.get_water_body_component().set_water_material(unreal.load_asset('/Water/Materials/WaterSurface/Water_Material_Ocean'))
    Levels.save_current_level()
    return World


def BuildProcessingScene():
    World = PrepareMap('L_ProcessingStation')
    Shape('FactoryFloor', (90, 0, -10), (6.6, 7.4, .2), 'DeckDark')
    Shape('BackWall', (410, 0, 175), (.18, 7.4, 3.5), 'Ivory')
    Shape('PortWall', (90, -365, 175), (6.6, .18, 3.5), 'Ivory')
    Shape('StarboardWall', (90, 365, 175), (6.6, .18, 3.5), 'Ivory')
    Shape('RearWall', (-230, 0, 175), (.18, 7.4, 3.5), 'Ivory')
    Shape('Ceiling', (90, 0, 355), (6.6, 7.4, .15), 'Ivory')
    for Height, Material in [(130, 'HullBlue'), (112, 'FloatYellow')]:
        Shape('WallStripe', (399, 0, Height), (.015, 7.3, .13), Material, Collision=False)
        for Side in [-1, 1]:
            Shape('SideStripe', (90, Side * 354, Height), (6.4, .015, .13), Material, Collision=False)
    for Lateral in [-280, 0, 280]:
        Shape('CeilingPipe', (90, Lateral, 320), (.07, .07, 6.1), 'Steel', '/Engine/BasicShapes/Cylinder', (90, 0, 0), Collision=False)
    for Longitudinal in [-100, 150, 340]:
        for Lateral in [-200, 200]:
            Shape('LampHousing', (Longitudinal, Lateral, 337), (.9, .28, .12), 'DarkSteel', Collision=False)
            Lamp = Spawn(unreal.RectLight, 'CeilingLamp', (Longitudinal, Lateral, 327), (-90, 0, 0))
            Lamp.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
            Lamp.light_component.set_intensity_units(unreal.LightUnits.LUMENS)
            Lamp.light_component.set_intensity(1200)
            Lamp.light_component.set_cast_shadows(False)
            Lamp.light_component.set_editor_property('source_width', 70)
            Lamp.light_component.set_editor_property('source_height', 18)
            Lamp.light_component.set_editor_property('temperature', 4700)
            Lamp.light_component.set_editor_property('use_temperature', True)
    Scenario = Spawn(Load('BP_FishProcessingScenario', BlueprintFolder).generated_class(), 'ProcessingWorkstation', (0, 0, 0))
    for Lateral in [-150, 150]:
        Shape('ConveyorRoller', (82, Lateral, 86), (.17, .17, .56), 'Steel', '/Engine/BasicShapes/Cylinder', (0, 0, 90))
        for Longitudinal in [57, 107]:
            Shape('ConveyorLeg', (Longitudinal, Lateral, 42), (.05, .05, .84), 'Steel')
    for Longitudinal in [50, 114]:
        Shape('ConveyorSide', (Longitudinal, 0, 92), (.05, 3.3, .18), 'Steel')
    Shape('FeedMachine', (82, -220, 125), (.8, .7, 2.5), 'Steel')
    Shape('FeedOpening', (80, -183, 110), (.6, .03, .4), 'Rubber', Collision=False)
    Bench = Spawn(Load('BP_FishProcessingBench').generated_class(), 'CuttingBench', (43, 0, 98), (0, 90, 0))
    Bench.surface.set_relative_scale3d(unreal.Vector(.5, .55, 1))
    Bench.intake.set_box_extent(unreal.Vector(24, 22, 16), False)
    Bench.intake.set_relative_location(unreal.Vector(0, 0, 26), False, False)
    Bench.set_editor_property('Scenario', Scenario)
    Scenario.set_editor_property('ProcessingBench', Bench)
    Shape('CuttingBoard', (43, 0, 112), (.42, .42, .025), 'Ivory')
    for SpeciesName, Lateral in [('Cod', -63), ('Haddock', 63)]:
        Tray = Spawn(Load('BP_FishSortingConveyor_' + SpeciesName).generated_class(), 'SortingTray_' + SpeciesName, (58, Lateral, 100))
        Tray.surface.set_static_mesh(Load('SM_FishingCrate', '/Game/Fishing/Meshes'))
        Tray.surface.set_relative_scale3d(unreal.Vector(.7, .7, .55))
        Tray.intake.set_box_extent(unreal.Vector(24, 19, 17), False)
        Tray.intake.set_relative_location(unreal.Vector(0, 0, 8), False, False)
        Tray.set_editor_property('BeltSpeed', 0)
        Tray.set_editor_property('Scenario', Scenario)
        Label('SpeciesSign_' + SpeciesName, str(Load('DA_' + SpeciesName, '/Game/Fishing/Data').display_name), (80, Lateral, 126), 7)
    Bins = []
    for Name, Lateral, Waste, Color, Title in [('ProductBin', -60, False, 'CrateBlue', 'ПРОДУКЦИЯ'), ('WasteBin', 60, True, 'SafetyOrange', 'ОТХОДЫ')]:
        Bin = Spawn(Load('BP_FishReceivingBin', BlueprintFolder).generated_class(), Name, (3, Lateral, 88))
        Bin.set_editor_property('bReceiveWaste', Waste)
        Bin.set_editor_property('Scenario', Scenario)
        Bin.surface.set_material(0, Load('M_' + Color, '/Game/Fishing/Materials'))
        Bins.append(Bin)
        Label(Name + 'Sign', Title, (-18, Lateral, 98), 6, (0, 180, 0))
    Scenario.set_editor_property('ReceivingBins', Bins)
    Scenario.set_editor_property('ConveyorControl', Control('ConveyorSpeedControl', 'СКОРОСТЬ ЛЕНТЫ', (20, -35, 114)))
    Scenario.set_editor_property('FeedControl', Control('FishFeedControl', 'ПОДАЧА РЫБЫ', (20, 35, 114)))
    Scenario.set_editor_property('WaterControl', Control('WashWaterControl', 'ВОДА', (68, 28, 114), 'CrateBlue'))
    Scenario.set_editor_property('CommandControl', Control('ReplaceContainersControl', 'ЗАМЕНА ТАРЫ', (-8, 35, 114), 'NetGreen'))
    Knife = Spawn(Load('BP_KnifeItem').generated_class(), 'ProcessingKnife', (37, -22, 115), (0, 90, 0))
    Knife.set_editor_property('Sharpness', 1)
    Scenario.set_editor_property('ProcessingKnife', Knife)
    Shape('WashPipe', (58, 18, 148), (.04, .04, .8), 'Steel', '/Engine/BasicShapes/Cylinder', (0, 0, 0), Collision=False)
    Shape('WashSpout', (48, 9, 168), (.035, .035, .25), 'Steel', '/Engine/BasicShapes/Cylinder', (90, 0, 0), Collision=False)
    Scenario.console_display.set_relative_location(unreal.Vector(162, 0, 199), False, False)
    Scenario.console_display.set_relative_rotation(unreal.Rotator(yaw=180), False, False)
    Scenario.console_display.set_relative_scale3d(unreal.Vector(.075, .075, .075))
    Label('ProcessingRoomSign', 'УЧАСТОК ПЕРВИЧНОЙ ОБРАБОТКИ', (398, 0, 280), 15)
    Fill = Spawn(unreal.PointLight, 'WorkAreaFill', (0, 0, 230))
    Fill.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Fill.light_component.set_intensity_units(unreal.LightUnits.LUMENS)
    Fill.light_component.set_intensity(2500)
    Fill.light_component.set_editor_property('attenuation_radius', 600)
    Fill.light_component.set_cast_shadows(False)
    for Actor in Actors.get_all_level_actors():
        if isinstance(Actor, unreal.PostProcessVolume):
            Settings = Actor.get_editor_property('settings')
            Settings.set_editor_property('AutoExposureBias', -2)
            Actor.set_editor_property('settings', Settings)
    Ambient = Spawn(unreal.SkyLight, 'InteriorAmbient', (90, 0, 200))
    Ambient.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Ambient.light_component.set_editor_property('source_type', unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
    Ambient.light_component.set_editor_property('cubemap', unreal.load_asset('/Engine/MapTemplates/Sky/DaylightAmbientCubemap'))
    Ambient.light_component.set_intensity(.6)
    Levels.save_current_level()
    return World


def ConfigureScenarioMaps():
    GameMode = Load('BP_PrototypeGameMode', '/Game/Base/Blueprints')
    Backup(GameMode)
    Defaults = unreal.get_default_object(GameMode.generated_class())
    Entries = []
    for MapName, Title in [('L_WinchStation', 'Оператор траловых лебёдок'), ('L_ProcessingStation', 'Обработка улова')]:
        Entry = unreal.ScenarioMapEntry()
        Entry.set_editor_property('display_name', unreal.Text(Title))
        Entry.set_editor_property('map', Load(MapName, '/Game/Maps'))
        Entries.append(Entry)
    Defaults.set_editor_property('ScenarioMaps', Entries)
    SaveBlueprint(GameMode)


if __name__ == '__main__':
    ConfigureWorkstationBlueprints()
    BuildWinchScene()
    BuildProcessingScene()
    ConfigureScenarioMaps()
    print('Stationary workstation maps configured')
