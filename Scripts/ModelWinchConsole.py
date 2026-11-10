import bpy
import json
import math
import pathlib
from mathutils import Matrix, Vector

ProjectDirectory = pathlib.Path('M:/Prototypes/Yakut-Simulator').resolve()
OutputDirectory = ProjectDirectory / 'SourceAssets/Fishing/WinchConsole'
OutputDirectory.mkdir(parents=True, exist_ok=True)
SceneName = 'YakutWinchConsole'
Scene = bpy.data.scenes.get(SceneName)
if Scene:
    for Object in list(Scene.objects):
        bpy.data.objects.remove(Object, do_unlink=True)
    for Collection in list(Scene.collection.children):
        bpy.data.collections.remove(Collection)
else:
    Scene = bpy.data.scenes.new(SceneName)
bpy.context.window.scene = Scene
Scene.unit_settings.system = 'METRIC'
Palette = {
    'ConsolePaint': (.035, .12, .20, 1), 'PanelPrint': (.04, .15, .22, 1),
    'Rubber': (.012, .017, .02, 1), 'Steel': (.32, .36, .38, 1),
    'DarkSteel': (.038, .05, .055, 1), 'Yellow': (.8, .59, .045, 1),
    'Red': (.55, .025, .014, 1), 'Green': (.035, .29, .12, 1),
    'DialPrint': (.7, .65, .45, 1), 'Needle': (.65, .045, .018, 1),
    'Glass': (.12, .2, .22, 1), 'Screen': (.006, .013, .014, 1)
}
Materials = {}
for Name, Color in Palette.items():
    Material = bpy.data.materials.new('Winch_' + Name)
    Material.use_nodes = True
    Shader = next(Node for Node in Material.node_tree.nodes if Node.type == 'BSDF_PRINCIPLED')
    Shader.inputs['Base Color'].default_value = Color
    Shader.inputs['Roughness'].default_value = .6
    if Name in ['Steel', 'DarkSteel']:
        Shader.inputs['Metallic'].default_value = .8
        Shader.inputs['Roughness'].default_value = .3
    if Name in ['PanelPrint', 'DialPrint', 'Screen']:
        Texture = Material.node_tree.nodes.new('ShaderNodeTexImage')
        Texture.image = bpy.data.images.load(str(OutputDirectory / {'PanelPrint': 'T_WinchPanel.png', 'DialPrint': 'T_WinchDial.png', 'Screen': 'T_WinchReadout.png'}[Name]), check_existing=True)
        Material.node_tree.links.new(Texture.outputs['Color'], Shader.inputs['Base Color'])
    Materials[Name] = Material
Assets = {}
CurrentCollection = None


def BeginAsset(Name, Collision='none'):
    global CurrentCollection
    CurrentCollection = bpy.data.collections.new(Name)
    Scene.collection.children.link(CurrentCollection)
    Assets[Name] = {'collision': Collision}


def Adopt(Object, Name, Material):
    for Collection in list(Object.users_collection):
        Collection.objects.unlink(Object)
    CurrentCollection.objects.link(Object)
    Object.name = Name
    Object.data.materials.append(Materials[Material])
    return Object


def Box(Name, Center, Size, Material, Bevel=.005):
    bpy.ops.mesh.primitive_cube_add(size=1, location=Center)
    Object = Adopt(bpy.context.object, Name, Material)
    Object.dimensions = Size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if Bevel > 0:
        Modifier = Object.modifiers.new('EdgeRadius', 'BEVEL')
        Modifier.width = Bevel
        Modifier.segments = 3
        Object.modifiers.new('WeightedNormals', 'WEIGHTED_NORMAL')
    return Object


def Cylinder(Name, First, Second, Radius, Material, Segments=32):
    Direction = Vector(Second) - Vector(First)
    bpy.ops.mesh.primitive_cylinder_add(vertices=Segments, radius=Radius, depth=Direction.length, location=(Vector(First) + Vector(Second)) / 2)
    Object = Adopt(bpy.context.object, Name, Material)
    Object.rotation_euler = Direction.to_track_quat('Z', 'Y').to_euler()
    Modifier = Object.modifiers.new('EdgeRadius', 'BEVEL')
    Modifier.width = min(.003, Radius * .12)
    Modifier.segments = 2
    Object.modifiers.new('WeightedNormals', 'WEIGHTED_NORMAL')
    return Object


