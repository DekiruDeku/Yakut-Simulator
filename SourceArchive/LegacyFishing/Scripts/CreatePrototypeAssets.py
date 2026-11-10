if __name__ == '__main__':
    raise RuntimeError('This legacy generator is retired. Use BuildStationaryWorkstations.py for the active stationary minigames.')

import unreal
import sys

sys.path.append(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()+'Scripts'))
from ConfigureProjectBlueprints import GetConfiguredClass,GetBlueprintClass


AssetTools = unreal.AssetToolsHelpers.get_asset_tools()
ActorSubsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LevelSubsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
Root = "/Game/Prototype"


def CreateAsset(Name, Folder, AssetClass, Factory):
    Path = Folder + "/" + Name
    if unreal.EditorAssetLibrary.does_asset_exist(Path):
        return unreal.load_asset(Path)
    Asset = AssetTools.create_asset(Name, Folder, AssetClass, Factory)
    if not Asset:
        raise RuntimeError("Asset creation failed: " + Path)
    return Asset


def CreateDataAsset(Name, AssetClass):
    Factory = unreal.DataAssetFactory()
    Factory.set_editor_property("data_asset_class", AssetClass)
    return CreateAsset(Name, Root + "/Input", AssetClass, Factory)


def CreateMaterial(Name, Color):
    Material = CreateAsset(Name, Root + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    if not unreal.MaterialEditingLibrary.get_num_material_expressions(Material):
        Expression = unreal.MaterialEditingLibrary.create_material_expression(Material, unreal.MaterialExpressionConstant3Vector)
        Expression.set_editor_property("constant", unreal.LinearColor(*Color, 1.0))
        unreal.MaterialEditingLibrary.connect_material_property(Expression, "", unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(Material)
    unreal.EditorAssetLibrary.save_loaded_asset(Material)
    return Material


def AddMapping(Context, Action, KeyName, Modifiers=None):
    Key = unreal.Key()
    Key.set_editor_property("key_name", KeyName)
    Mapping = unreal.EnhancedActionKeyMapping()
    Mapping.set_editor_property("action", Action)
    Mapping.set_editor_property("key", Key)
    if Modifiers:
        Mapping.set_editor_property("modifiers", Modifiers)
    MappingData = Context.get_editor_property("default_key_mappings")
    Mappings = MappingData.get_editor_property("mappings")
    Mappings.append(Mapping)
    MappingData.set_editor_property("mappings", Mappings)
    Context.set_editor_property("default_key_mappings", MappingData)


def Swizzle():
    Modifier = unreal.InputModifierSwizzleAxis(outer=Context)
    Modifier.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
    return Modifier


def Negate():
    return unreal.InputModifierNegate(outer=Context)


ActionDefinitions = {
    "IA_GrabLeft": unreal.InputActionValueType.AXIS1D,
    "IA_GrabRight": unreal.InputActionValueType.AXIS1D,
    "IA_UseLeft": unreal.InputActionValueType.AXIS1D,
    "IA_UseRight": unreal.InputActionValueType.AXIS1D,
    "IA_Teleport": unreal.InputActionValueType.AXIS2D,
    "IA_SnapTurn": unreal.InputActionValueType.AXIS1D,
    "IA_Cancel": unreal.InputActionValueType.BOOLEAN,
    "IA_DesktopMove": unreal.InputActionValueType.AXIS2D,
    "IA_DesktopLook": unreal.InputActionValueType.AXIS2D,
    "IA_TeleportHold": unreal.InputActionValueType.BOOLEAN,
}
Actions = {}
for Name, ValueType in ActionDefinitions.items():
    Action = CreateDataAsset(Name, unreal.InputAction)
    Action.set_editor_property("value_type", ValueType)
    Actions[Name] = Action
    unreal.EditorAssetLibrary.save_loaded_asset(Action)

Context = CreateDataAsset("IMC_Interaction", unreal.InputMappingContext)
Context.unmap_all()
for Prefix in ["OculusTouch", "PICO", "ValveIndex"]:
    AddMapping(Context, Actions["IA_GrabLeft"], Prefix + "_Left_Grip_Axis")
    AddMapping(Context, Actions["IA_GrabRight"], Prefix + "_Right_Grip_Axis")
    AddMapping(Context, Actions["IA_UseLeft"], Prefix + "_Left_Trigger_Axis")
    AddMapping(Context, Actions["IA_UseRight"], Prefix + "_Right_Trigger_Axis")
    AddMapping(Context, Actions["IA_Teleport"], Prefix + "_Left_Thumbstick_2D")
    AddMapping(Context, Actions["IA_SnapTurn"], Prefix + "_Right_Thumbstick_X")
    AddMapping(Context, Actions["IA_Cancel"], Prefix + "_Right_B_Click")

for Side, Suffix in [("Left", "Left"), ("Right", "Right")]:
    AddMapping(Context, Actions["IA_Grab" + Suffix], "Vive_" + Side + "_Grip_Click")
    AddMapping(Context, Actions["IA_Use" + Suffix], "Vive_" + Side + "_Trigger_Axis")
AddMapping(Context, Actions["IA_TeleportHold"], "Vive_Left_Trackpad_Click")
AddMapping(Context, Actions["IA_SnapTurn"], "Vive_Right_Trackpad_X")
AddMapping(Context, Actions["IA_Cancel"], "Vive_Right_Menu_Click")
AddMapping(Context, Actions["IA_GrabRight"], "E")
AddMapping(Context, Actions["IA_GrabLeft"], "Q")
AddMapping(Context, Actions["IA_UseRight"], "LeftMouseButton")
AddMapping(Context, Actions["IA_UseLeft"], "F")
AddMapping(Context, Actions["IA_Cancel"], "BackSpace")
AddMapping(Context, Actions["IA_DesktopMove"], "W", [Swizzle()])
AddMapping(Context, Actions["IA_DesktopMove"], "S", [Negate(), Swizzle()])
AddMapping(Context, Actions["IA_DesktopMove"], "D")
AddMapping(Context, Actions["IA_DesktopMove"], "A", [Negate()])
AddMapping(Context, Actions["IA_DesktopLook"], "Mouse2D")
unreal.EditorAssetLibrary.save_loaded_asset(Context)

UIAssetNames = ["WBP_MainMenu", "WBP_MapEntry"]
if not all(unreal.EditorAssetLibrary.does_asset_exist(Root + "/UI/" + Name) for Name in UIAssetNames):
    UIScriptPath = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir() + "Scripts/CreatePrototypeUI.py")
    with open(UIScriptPath, encoding="utf-8-sig") as UIScriptFile:
        exec(compile(UIScriptFile.read(), UIScriptPath, "exec"), {"__name__": "__main__"})

FloorMaterial = CreateMaterial("M_Floor", (0.12, 0.15, 0.19))
WallMaterial = CreateMaterial("M_Wall", (0.42, 0.48, 0.55))
TableMaterial = CreateMaterial("M_Table", (0.20, 0.25, 0.30))
FishingMaterial = CreateMaterial("M_FishingItem", (0.08, 0.40, 0.66))
ClinicMaterial = CreateMaterial("M_ClinicItem", (0.78, 0.66, 0.16))
PlacementMaterial = CreateMaterial("M_Placement", (0.05, 0.65, 0.45))


def Spawn(ActorClass, Name, Position, Rotation=None):
    if Rotation is None:
        Rotation = unreal.Rotator()
    Actor = ActorSubsystem.spawn_actor_from_class(GetConfiguredClass(ActorClass,Name), unreal.Vector(*Position), Rotation)
    Actor.set_actor_label(Name)
    return Actor


def Geometry(Name, Position, Scale, Material, Shape="Cube"):
    Actor = Spawn(unreal.StaticMeshActor, Name, Position)
    Body = Actor.static_mesh_component
    Body.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/" + Shape))
    Body.set_material(0, Material)
    Actor.set_actor_scale3d(unreal.Vector(*Scale))
    return Actor


MapDefinitions = [
    ("L_Fishing", "Рыболовный траулер", FishingMaterial),
    ("L_Clinic", "Кабинет терапевта", ClinicMaterial),
    ("L_MainMenu", "Выбор карты", TableMaterial),
]
for MapName, Title, ItemMaterial in MapDefinitions:
    MapPath = Root + "/Maps/" + MapName
    if unreal.EditorAssetLibrary.does_asset_exist(MapPath):
        unreal.log("Keeping existing map: " + MapPath)
        continue
    if not LevelSubsystem.new_level(MapPath):
        raise RuntimeError("Map creation failed: " + MapPath)
    World = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    World.get_world_settings().set_editor_property("default_game_mode", GetBlueprintClass('PrototypeGameMode'))
    Geometry("Floor", (80, 0, -10), (10, 10, 0.2), FloorMaterial)
    Geometry("BackWall", (450, 0, 140), (0.15, 10, 3), WallMaterial)
    Geometry("LeftWall", (80, -490, 140), (10, 0.15, 3), WallMaterial)
    Geometry("RightWall", (80, 490, 140), (10, 0.15, 3), WallMaterial)
    Spawn(unreal.PlayerStart, "PlayerStart", (0, 0, 0))
    Sun = Spawn(unreal.DirectionalLight, "Daylight", (0, 0, 400), unreal.Rotator(pitch=-55, yaw=-35))
    Sun.light_component.set_editor_property("intensity", 5)
    Spawn(unreal.SkyAtmosphere, "SkyAtmosphere", (0, 0, 0))
    Sky = Spawn(unreal.SkyLight, "AmbientLight", (0, 0, 300))
    Sky.light_component.set_editor_property("intensity", 1.4)
    Fill = Spawn(unreal.PointLight, "WorkLight", (30, -80, 260))
    Fill.light_component.set_editor_property("intensity", 5000)
    Fill.light_component.set_editor_property("attenuation_radius", 900)
    if MapName != "L_MainMenu":
        Geometry("WorkTable", (110, 0, 90), (1.55, 1.5, 0.12), TableMaterial)
        Geometry("TableLegLeft", (110, -55, 43), (0.12, 0.12, 0.86), TableMaterial)
        Geometry("TableLegRight", (110, 55, 43), (0.12, 0.12, 0.86), TableMaterial)
        Director = Spawn(unreal.ScenarioDirector, "ScenarioDirector", (0, 0, 0))
        Director.set_editor_property("scenario_title", unreal.Text(Title))
        Station = Spawn(unreal.PlacementStation, "PlacementStation", (145, 20, 98))
        Station.surface.set_material(0, PlacementMaterial)
        Station.acceptance_zone.set_editor_property("accepted_item_type", "PracticeItem")
        for ItemIndex, Side in enumerate([-20, 20]):
            Item = Spawn(unreal.WorkItem, "PracticeItem" + str(ItemIndex + 1), (70, Side, 106))
            Item.body.set_material(0, ItemMaterial)
            Item.set_editor_property("display_name", unreal.Text("Учебный предмет " + str(ItemIndex + 1)))
        Geometry("EquipmentShelf", (360, 0, 110), (0.55, 3.8, 0.12), TableMaterial)
    if not LevelSubsystem.save_current_level():
        raise RuntimeError("Map save failed: " + MapPath)
    unreal.log("Created map: " + MapPath)

LevelSubsystem.load_level(Root + "/Maps/L_MainMenu")
unreal.log("Prototype assets are ready")

