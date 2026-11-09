import unreal


ActorSubsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LevelSubsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
AssetTools = unreal.AssetToolsHelpers.get_asset_tools()
Root = "/Game/Prototype"


def Material(Name, Color, Roughness=0.75, Metallic=0.0):
    Path = Root + "/Materials/" + Name
    if unreal.EditorAssetLibrary.does_asset_exist(Path):
        return unreal.load_asset(Path)
    Result = AssetTools.create_asset(Name, Root + "/Materials", unreal.Material, unreal.MaterialFactoryNew())
    ColorExpression = unreal.MaterialEditingLibrary.create_material_expression(Result, unreal.MaterialExpressionConstant3Vector)
    ColorExpression.set_editor_property("constant", unreal.LinearColor(*Color, 1))
    unreal.MaterialEditingLibrary.connect_material_property(ColorExpression, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for Value, Property in [(Roughness, unreal.MaterialProperty.MP_ROUGHNESS), (Metallic, unreal.MaterialProperty.MP_METALLIC)]:
        Expression = unreal.MaterialEditingLibrary.create_material_expression(Result, unreal.MaterialExpressionConstant)
        Expression.set_editor_property("r", Value)
        unreal.MaterialEditingLibrary.connect_material_property(Expression, "", Property)
    unreal.MaterialEditingLibrary.recompile_material(Result)
    unreal.EditorAssetLibrary.save_loaded_asset(Result)
    return Result


def Spawn(ActorClass, Name, Position, Rotation=None):
    if Rotation is None:
        Rotation = unreal.Rotator()
    Actor = ActorSubsystem.spawn_actor_from_class(ActorClass, unreal.Vector(*Position), Rotation)
    Actor.set_actor_label(Name)
    return Actor


def Shape(Name, Position, Scale, Surface, Geometry="Cube", Rotation=None):
    Actor = Spawn(unreal.StaticMeshActor, Name, Position, Rotation)
    Actor.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/" + Geometry))
    Actor.static_mesh_component.set_material(0, Surface)
    Actor.set_actor_scale3d(unreal.Vector(*Scale))
    return Actor


Deck = Material("M_Deck", (0.10, 0.16, 0.18), 0.90)
Seam = Material("M_Seam", (0.025, 0.04, 0.045))
Ivory = Material("M_Ivory", (0.57, 0.59, 0.56))
Teal = Material("M_Teal", (0.025, 0.22, 0.24))
Steel = Material("M_Steel", (0.29, 0.34, 0.38), 0.45, 0.4)
DarkSteel = Material("M_DarkSteel", (0.065, 0.085, 0.10), 0.6, 0.25)
Glass = Material("M_Window", (0.045, 0.12, 0.17), 0.25, 0.2)
Ocean = Material("M_Ocean", (0.025, 0.12, 0.19), 0.45)
Orange = Material("M_Orange", (0.72, 0.22, 0.045))
Rubber = Material("M_Rubber", (0.025, 0.035, 0.045), 0.95)
White = Material("M_ClinicWhite", (0.60, 0.64, 0.65))
Floor = Material("M_ClinicFloor", (0.31, 0.36, 0.39), 0.85)
Wood = Material("M_Wood", (0.36, 0.24, 0.14), 0.85)
Fabric = Material("M_Fabric", (0.045, 0.29, 0.31), 0.95)


def Prepare(MapName):
    if not LevelSubsystem.load_level(Root + "/Maps/" + MapName):
        raise RuntimeError("Map could not be loaded: " + MapName)
    for Actor in ActorSubsystem.get_all_level_actors():
        ActorSubsystem.destroy_actor(Actor)
    World = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    World.get_world_settings().set_editor_property("default_game_mode", unreal.PrototypeGameMode)
    Spawn(unreal.PlayerStart, "PlayerStart", (0, 0, 0))
    Exposure = Spawn(unreal.PostProcessVolume, "SceneExposure", (0, 0, 0))
    Exposure.set_editor_property("unbound", True)
    Settings = Exposure.get_editor_property("settings")
    for Property, Value in {
        "override_auto_exposure_method": True,
        "auto_exposure_method": unreal.AutoExposureMethod.AEM_MANUAL,
        "override_auto_exposure_apply_physical_camera_exposure": True,
        "auto_exposure_apply_physical_camera_exposure": False,
        "override_auto_exposure_bias": True,
        "auto_exposure_bias": 0.0,
        "override_bloom_intensity": True,
        "bloom_intensity": 0.0,
        "override_motion_blur_amount": True,
        "motion_blur_amount": 0.0,
    }.items():
        Settings.set_editor_property(Property, Value)
    Exposure.set_editor_property("settings", Settings)
    Sun = Spawn(unreal.DirectionalLight, "Sunlight", (0, 0, 600), unreal.Rotator(pitch=-48, yaw=-35))
    Sun.light_component.set_editor_property("intensity", 4.0)
    Sun.light_component.set_editor_property("atmosphere_sun_light", True)
    Sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    Spawn(unreal.SkyAtmosphere, "Sky", (0, 0, 0))
    Sky = Spawn(unreal.SkyLight, "AmbientLight", (0, 0, 500))
    Sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    Sky.light_component.set_editor_property("intensity", 0.6)
    Sky.light_component.set_editor_property("real_time_capture", True)


def WorkTable(Position):
    CenterX, CenterY, Height = Position
    Shape("Worktop", (CenterX, CenterY, Height), (1.6, 1.2, 0.06), Steel)
    for OffsetX in [-65, 65]:
        for OffsetY in [-45, 45]:
            Shape("TableLeg", (CenterX + OffsetX, CenterY + OffsetY, Height / 2), (0.06, 0.06, Height / 100), DarkSteel)
    Shape("TableShelf", (CenterX, CenterY, 26), (1.4, 1.0, 0.04), DarkSteel)


def Items():
    ItemClass = unreal.load_class(None, Root + "/Blueprints/BP_WorkItem.BP_WorkItem_C")
    for ItemIndex, Surface in enumerate([Orange, Teal]):
        Item = Spawn(ItemClass, "InteractiveItem" + str(ItemIndex + 1), (70, -20 + ItemIndex * 40, 106))
        Item.body.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cylinder"))
        Item.set_actor_scale3d(unreal.Vector(0.07, 0.07, 0.18))
        Item.body.set_material(0, Surface)
        Item.set_editor_property("display_name", unreal.Text("Переносной фонарь"))


Prepare("L_Fishing")
Shape("Ocean", (300, 0, -95), (400, 400, 0.1), Ocean)
Shape("Hull", (350, 0, -45), (14, 8, 0.9), DarkSteel)
Shape("Deck", (350, 0, -5), (14, 8, 0.1), Deck)
for SeamIndex in range(16):
    Shape("DeckSeam", (-320 + SeamIndex * 85, 0, 0.25), (0.012, 7.8, 0.005), Seam)
for Side in [-1, 1]:
    Shape("Bulwark", (350, Side * 395, 25), (14, 0.1, 0.5), Ivory)
    Shape("UpperRail", (350, Side * 395, 100), (14, 0.045, 0.045), Steel)
    Shape("MiddleRail", (350, Side * 395, 70), (14, 0.035, 0.035), Steel)
    for PostIndex in range(12):
        Shape("RailPost", (-300 + PostIndex * 120, Side * 395, 65), (0.04, 0.04, 0.7), Steel)
Shape("CabinFrontLower", (475, 0, 65), (0.12, 4.6, 1.3), Ivory)
Shape("CabinFrontUpper", (475, 0, 250), (0.12, 4.6, 0.3), Ivory)
for WindowY in [-155, 0, 155]:
    Shape("CabinWindow", (474, WindowY, 182), (0.10, 1.3, 1.0), Glass)
for FrameY in [-230, -78, 78, 230]:
    Shape("WindowPillar", (465, FrameY, 182), (0.16, 0.10, 1.15), Ivory)
for Side in [-1, 1]:
    Shape("CabinSide", (700, Side * 230, 130), (4.5, 0.12, 2.6), Ivory)
    Shape("CabinStripe", (700, Side * 237, 80), (4.5, 0.015, 0.32), Teal)
Shape("CabinRoof", (700, 0, 270), (4.8, 4.9, 0.15), Ivory)
Shape("CabinFrontStripe", (467, 0, 80), (0.015, 4.6, 0.32), Teal)
Shape("Mast", (710, 0, 395), (0.12, 0.12, 2.4), Steel, "Cylinder")
Shape("MastCrossbar", (710, 0, 475), (0.06, 2.8, 0.06), Steel)
Shape("WinchFoundation", (280, -245, 24), (1.65, 1.75, 0.48), Teal)
Shape("WinchDrum", (280, -245, 92), (0.9, 0.9, 1.3), Rubber, "Cylinder", unreal.Rotator(roll=90))
for OffsetY in [-70, 70]:
    Shape("WinchFlange", (280, -245 + OffsetY, 92), (1.45, 1.45, 0.06), Steel, "Cylinder", unreal.Rotator(roll=90))
for SupportY in [-325, -165]:
    Shape("WinchSupport", (280, SupportY, 60), (0.30, 0.14, 1.1), DarkSteel)
for CrateIndex in range(3):
    Shape("FishCrate", (260 + CrateIndex * 80, 240, 30), (0.65, 0.9, 0.6), Teal)
    Shape("CrateRim", (260 + CrateIndex * 80, 240, 62), (0.70, 0.95, 0.06), DarkSteel)
Shape("CrateStack", (340, 240, 94), (0.65, 0.9, 0.6), Teal)
WorkTable((110, 0, 90))
Items()
LevelSubsystem.save_current_level()

Prepare("L_Clinic")
Shape("Floor", (300, 0, -8), (12, 9, 0.16), Floor)
Shape("BackWall", (890, 0, 155), (0.15, 9, 3.1), White)
for Side in [-1, 1]:
    Shape("SideWall", (300, Side * 445, 155), (12, 0.15, 3.1), White)
    Shape("WallBase", (300, Side * 435, 8), (12, 0.035, 0.16), DarkSteel)
    Shape("WallTrim", (300, Side * 435, 110), (12, 0.035, 0.06), Teal)
Shape("BackWallBase", (880, 0, 8), (0.035, 9, 0.16), DarkSteel)
Shape("BackWallTrim", (880, 0, 110), (0.035, 9, 0.06), Teal)
for TileIndex in range(13):
    Shape("FloorJoint", (-300 + TileIndex * 100, 0, 0.25), (0.008, 9, 0.005), Seam)
for TileIndex in range(10):
    Shape("FloorJoint", (300, -450 + TileIndex * 100, 0.25), (12, 0.008, 0.005), Seam)
Shape("WindowFrame", (250, -432, 205), (3.5, 0.10, 1.6), Ivory)
Shape("WindowGlass", (250, -424, 205), (3.2, 0.04, 1.35), Glass)
Shape("WindowDivider", (250, -420, 205), (0.06, 0.06, 1.35), Ivory)
Shape("WindowSill", (250, -412, 133), (3.7, 0.30, 0.06), Ivory)
Shape("ExamCouchBase", (500, 280, 40), (2.2, 0.8, 0.65), Ivory)
Shape("ExamCouch", (500, 280, 80), (2.3, 0.88, 0.20), Fabric)
Shape("ExamHeadrest", (610, 280, 98), (0.55, 0.88, 0.16), Fabric, "Cube", unreal.Rotator(roll=18))
for CabinetY in [-260, -130]:
    Shape("StorageCabinet", (835, CabinetY, 110), (0.8, 1.1, 2.2), Ivory)
    Shape("CabinetDoor", (791, CabinetY, 112), (0.04, 1.02, 2.08), White)
    Shape("CabinetHandle", (785, CabinetY + 35, 115), (0.045, 0.045, 0.20), Steel)
Shape("Desk", (480, -100, 76), (1.8, 0.85, 0.07), Wood)
for DeskY in [-132, -68]:
    Shape("DeskSupport", (480, DeskY, 38), (1.55, 0.05, 0.76), DarkSteel)
Shape("MonitorFoot", (510, -100, 84), (0.35, 0.28, 0.04), DarkSteel)
Shape("MonitorStand", (510, -100, 98), (0.05, 0.05, 0.28), DarkSteel)
Shape("Monitor", (510, -100, 119), (0.05, 0.65, 0.43), DarkSteel)
Shape("MonitorScreen", (506, -100, 119), (0.01, 0.58, 0.36), Glass)
Shape("ChairSeat", (395, -100, 46), (0.48, 0.48, 0.12), Fabric)
Shape("ChairBack", (365, -100, 75), (0.10, 0.48, 0.65), Fabric)
Shape("ChairStem", (395, -100, 23), (0.055, 0.055, 0.46), Steel, "Cylinder")
Shape("ChairBase", (395, -100, 5), (0.55, 0.55, 0.04), DarkSteel, "Cylinder")
WorkTable((110, 0, 90))
Items()
for LightPosition in [(180, -80, 290), (600, 100, 290)]:
    Shape("CeilingLight", LightPosition, (1.4, 0.7, 0.04), Ivory)
    Lamp = Spawn(unreal.RectLight, "RoomLight", LightPosition, unreal.Rotator(pitch=-90))
    Lamp.light_component.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
    Lamp.light_component.set_editor_property("intensity", 180)
    Lamp.light_component.set_editor_property("source_width", 140)
    Lamp.light_component.set_editor_property("source_height", 70)
    Lamp.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
LevelSubsystem.save_current_level()

Prepare("L_MainMenu")
Shape("Floor", (200, 0, -10), (16, 16, 0.2), Deck)
LevelSubsystem.save_current_level()
LevelSubsystem.load_level(Root + "/Maps/L_Fishing")
unreal.EditorLevelLibrary.editor_set_game_view(True)
unreal.log("Maps rebuilt without training actors and with controlled exposure")