def Sphere(Name, Center, Radius, Material):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=Radius, location=Center)
    Object = Adopt(bpy.context.object, Name, Material)
    for Polygon in Object.data.polygons:
        Polygon.use_smooth = True
    return Object


def Tube(Name, Points, Radius, Material):
    Curve = bpy.data.curves.new(Name, 'CURVE')
    Curve.dimensions = '3D'
    Curve.bevel_depth = Radius
    Curve.bevel_resolution = 3
    Spline = Curve.splines.new('POLY')
    Spline.points.add(len(Points) - 1)
    for Point, Position in zip(Spline.points, Points):
        Point.co = (*Position, 1)
    Object = bpy.data.objects.new(Name, Curve)
    CurrentCollection.objects.link(Object)
    Curve.materials.append(Materials[Material])
    return Object


def SurfaceHeight(Depth):
    return 1.04 - .32 * Depth


def PanelPlane(Name, Horizontal, Depth, Width, Height, Material, Elevation=.004):
    Vertices = [(Horizontal - Width / 2, Depth - Height / 2, SurfaceHeight(Depth - Height / 2) + Elevation), (Horizontal + Width / 2, Depth - Height / 2, SurfaceHeight(Depth - Height / 2) + Elevation), (Horizontal + Width / 2, Depth + Height / 2, SurfaceHeight(Depth + Height / 2) + Elevation), (Horizontal - Width / 2, Depth + Height / 2, SurfaceHeight(Depth + Height / 2) + Elevation)]
    Mesh = bpy.data.meshes.new(Name)
    Mesh.from_pydata(Vertices, [], [(0, 1, 2, 3)])
    Mesh.update()
    Object = bpy.data.objects.new(Name, Mesh)
    CurrentCollection.objects.link(Object)
    Mesh.materials.append(Materials[Material])
    Coordinates = [(0, 1), (1, 1), (1, 0), (0, 0)]
    Layer = Mesh.uv_layers.new()
    for Index, Coordinate in enumerate(Coordinates):
        Layer.data[Index].uv = Coordinate
    return Object


def PanelBox(Name, Horizontal, Depth, Width, Height, Thickness, Material, Offset=0):
    Object = Box(Name, (Horizontal, Depth, SurfaceHeight(Depth) + Offset), (Width, Height, Thickness), Material)
    Object.rotation_euler.x = -math.atan(.32)
    return Object


def DialFace(Horizontal, Depth):
    Vertices = []
    Coordinates = []
    for Index in range(64):
        Angle = Index * math.tau / 64
        OffsetHorizontal = math.cos(Angle) * .097
        OffsetDepth = math.sin(Angle) * .097
        Vertices.append((Horizontal + OffsetHorizontal, Depth + OffsetDepth, SurfaceHeight(Depth + OffsetDepth) + .035))
        Coordinates.append((.5 + OffsetHorizontal / .194, .5 - OffsetDepth / .194))
    Mesh = bpy.data.meshes.new('PrintedGaugeFace')
    Mesh.from_pydata(Vertices, [], [tuple(range(64))])
    Mesh.update()
    Object = bpy.data.objects.new('PrintedGaugeFace', Mesh)
    CurrentCollection.objects.link(Object)
    Mesh.materials.append(Materials['DialPrint'])
    Layer = Mesh.uv_layers.new()
    for Index, Coordinate in enumerate(Coordinates):
        Layer.data[Index].uv = Coordinate


