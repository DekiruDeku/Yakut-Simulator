import datetime
import importlib
import pathlib
import shutil
import sys
import unreal

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.insert(0,str(ProjectDirectory/'Scripts'))
import ImportFishingAssets
importlib.reload(ImportFishingAssets)
from ConfigureProjectBlueprints import AddVisual


def ConfigureTrawlerWheelhouse():
    BackupDirectory = ProjectDirectory/'Saved/Backups/TrawlerWheelhouse'/datetime.datetime.now().strftime('%Y%m%d_%H%M%S')
    for RelativePath in ['Content/Fishing/Blueprints/BP_TrawlerVessel.uasset','Content/Fishing/Meshes/SM_Wheelhouse.uasset','Content/Fishing/Meshes/SM_WheelhouseGlass.uasset','Content/Fishing/Materials/M_WheelhouseGlass.uasset']:
        Source = ProjectDirectory/RelativePath
        if Source.exists():
            Destination = BackupDirectory/RelativePath
            Destination.parent.mkdir(parents=True,exist_ok=True)
            shutil.copy2(Source,Destination)
    ImportFishingAssets.ImportFishingAssets(Reimport=True,AssetNames=['SM_Wheelhouse','SM_WheelhouseGlass'])
    Blueprint = unreal.load_asset('/Game/Fishing/Blueprints/BP_TrawlerVessel')
    Subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    Library = unreal.SubobjectDataBlueprintFunctionLibrary
    for Subobject in Subsystem.k2_gather_subobject_data_for_blueprint(Blueprint):
        Component = Library.get_associated_object(Library.get_data(Subobject))
        if isinstance(Component,unreal.StaticMeshComponent) and Component.get_name()=='WheelhouseVisual_GEN_VARIABLE':
            Component.set_collision_profile_name('BlockAllDynamic')
    AddVisual(Blueprint,'WheelhouseGlassVisual','SM_WheelhouseGlass',(0,0,0),'BlockAllDynamic')
    unreal.BlueprintEditorLibrary.compile_blueprint(Blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(Blueprint,False)
    World = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    Vessels = [Actor for Actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors() if Actor.get_class()==Blueprint.generated_class()]
    if Vessels:
        RelativeMapPath = 'Content/'+World.get_path_name().split('.')[0].removeprefix('/Game/')+'.umap'
        Destination = BackupDirectory/RelativeMapPath
        Destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(ProjectDirectory/RelativeMapPath,Destination)
        for Vessel in Vessels:
            for Component in Vessel.get_components_by_class(unreal.StaticMeshComponent):
                if Component.get_name() in ['WheelhouseVisual','WheelhouseGlassVisual']:
                    MeshName = 'SM_Wheelhouse'
                    if Component.get_name()=='WheelhouseGlassVisual':
                        MeshName = 'SM_WheelhouseGlass'
                    Component.set_static_mesh(unreal.load_asset('/Game/Fishing/Meshes/'+MeshName))
                    Component.set_collision_profile_name('BlockAllDynamic')
        unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    print('Trawler wheelhouse shell, glazing and Blueprint saved')


if __name__=='__main__':
    ConfigureTrawlerWheelhouse()
