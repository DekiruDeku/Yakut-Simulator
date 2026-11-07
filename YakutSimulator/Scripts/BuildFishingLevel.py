"""Run in Unreal Editor's Python console. Creates a new map and never overwrites one."""
import unreal

LEVEL = "/Game/Fishing/Maps/L_FishingPrototype"


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL):
        raise RuntimeError(f"{LEVEL} already exists. Open and edit it; this script never overwrites your work.")
    if not hasattr(unreal, "FishingScenario"):
        raise RuntimeError("Build YakutSimulatorEditor and restart the editor before creating the level.")
    font = unreal.load_asset("/Game/Fishing/UI/F_FishingCyrillic")
    text_material = unreal.load_asset("/Game/Fishing/UI/MI_FishingText")
    if font is None or text_material is None:
        raise RuntimeError("Run Fishing.PrepareTextAssets in the editor console before creating the map.")
    game_mode = unreal.load_class(None, "/Game/XRFramework/Blueprints/BP_XRGameMode.BP_XRGameMode_C")
    if game_mode is None:
        raise RuntimeError("The project's BP_XRGameMode is missing.")
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    # Save the user's current level before switching to the new map.
    if not levels.save_current_level():
        raise RuntimeError("Could not save the current level; map creation cancelled.")
    unreal.EditorAssetLibrary.make_directory("/Game/Fishing/Maps")
    if not levels.new_level(LEVEL):
        raise RuntimeError("Could not create the fishing map.")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)

    def spawn(cls, label, position, yaw=0):
        actor = actors.spawn_actor_from_class(cls, unreal.Vector(*position), unreal.Rotator(pitch=0, yaw=yaw, roll=0))
        if actor is None:
            raise RuntimeError(f"Could not spawn {label}")
        actor.set_actor_label(label)
        actor.set_folder_path("FishingPrototype")
        rotation = actor.get_actor_rotation()
        assert abs((rotation.yaw - yaw + 180) % 360 - 180) < .1, f"Wrong yaw: {label}"
        assert abs(rotation.pitch) < .1 and abs(rotation.roll) < .1, f"Tilted actor: {label}"
        return actor

    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    sphere = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    assert cube is not None and sphere is not None

    def solid(label, position, scale):
        actor = spawn(unreal.StaticMeshActor, label, position)
        mesh = actor.static_mesh_component
        mesh.set_static_mesh(cube)
        mesh.set_mobility(unreal.ComponentMobility.MOVABLE)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        mesh.set_collision_profile_name("BlockAll")
        return actor

    solid("UpperDeck / floor Z=300", (-700, 0, 285), (5, 4, .3))
    solid("LowerDeck / floor Z=0", (120, 0, -15), (7, 5, .3))
    solid("WinchConsole", (-700, 10, 380), (2.2, 1.1, .2))
    solid("Conveyor", (0, 0, 85), (.5, 2.4, .1))
    solid("ProcessingTable", (240, 10, 85), (1.5, 1.5, .1))
    scenario = spawn(unreal.FishingScenario, "FishingScenario", (0, 0, 0))
    scenario.set_editor_property("fish_count", 8)

    def station(label, kind, position, yaw=0, species=None):
        actor = spawn(unreal.FishingStation, label, position, yaw)
        actor.set_editor_property("scenario", scenario)
        actor.set_editor_property("kind", kind)
        if species is not None:
            actor.set_editor_property("species", species)
        # Refresh the component transform after editor properties were changed.
        actor.set_actor_transform(actor.get_actor_transform(), False, True)
        handle = actor.get_editor_property("handle")
        handle.set_visibility(kind == unreal.FishingStationKind.WINCH_LEVER)
        actor.get_editor_property("mesh").set_visibility(kind != unreal.FishingStationKind.STATUS_BOARD)
        label_component = actor.get_editor_property("label")
        label_component.set_font(font)
        label_component.set_text_material(text_material)
        label_component.set_text(unreal.Text(label))
        label_component.set_relative_rotation(unreal.Rotator(pitch=0, yaw=0 if kind == unreal.FishingStationKind.STATUS_BOARD else 180, roll=0), False, True)
        if kind == unreal.FishingStationKind.STATUS_BOARD:
            label_component.set_world_size(4)
        return actor

    K = unreal.FishingStationKind
    stations = [
        station("НАДЕТЬ ЗАЩИТУ", K.SAFETY_EQUIPMENT, (-780, -20, 410), 90),
        station("СТОП", K.EMERGENCY_STOP, (-740, -20, 410), 90),
        station("ПУСК / СБРОС", K.START_OR_RESUME, (-660, -20, 410), 90),
        station("ЛЕБЁДКА", K.WINCH_LEVER, (-700, 0, 410), 90),
        station("В ЦЕХ", K.GO_TO_SORTING, (-620, -20, 410), 90),
        station("ЛОТОК A", K.SORTING_BIN, (-35, -55, 95), 0, unreal.FishSpecies.A),
        station("ЛОТОК B", K.SORTING_BIN, (-35, 55, 95), 0, unreal.FishSpecies.B),
        station("К ОБРАБОТКЕ", K.GO_TO_PROCESSING, (-60, 65, 110)),
        station("СТОЛ ОБРАБОТКИ", K.PROCESSING_BOARD, (240, 0, 95), 90),
        station("ГОТОВЫЙ УЛОВ", K.DELIVERY_BIN, (240, 65, 95), 90),
        station("НОВАЯ СМЕНА", K.RESTART, (300, -5, 110), 90),
        station("ЗАДАНИЕ / ПАЛУБА", K.STATUS_BOARD, (-700, 90, 520), -90),
        station("ЗАДАНИЕ / СОРТИРОВКА", K.STATUS_BOARD, (90, 0, 190), 180),
        station("ЗАДАНИЕ / ОБРАБОТКА", K.STATUS_BOARD, (240, 95, 210), -90),
    ]
    start = spawn(unreal.PlayerStart, "PlayerStart_Deck", (-700, -65, 300), 90)
    assert start is not None
    net = spawn(unreal.StaticMeshActor, "NetPlaceholder / replace with net mesh", (-700, 150, 400))
    net.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    net.static_mesh_component.set_static_mesh(sphere)
    net.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    net.set_actor_scale3d(unreal.Vector(.7, .7, .7))
    net.attach_to_component(scenario.get_editor_property("net_position"), "", unreal.AttachmentRule.KEEP_WORLD,
                            unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    for label, position in [("DeckLight", (-700, -80, 620)), ("SortingLight", (-60, -80, 330)), ("ProcessingLight", (240, -80, 330))]:
        light = spawn(unreal.PointLight, label, position)
        component = light.get_component_by_class(unreal.PointLightComponent)
        component.set_mobility(unreal.ComponentMobility.MOVABLE)
        component.set_intensity(120)
        component.set_attenuation_radius(1200)
        component.set_cast_shadows(False)

    assert len(stations) == 14
    assert all(s.get_editor_property("scenario") == scenario for s in stations)
    assert sum(isinstance(a, unreal.FishingScenario) for a in actors.get_all_level_actors()) == 1
    assert scenario.get_editor_property("conveyor_path").get_spline_length() > 100
    assert levels.save_current_level(), "Could not save the prototype map"
    unreal.log(f"FISHING_LEVEL_READY: {LEVEL}; {len(stations)} stations; existing XR assets preserved")


build()