BeginAsset('SM_WinchControlConsole', 'cabinet')
Box('CabinetFoot', (0, 0, .055), (1.15, .62, .11), 'DarkSteel', .02)
Box('CabinetPlinth', (0, 0, .41), (1.04, .55, .72), 'ConsolePaint', .025)
Box('ServiceDoor', (0, .285, .48), (.9, .022, .55), 'ConsolePaint', .008)
Box('DoorLock', (.34, .305, .52), (.022, .018, .05), 'Steel', .002)
for Horizontal in [-.39, .39]:
    for Vertical in [.29, .68]:
        Box('DoorHinge', (Horizontal, .302, Vertical), (.027, .028, .064), 'Steel', .003)
for Index in range(7):
    Box('VentSlot', (-.25 + Index * .085, .301, .26), (.047, .006, .008), 'Rubber', .002)
Vertices = []
for Horizontal in [-.64, .64]:
    Vertices.extend([(Horizontal, -.4, .78), (Horizontal, .4, .78), (Horizontal, .4, SurfaceHeight(.4)), (Horizontal, -.4, SurfaceHeight(-.4))])
Mesh = bpy.data.meshes.new('SlopedCabinet')
Mesh.from_pydata(Vertices, [], [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (3, 7, 6, 2), (0, 4, 7, 3)])
Mesh.update()
Object = bpy.data.objects.new('SlopedCabinet', Mesh)
CurrentCollection.objects.link(Object)
Mesh.materials.append(Materials['ConsolePaint'])
Modifier = Object.modifiers.new('EdgeRadius', 'BEVEL')
Modifier.width = .012
Modifier.segments = 3
Object.modifiers.new('WeightedNormals', 'WEIGHTED_NORMAL')
PanelPlane('PrintedControlPlate', 0, 0, 1.26, .78, 'PanelPrint')
for Horizontal in [-.605, .605]:
    for Depth in [-.35, .35]:
        Cylinder('PanelFastener', (Horizontal, Depth, SurfaceHeight(Depth) + .005), (Horizontal, Depth, SurfaceHeight(Depth) + .011), .009, 'Steel', 12)
        Box('ScrewSlot', (Horizontal, Depth, SurfaceHeight(Depth) + .012), (.011, .002, .001), 'DarkSteel', 0)
    Tube('GrabRail', [(Horizontal, .39, .82), (Horizontal, .39, 1.06), (Horizontal, -.34, 1.29), (Horizontal, -.43, 1.2)], .018, 'Steel')
PanelBox('ReadoutBezel', 0, .287, .885, .178, .018, 'Rubber', .009)
PanelPlane('ReadoutRecess', 0, .287, .85, .145, 'Screen', .022)
for Horizontal in [-.435, .435]:
    for Depth in [.215, .36]:
        Cylinder('BezelScrew', (Horizontal, Depth, SurfaceHeight(Depth) + .023), (Horizontal, Depth, SurfaceHeight(Depth) + .027), .005, 'Steel', 8)
for Horizontal in [-.445, .445]:
    Depth = -.255
    Normal = Vector((0, .32, 1)).normalized()
    Center = Vector((Horizontal, Depth, SurfaceHeight(Depth)))
    Cylinder('InstrumentMount', Center, Center + Normal * .021, .114, 'DarkSteel', 64)
    Cylinder('InstrumentRim', Center + Normal * .017, Center + Normal * .03, .108, 'Steel', 64)
    DialFace(Horizontal, Depth)
    for Corner in [-1, 1]:
        Cylinder('GaugeMountScrew', (Horizontal + Corner * .102, Depth, SurfaceHeight(Depth) + .027), (Horizontal + Corner * .102, Depth, SurfaceHeight(Depth) + .033), .004, 'Steel', 8)

BeginAsset('SM_WinchJoystickMount', 'box')
Box('MountingFlange', (0, 0, .005), (.23, .215, .014), 'DarkSteel', .014)
for Index in range(6):
    Size = .205 - Index * .025
    Box('BellowsFold', (0, 0, .017 + Index * .009), (Size, Size * .93, .012), 'Rubber', .014)
for Horizontal in [-.095, .095]:
    for Depth in [-.085, .085]:
        Cylinder('MountBolt', (Horizontal, Depth, .012), (Horizontal, Depth, .018), .005, 'Steel', 10)

