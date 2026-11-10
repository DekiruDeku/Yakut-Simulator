import pathlib
import shutil
import unreal

ProjectDirectory=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
WaveAssetPath='/Game/Fishing/Water/Waves_CalmOcean'
MaterialPath='/Game/Fishing/Materials/MI_CalmOcean'


def ConfigureFishingOcean(SaveMap=True):
    Actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    Levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    World=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    if World.get_path_name()!='/Game/Prototype/Maps/L_Fishing.L_Fishing':
        raise RuntimeError('The fishing ocean must be configured in L_Fishing')
    BackupDirectory=ProjectDirectory/'Saved/Backups/WaterBodyOcean'
    BackupDirectory.mkdir(parents=True,exist_ok=True)
    if not (BackupDirectory/'L_Fishing.umap').exists():
        shutil.copy2(ProjectDirectory/'Content/Prototype/Maps/L_Fishing.umap',BackupDirectory/'L_Fishing.umap')

    Oceans=list(unreal.GameplayStatics.get_all_actors_of_class(World,unreal.WaterBodyOcean))
    if len(Oceans)>1:
        raise RuntimeError('Multiple ocean water bodies require an explicit selection')
    Created=not Oceans
    if Oceans:
        Ocean,=Oceans
    else:
        Ocean=Actors.spawn_actor_from_class(unreal.WaterBodyOcean,unreal.Vector(0,0,-100))
        Ocean.set_actor_label('WaterBodyOcean')
    Component=Ocean.get_water_body_component()

    Material=unreal.load_asset(MaterialPath)
    if not Material:
        Material=unreal.AssetToolsHelpers.get_asset_tools().create_asset('MI_CalmOcean','/Game/Fishing/Materials',unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    if Material.get_editor_property('parent')!=unreal.load_asset('/Water/Materials/WaterSurface/Water_Material_Ocean'):
        unreal.MaterialEditingLibrary.set_material_instance_parent(Material,unreal.load_asset('/Water/Materials/WaterSurface/Water_Material_Ocean'))
        unreal.EditorAssetLibrary.save_loaded_asset(Material,False)

    Waves=unreal.load_asset(WaveAssetPath)
    if not Waves:
        Waves=unreal.EditorAssetLibrary.duplicate_asset('/Water/Waves/GerstnerWaves_Ocean',WaveAssetPath)
        Source=Waves.get_editor_property('WaterWaves')
        Generator=Source.get_editor_property('GerstnerWaveGenerator')
        for Property,Value in {'NumWaves':12,'MinWavelength':250.0,'MaxWavelength':2000.0,'MinAmplitude':0.5,'MaxAmplitude':3.0,'WindAngleDeg':35.0,'DirectionAngularSpreadDeg':70.0}.items():
            Generator.set_editor_property(Property,Value)
        Source.set_editor_property('GerstnerWaveGenerator',None)
        Source.set_editor_property('GerstnerWaveGenerator',Generator)
        unreal.EditorAssetLibrary.save_loaded_asset(Waves,False)

    Zones=list(unreal.GameplayStatics.get_all_actors_of_class(World,unreal.WaterZone))
    if len(Zones)>1:
        raise RuntimeError('Multiple water zones require an explicit selection')
    if Zones:
        Zone,=Zones
    else:
        Zone=Actors.spawn_actor_from_class(unreal.WaterZone,unreal.Vector(0,0,Ocean.get_actor_location().z))
        Zone.set_actor_label('WaterZone')
    if Created:
        Zone.set_editor_property('ZoneExtent',unreal.Vector2D(300000,300000))
        Zone.water_mesh.set_editor_property('FarDistanceMaterial',unreal.load_asset('/Water/Materials/WaterSurface/LODs/Water_Material_Ocean_LOD'))
        Zone.water_mesh.set_editor_property('FarDistanceMeshExtent',2000000.0)
        Component.set_editor_property('bAffectsLandscape',False)
        Component.set_editor_property('OceanExtents',unreal.Vector2D(300000,300000))
        Component.set_editor_property('CollisionExtents',unreal.Vector(150000,150000,10000))
        Component.set_collision_profile_name('WaterBodyCollision')
        Component.set_water_material(Material)
        Ocean.water_waves.set_editor_property('WaterWavesAsset',Waves)
        Spline=Ocean.get_water_spline()
        Spline.set_spline_points([unreal.Vector(119500,119500,0),unreal.Vector(120500,119500,0),unreal.Vector(120500,120500,0),unreal.Vector(119500,120500,0)],unreal.SplineCoordinateSpace.LOCAL,False)
        Spline.set_closed_loop(True,False)
        for PointIndex in range(Spline.get_number_of_spline_points()):
            Spline.set_spline_point_type(PointIndex,unreal.SplinePointType.LINEAR,False)
        Spline.update_spline()
        Spline.k2_synchronize_and_broadcast_data_change()
    Component.set_water_zone_override(Zone)
    for Actor in list(Actors.get_all_level_actors()):
        if isinstance(Actor,unreal.StaticMeshActor) and Actor.get_actor_label()=='WaterPluginOcean':
            Actors.destroy_actor(Actor)
    for Scenario in unreal.GameplayStatics.get_all_actors_of_class(World,unreal.FishingScenario):
        Scenario.sonar.set_editor_property('WaterHeight',Ocean.get_actor_location().z)
        Scenario.trawl.set_editor_property('WaterHeight',Ocean.get_actor_location().z)
    if SaveMap and not Levels.save_current_level():
        raise RuntimeError('The ocean water body map could not be saved')
    print('Fishing ocean configured:',Ocean.get_class().get_path_name(),Zone.get_class().get_path_name())
    return Ocean


if __name__=='__main__':
    ConfigureFishingOcean()
