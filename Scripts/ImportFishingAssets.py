import json
import pathlib
import unreal

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
SourceDirectory = ProjectDirectory / 'SourceAssets/Fishing/Models'
Metadata = json.loads((SourceDirectory / 'FishingAssets.json').read_text(encoding='utf-8'))
AssetTools = unreal.AssetToolsHelpers.get_asset_tools()
MeshEditor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
RootPath = '/Game/Minigames/Fishing/Trawling'


def CreatePaint(Name, Color):
    Path = RootPath + '/Materials/M_' + Name
    if unreal.EditorAssetLibrary.does_asset_exist(Path):
        return unreal.load_asset(Path)
    Result = AssetTools.create_asset('M_' + Name, RootPath + '/Materials', unreal.Material, unreal.MaterialFactoryNew())
    Base = unreal.MaterialEditingLibrary.create_material_expression(Result, unreal.MaterialExpressionConstant3Vector)
    Base.set_editor_property('constant', unreal.LinearColor(*Color))
    Tint = unreal.MaterialEditingLibrary.create_material_expression(Result, unreal.MaterialExpressionVectorParameter)
    Tint.set_editor_property('parameter_name', 'ProductTint')
    Tint.set_editor_property('default_value', unreal.LinearColor(1,1,1,1))
    Multiply = unreal.MaterialEditingLibrary.create_material_expression(Result, unreal.MaterialExpressionMultiply)
    unreal.MaterialEditingLibrary.connect_material_expressions(Base, '', Multiply, 'A')
    unreal.MaterialEditingLibrary.connect_material_expressions(Tint, '', Multiply, 'B')
    unreal.MaterialEditingLibrary.connect_material_property(Multiply, '', unreal.MaterialProperty.MP_BASE_COLOR)
    Roughness = .65
    Metallic = .15
    if Name in ['Steel', 'DarkSteel', 'FishSilver', 'FishBlue', 'FishBack']:
        Metallic = .75
        Roughness = .36
    if Name == 'WindowGlass':
        Metallic = .75
        Roughness = .12
    if Name == 'WheelhouseGlass':
        Metallic = 0
        Roughness = .08
        Result.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
        Result.set_editor_property('two_sided',True)
        Result.set_editor_property('translucency_lighting_mode',unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
        Opacity = unreal.MaterialEditingLibrary.create_material_expression(Result,unreal.MaterialExpressionConstant)
        Opacity.set_editor_property('r',.12)
        unreal.MaterialEditingLibrary.connect_material_property(Opacity,'',unreal.MaterialProperty.MP_OPACITY)
    if Name in ['Rubber','NetGreen','Rope','Stone']:
        Metallic = 0
        Roughness = .88
    for Value, Property in [(Roughness,unreal.MaterialProperty.MP_ROUGHNESS),(Metallic,unreal.MaterialProperty.MP_METALLIC)]:
        Constant = unreal.MaterialEditingLibrary.create_material_expression(Result,unreal.MaterialExpressionConstant)
        Constant.set_editor_property('r',Value)
        unreal.MaterialEditingLibrary.connect_material_property(Constant,'',Property)
    unreal.MaterialEditingLibrary.recompile_material(Result)
    if Name in ['FishSilver','FishBack','FishBlue','Eye']:
        unreal.MaterialEditingLibrary.set_material_usage(Result,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    unreal.EditorAssetLibrary.save_loaded_asset(Result)
    return Result


def ImportFishingAssets(Reimport=False,AssetNames=None):
    Assets = Metadata['assets']
    if AssetNames:
        Assets = {Name:Assets[Name] for Name in AssetNames}
    MaterialNames = {Name for Information in Assets.values() for Name in Information['materials']}
    Paints = {Name:CreatePaint(Name,Metadata['palette'][Name]) for Name in MaterialNames}
    Tasks=[]
    for Name in Assets:
        if unreal.EditorAssetLibrary.does_asset_exist(RootPath+'/Meshes/'+Name) and not Reimport:
            continue
        Task=unreal.AssetImportTask()
        Task.set_editor_property('filename',str(SourceDirectory/(Name+'.obj')))
        Task.set_editor_property('destination_path',RootPath+'/Meshes')
        Task.set_editor_property('destination_name',Name)
        Task.set_editor_property('automated',True)
        Task.set_editor_property('save',True)
        Task.set_editor_property('replace_existing',Reimport)
        Options=unreal.FbxImportUI()
        Options.set_editor_property('import_materials',False)
        Options.set_editor_property('import_textures',False)
        Options.static_mesh_import_data.set_editor_property('combine_meshes',True)
        Options.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
        Task.set_editor_property('options',Options)
        Tasks.append(Task)
    if Tasks:
        AssetTools.import_asset_tasks(Tasks)
    for Name,Information in Assets.items():
        Mesh=unreal.load_asset(RootPath+'/Meshes/'+Name)
        if not Mesh:
            raise RuntimeError('Missing imported mesh: '+Name)
        for SlotIndex,Slot in enumerate(Mesh.get_editor_property('static_materials')):
            MaterialName=str(Slot.get_editor_property('material_slot_name'))
            if MaterialName not in Paints:
                if MaterialName not in Metadata['palette']:
                    raise RuntimeError('Unmapped material slot: '+Name+'/'+MaterialName)
                Paints[MaterialName]=CreatePaint(MaterialName,Metadata['palette'][MaterialName])
            Mesh.set_material(SlotIndex,Paints[MaterialName])
        MeshEditor.remove_collisions(Mesh)
        Setup=Mesh.get_editor_property('body_setup')
        Setup.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
        if Information['collision']=='deck':
            if not MeshEditor.set_convex_decomposition_collisions(Mesh,1,64,100000):
                raise RuntimeError('Deck convex collision generation failed')
        elif Information['collision']=='convex':
            MeshEditor.add_simple_collisions(Mesh,unreal.ScriptCollisionShapeType.BOX)
        elif Information['collision'] in ['crate','fish','boxes']:
            Geometry=Setup.get_editor_property('agg_geom')
            Boxes=[]
            Specifications=[((0,0,-18),(70,50,4)),((0,-23,0),(70,4,40)),((0,23,0),(70,4,40)),((-33,0,0),(4,42,40)),((33,0,0),(4,42,40))]
            if Information['collision']=='fish':
                Specifications=[((0,0,0),(50,15,17))]
            if Information['collision']=='boxes':
                Specifications=[(Entry['center'],Entry['size']) for Entry in Information['boxes']]
            for Center,Size in Specifications:
                Box=unreal.KBoxElem()
                Box.set_editor_property('center',unreal.Vector(*Center))
                Box.set_editor_property('x',Size[0])
                Box.set_editor_property('y',Size[1])
                Box.set_editor_property('z',Size[2])
                Boxes.append(Box)
            Geometry.set_editor_property('box_elems',Boxes)
            Setup.set_editor_property('agg_geom',Geometry)
        else:
            Setup.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        Setup.set_editor_property('double_sided_geometry',True)
        unreal.EditorAssetLibrary.save_loaded_asset(Mesh)
        print('Configured mesh:',Name,Mesh.get_num_sections(0))

    print('Fishing assets imported and configured')


if __name__ == '__main__':
    ImportFishingAssets()