BeginAsset('SM_WinchJoystick', 'convex')
Cylinder('JoystickStem', (0, 0, -.005), (0, 0, .12), .012, 'Steel')
Cylinder('GripCollar', (0, 0, .09), (0, 0, .122), .017, 'Rubber')
Sphere('YellowGrip', (0, 0, .142), .036, 'Yellow')

BeginAsset('SM_WinchRotaryMount', 'box')
Cylinder('MountingFlange', (-.012, 0, 0), (.006, 0, 0), .039, 'DarkSteel', 48)
Cylinder('LockNut', (.005, 0, 0), (.015, 0, 0), .024, 'Steel', 6)

BeginAsset('SM_WinchRotaryKnob', 'convex')
Cylinder('RotaryShaft', (0, 0, 0), (.02, 0, 0), .012, 'Steel')
Cylinder('KnurledKnob', (.012, 0, 0), (.047, 0, 0), .028, 'Rubber', 48)
for Index in range(20):
    Angle = Index * math.tau / 20
    Cylinder('GripRib', (.016, math.cos(Angle) * .028, math.sin(Angle) * .028), (.043, math.cos(Angle) * .028, math.sin(Angle) * .028), .002, 'DarkSteel', 6)
Box('PositionMark', (.048, 0, .014), (.002, .004, .02), 'Yellow', .001)

BeginAsset('SM_WinchButtonMount', 'box')
Cylinder('ButtonFlange', (-.006, 0, 0), (.009, 0, 0), .04, 'Yellow', 48)
Cylinder('ButtonCollar', (.008, 0, 0), (.025, 0, 0), .027, 'DarkSteel', 48)

BeginAsset('SM_WinchButtonStem', 'convex')
Cylinder('PushStem', (0, 0, 0), (.033, 0, 0), .019, 'DarkSteel')

BeginAsset('SM_WinchEmergencyCap')
Cylinder('MushroomCap', (0, 0, 0), (.018, 0, 0), .037, 'Red', 64)
Cylinder('CapTop', (.017, 0, 0), (.022, 0, 0), .031, 'Red', 64)

BeginAsset('SM_WinchConfirmCap')
Cylinder('GreenButtonCap', (0, 0, 0), (.012, 0, 0), .025, 'Green', 48)

BeginAsset('SM_WinchGaugeNeedle')
Box('NeedleBlade', (0, .035, .001), (.004, .078, .002), 'Needle', .001)
Cylinder('NeedleHub', (0, 0, -.002), (0, 0, .005), .012, 'DarkSteel', 32)

DependencyGraph = bpy.context.evaluated_depsgraph_get()
MaterialLines = []
for Name, Color in Palette.items():
    MaterialLines.extend(['newmtl ' + Name, 'Kd {} {} {}'.format(*Color[:3])])
(OutputDirectory / 'WinchMaterials.mtl').write_text('\n'.join(MaterialLines), encoding='utf-8')
for AssetName, Information in Assets.items():
    Collection = bpy.data.collections[AssetName]
    Lines = ['mtllib WinchMaterials.mtl', 'o ' + AssetName]
    VertexOffset = 0
    CornerOffset = 0
    MaterialFaces = {}
    for Object in Collection.objects:
        Evaluated = Object.evaluated_get(DependencyGraph)
        Data = Evaluated.to_mesh()
        Data.calc_loop_triangles()
        Data.calc_normals_split()
        for Vertex in Data.vertices:
            Position = Object.matrix_world @ Vertex.co
            Lines.append('v {:.6f} {:.6f} {:.6f}'.format(Position.x * 100, -Position.y * 100, Position.z * 100))
        NormalTransform = Object.matrix_world.to_3x3().inverted().transposed()
        for Index, Loop in enumerate(Data.loops):
            Coordinate = (0, 0)
            if Data.uv_layers.active:
                Coordinate = Data.uv_layers.active.data[Index].uv
            Lines.append('vt {:.6f} {:.6f}'.format(*Coordinate))
            Normal = (NormalTransform @ Loop.normal).normalized()
            Lines.append('vn {:.6f} {:.6f} {:.6f}'.format(Normal.x, -Normal.y, Normal.z))
        for Triangle in Data.loop_triangles:
            MaterialName = Data.materials[Triangle.material_index].name.removeprefix('Winch_').split('.')[0]
            Face = []
            for LoopIndex in Triangle.loops:
                VertexIndex = VertexOffset + Data.loops[LoopIndex].vertex_index + 1
                CornerIndex = CornerOffset + LoopIndex + 1
                Face.append('{}/{}/{}'.format(VertexIndex, CornerIndex, CornerIndex))
            MaterialFaces.setdefault(MaterialName, []).append('f ' + ' '.join(reversed(Face)))
        VertexOffset += len(Data.vertices)
        CornerOffset += len(Data.loops)
        Evaluated.to_mesh_clear()
    for MaterialName, Faces in MaterialFaces.items():
        Lines.append('usemtl ' + MaterialName)
        Lines.extend(Faces)
    (OutputDirectory / (AssetName + '.obj')).write_text('\n'.join(Lines), encoding='utf-8')
    Information['materials'] = list(MaterialFaces)
    Information['vertices'] = VertexOffset
