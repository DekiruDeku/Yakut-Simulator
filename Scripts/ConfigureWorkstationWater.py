import json
import pathlib
import shutil
import unreal
from editor_toolset.toolsets.material_instance import MaterialInstanceTools


def ConfigureWorkstationWater():
    ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
    World = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    MapPath = World.get_path_name().split('.')[0]
    Source = (ProjectDirectory / 'Content' / MapPath.removeprefix('/Game/')).with_suffix('.umap')
    Backup = ProjectDirectory / 'Saved/Backups/WaterSurface' / Source.relative_to(ProjectDirectory)
    if Source.exists() and not Backup.exists():
        Backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(Source, Backup)
    ActorSubsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    Actors = ActorSubsystem.get_all_level_actors()
    Ocean = next(Actor for Actor in Actors if isinstance(Actor, unreal.WaterBodyOcean))
    Component = Ocean.get_water_body_component()
    Zone = Component.get_editor_property('water_zone_override')
    if not Zone:
        Zone = Component.get_editor_property('owning_water_zone')
    MaterialPath = '/Game/Minigames/Fishing/Workstations/Water/MI_WorkstationOcean'
    Material = unreal.load_asset(MaterialPath)
    if not Material:
        Material = MaterialInstanceTools.create('/Game/Minigames/Fishing/Workstations/Water', 'MI_WorkstationOcean', unreal.load_asset('/Water/Materials/WaterSurface/Water_Material_Ocean'))
    MaterialInstanceTools.set_static_switch_parameter(Material, 'Enable Waves', False)
    MaterialInstanceTools.set_scalar_parameter(Material, 'Fluid Displacement Multiplier', 0)
    unreal.EditorAssetLibrary.save_loaded_asset(Material, False)
    Ocean.set_editor_property('WaterWaves', None)
    Component.set_water_material(Material)
    FarMaterial = unreal.load_asset('/Water/Materials/WaterSurface/Water_FarMesh')
    Zone.water_mesh.set_editor_property('FarDistanceMaterial', FarMaterial)
    RemovedZones = []
    for Candidate in Actors:
        if isinstance(Candidate, unreal.WaterZone) and Candidate != Zone:
            AssignedBodies = [Body for Body in Actors if isinstance(Body, unreal.WaterBody) and (Body.get_water_body_component().get_editor_property('water_zone_override') == Candidate or Body.get_water_body_component().get_editor_property('owning_water_zone') == Candidate)]
            if not AssignedBodies:
                RemovedZones.append(Candidate.get_path_name())
                ActorSubsystem.destroy_actor(Candidate)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    ReportDirectory = ProjectDirectory / 'Saved/Verification/WaterSurface'
    ReportDirectory.mkdir(parents=True, exist_ok=True)
    Report = {'Map': MapPath, 'Ocean': Ocean.get_path_name(), 'Zone': Zone.get_path_name(), 'RemovedUnusedZones': RemovedZones, 'WaterMaterial': Material.get_path_name(), 'FarMaterial': FarMaterial.get_path_name(), 'GeometricWaves': unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(Material, 'Enable Waves'), 'FarFixedHeight': unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(FarMaterial, 'UseFixedZ'), 'FarWaves': unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(FarMaterial, 'Enable Waves'), 'Runtime': 'PIE and Automation were not run'}
    (ReportDirectory / 'Verification.json').write_text(json.dumps(Report, indent=2), encoding='utf-8')
    print(json.dumps(Report))


if __name__ == '__main__':
    ConfigureWorkstationWater()
