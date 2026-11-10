import json
import pathlib
import shutil
import unreal

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SourceDirectory = ProjectDirectory / 'SourceAssets/Fishing/WinchConsole'
BackupDirectory = ProjectDirectory / 'Saved/Backups/WinchConsole'
RootPath = '/Game/Minigames/Fishing/Workstations/WinchConsole'
Metadata = json.loads((SourceDirectory / 'WinchConsole.json').read_text(encoding='utf-8'))
AssetTools = unreal.AssetToolsHelpers.get_asset_tools()
MeshEditor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)


def Backup(Asset):
    Package = Asset.get_path_name().split('.')[0]
    Relative = pathlib.Path('Content') / Package.removeprefix('/Game/')
    for Extension in ['.uasset', '.umap']:
        Source = (ProjectDirectory / Relative).with_suffix(Extension)
        Destination = (BackupDirectory / Relative).with_suffix(Extension)
        if Source.exists() and not Destination.exists():
            Destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(Source, Destination)


def ImportWinchConsole():
    Textures = {}
    for Name in ['T_WinchPanel', 'T_WinchDial', 'T_WinchReadout']:
        Task = unreal.AssetImportTask()
        Task.set_editor_property('filename', str(SourceDirectory / (Name + '.png')))
        Task.set_editor_property('destination_path', RootPath + '/Textures')
        Task.set_editor_property('automated', True)
        Task.set_editor_property('save', True)
        Task.set_editor_property('replace_existing', True)
        Existing = unreal.load_asset(RootPath + '/Textures/' + Name)
        if Existing:
            Backup(Existing)
        AssetTools.import_asset_tasks([Task])
        Textures[Name] = unreal.load_asset(RootPath + '/Textures/' + Name)
    Materials = {}
    for Name, Color in Metadata['palette'].items():
        Material = unreal.load_asset(RootPath + '/Materials/M_Winch' + Name)
        if not Material or Name == 'Screen':
            if Material:
                Backup(Material)
                unreal.MaterialEditingLibrary.delete_all_material_expressions(Material)
            else:
                Material = AssetTools.create_asset('M_Winch' + Name, RootPath + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
            Base = unreal.MaterialEditingLibrary.create_material_expression(Material, unreal.MaterialExpressionConstant3Vector)
            Base.set_editor_property('constant', unreal.LinearColor(*Color))
            if Name in ['PanelPrint', 'DialPrint', 'Screen']:
                Base = unreal.MaterialEditingLibrary.create_material_expression(Material, unreal.MaterialExpressionTextureSample)
                TextureName = {'PanelPrint': 'T_WinchPanel', 'DialPrint': 'T_WinchDial', 'Screen': 'T_WinchReadout'}[Name]
                Base.set_editor_property('texture', Textures[TextureName])
            unreal.MaterialEditingLibrary.connect_material_property(Base, '', unreal.MaterialProperty.MP_BASE_COLOR)
            if Name == 'Screen':
                unreal.MaterialEditingLibrary.connect_material_property(Base, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            Metallic = .15
            Roughness = .58
            if Name in ['Steel', 'DarkSteel']:
                Metallic = .8
                Roughness = .29
            if Name in ['Rubber', 'DialPrint']:
                Metallic = 0
                Roughness = .84
            for Value, Property in [(Metallic, unreal.MaterialProperty.MP_METALLIC), (Roughness, unreal.MaterialProperty.MP_ROUGHNESS)]:
                Constant = unreal.MaterialEditingLibrary.create_material_expression(Material, unreal.MaterialExpressionConstant)
                Constant.set_editor_property('r', Value)
                unreal.MaterialEditingLibrary.connect_material_property(Constant, '', Property)
            unreal.MaterialEditingLibrary.recompile_material(Material)
            unreal.EditorAssetLibrary.save_loaded_asset(Material, False)
        Materials[Name] = Material
    Tasks = []
    for Name in Metadata['assets']:
        Existing = unreal.load_asset(RootPath + '/Meshes/' + Name)
        if Existing:
            Backup(Existing)
        Task = unreal.AssetImportTask()
        for Property, Value in {'filename': str(SourceDirectory / (Name + '.obj')), 'destination_path': RootPath + '/Meshes', 'destination_name': Name, 'automated': True, 'save': True, 'replace_existing': True}.items():
            Task.set_editor_property(Property, Value)
        Options = unreal.FbxImportUI()
        Options.set_editor_property('import_materials', False)
        Options.set_editor_property('import_textures', False)
        Options.static_mesh_import_data.set_editor_property('combine_meshes', True)
        Options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
        Task.set_editor_property('options', Options)
        Tasks.append(Task)
    AssetTools.import_asset_tasks(Tasks)
    for Name, Information in Metadata['assets'].items():
        Mesh = unreal.load_asset(RootPath + '/Meshes/' + Name)
        if not Mesh:
            raise RuntimeError('Winch console mesh was not imported: ' + Name)
        for Index, Slot in enumerate(Mesh.get_editor_property('static_materials')):
            MaterialName = str(Slot.get_editor_property('material_slot_name'))
            Mesh.set_material(Index, Materials[MaterialName])
        MeshEditor.remove_collisions(Mesh)
        Setup = Mesh.get_editor_property('body_setup')
        Setup.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
        if Information['collision'] == 'cabinet':
            Geometry = Setup.get_editor_property('agg_geom')
            Boxes = []
            for Center, Size, Roll in [((0, 0, 41), (104, 55, 72), 0), ((0, 0, 84), (128, 80, 12), 0), ((0, 0, 102), (126, 82, 3), 17.744)]:
                Box = unreal.KBoxElem()
                Box.set_editor_property('center', unreal.Vector(*Center))
                Box.set_editor_property('x', Size[0])
                Box.set_editor_property('y', Size[1])
                Box.set_editor_property('z', Size[2])
                Box.set_editor_property('rotation', unreal.Rotator(roll=Roll))
                Boxes.append(Box)
            Geometry.set_editor_property('box_elems', Boxes)
            Setup.set_editor_property('agg_geom', Geometry)
        elif Information['collision'] == 'box':
            MeshEditor.add_simple_collisions(Mesh, unreal.ScriptCollisionShapeType.BOX)
        elif Information['collision'] == 'convex':
            MeshEditor.set_convex_decomposition_collisions(Mesh, 2, 24, 10000)
        unreal.EditorAssetLibrary.save_loaded_asset(Mesh, False)
    print('Winch console meshes, materials and collision saved')


if __name__ == '__main__':
    ImportWinchConsole()