(OutputDirectory / 'WinchConsole.json').write_text(json.dumps({'palette': Palette, 'assets': Assets}, indent=2), encoding='utf-8')
for Collection in Scene.collection.children:
    Collection.hide_render = Collection.name != 'SM_WinchControlConsole'
    Collection.hide_viewport = Collection.name != 'SM_WinchControlConsole'
Scene.world = bpy.data.worlds.new('WinchPreviewWorld')
Scene.world.use_nodes = True
Scene.world.node_tree.nodes.get('Background').inputs['Color'].default_value = (.3, .35, .4, 1)
Scene.world.node_tree.nodes.get('Background').inputs['Strength'].default_value = .6
Preview = bpy.data.collections.new('AssembledControls')
Scene.collection.children.link(Preview)


def PreviewAsset(AssetName, Position, Rotation):
    Transform = Matrix.Translation(Position) @ Rotation
    for Source in bpy.data.collections[AssetName].objects:
        Object = Source.copy()
        Object.data = Source.data
        Object.matrix_world = Transform @ Source.matrix_world
        Preview.objects.link(Object)


PanelRotation = Matrix.Rotation(-math.atan(.32), 4, 'X')
NormalRotation = PanelRotation @ Matrix.Rotation(-math.pi / 2, 4, 'Y')
for Horizontal in [-.27, .27]:
    PreviewAsset('SM_WinchJoystickMount', (Horizontal, 0, 1.048), PanelRotation)
    PreviewAsset('SM_WinchJoystick', (Horizontal, .0152, 1.096), PanelRotation)
for Horizontal in [-.515, .515]:
    PreviewAsset('SM_WinchRotaryMount', (Horizontal, .165, .9952), NormalRotation)
    PreviewAsset('SM_WinchRotaryKnob', (Horizontal, .165, .9952), NormalRotation)
for Depth, CapName in [(-.255, 'SM_WinchEmergencyCap'), (.015, 'SM_WinchConfirmCap')]:
    Position = Vector((0, Depth, SurfaceHeight(Depth) + .008))
    PreviewAsset('SM_WinchButtonMount', Position, NormalRotation)
    PreviewAsset('SM_WinchButtonStem', Position, NormalRotation)
    PreviewAsset(CapName, Position + PanelRotation.to_3x3() @ Vector((0, 0, .033)), NormalRotation)
for Horizontal in [-.445, .445]:
    PreviewAsset('SM_WinchGaugeNeedle', (Horizontal, -.255, 1.1626), PanelRotation @ Matrix.Rotation(math.radians(-50), 4, 'Z'))
bpy.ops.wm.save_as_mainfile(filepath=str(OutputDirectory / 'WinchConsole.blend'))
print(json.dumps({'assets': Assets, 'blend': str(OutputDirectory / 'WinchConsole.blend')}))
