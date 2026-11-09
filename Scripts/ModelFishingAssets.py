import bpy
import bmesh
import math
import json
import pathlib
from mathutils import Vector

ProjectDirectory = pathlib.Path('M:/Prototypes/Yakut-Simulator')
OutputDirectory = ProjectDirectory / 'SourceAssets/Fishing/Models'
OutputDirectory.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
Materials = {}
Palette = {
    'HullBlue': (0.025,0.12,0.16,1), 'DeckDark': (0.075,0.10,0.105,1),
    'Ivory': (0.72,0.73,0.66,1), 'SafetyOrange': (0.65,0.19,0.035,1),
    'Steel': (0.38,0.43,0.44,1), 'DarkSteel': (0.06,0.075,0.08,1),
    'Rubber': (0.018,0.025,0.027,1), 'WindowGlass': (0.025,0.07,0.09,1),
    'Rope': (0.36,0.31,0.20,1), 'NetGreen': (0.08,0.22,0.15,1),
    'FloatYellow': (0.87,0.61,0.10,1), 'CrateBlue': (0.035,0.24,0.32,1),
    'FishSilver': (0.50,0.62,0.61,1), 'FishBack': (0.12,0.24,0.22,1),
    'FishBlue': (0.22,0.36,0.42,1), 'Eye': (0.007,0.009,0.01,1),
    'Stone': (0.12,0.14,0.13,1)
}
for Name, Color in Palette.items():
    Material = bpy.data.materials.new(Name)
    Material.diffuse_color = Color
    Materials[Name] = Material
CurrentCollection = None
AssetMetadata = {}


def BeginAsset(Name, Collision='complex'):
    global CurrentCollection
    CurrentCollection = bpy.data.collections.new(Name)
    bpy.context.scene.collection.children.link(CurrentCollection)
    AssetMetadata[Name] = {'collision': Collision}


def Adopt(Object, MaterialName):
    for Collection in list(Object.users_collection):
        Collection.objects.unlink(Object)
    CurrentCollection.objects.link(Object)
    Object.data.materials.append(Materials[MaterialName])
    return Object


def Box(Name, Center, Size, MaterialName, Bevel=0.025):
    bpy.ops.mesh.primitive_cube_add(size=1, location=Center)
    Object = Adopt(bpy.context.object, MaterialName)
    Object.name = Name
    Object.dimensions = Size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if Bevel:
        Modifier = Object.modifiers.new('RoundedEdges', 'BEVEL')
        Modifier.width = Bevel
        Modifier.segments = 3
        Modifier = Object.modifiers.new('WeightedNormals', 'WEIGHTED_NORMAL')
    return Object


def Sphere(Name, Center, Radius, MaterialName, Scale=(1,1,1)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=16, ring_count=8, radius=Radius, location=Center)
    Object = Adopt(bpy.context.object, MaterialName)
    Object.name = Name
    Object.scale = Scale
    for Polygon in Object.data.polygons:
        Polygon.use_smooth = True
    return Object


def Tube(Name, Points, Radius, MaterialName, Resolution=8):
    Curve = bpy.data.curves.new(Name, 'CURVE')
    Curve.dimensions = '3D'
    Curve.bevel_depth = Radius
    Curve.bevel_resolution = 1
    Spline = Curve.splines.new('POLY')
    Spline.points.add(len(Points)-1)
    for Point, Position in zip(Spline.points, Points):
        Point.co = (*Position, 1)
    Object = bpy.data.objects.new(Name, Curve)
    CurrentCollection.objects.link(Object)
    Curve.materials.append(Materials[MaterialName])
    return Object


def Cylinder(Name, First, Second, Radius, MaterialName, Segments=24):
    Direction = Vector(Second) - Vector(First)
    bpy.ops.mesh.primitive_cylinder_add(vertices=Segments, radius=Radius, depth=Direction.length, location=(Vector(First)+Vector(Second))/2)
    Object = Adopt(bpy.context.object, MaterialName)
    Object.name = Name
    Object.rotation_euler = Direction.to_track_quat('Z','Y').to_euler()
    Bevel = Object.modifiers.new('EdgeBevel', 'BEVEL')
    Bevel.width = min(0.015, Radius * 0.12)
    Bevel.segments = 2
    for Polygon in Object.data.polygons:
        Polygon.use_smooth = True
    return Object


def Mesh(Name, Vertices, Faces, MaterialName):
    Data = bpy.data.meshes.new(Name)
    Data.from_pydata(Vertices, [], Faces)
    Data.update()
    Geometry = bmesh.new()
    Geometry.from_mesh(Data)
    bmesh.ops.recalc_face_normals(Geometry, faces=Geometry.faces)
    Geometry.to_mesh(Data)
    Geometry.free()
    Object = bpy.data.objects.new(Name, Data)
    CurrentCollection.objects.link(Object)
    Data.materials.append(Materials[MaterialName])
    return Object


def Ring(Name, Center, Radius, MaterialName, MinorRadius=0.025, Axis='Z'):
    Points = []
    for Segment in range(49):
        Angle = Segment * math.tau / 48
        if Axis == 'Y':
            Offset = (math.cos(Angle)*Radius,0,math.sin(Angle)*Radius)
        else:
            Offset = (math.cos(Angle)*Radius,math.sin(Angle)*Radius,0)
        Points.append(tuple(Center[Coordinate] + Offset[Coordinate] for Coordinate in range(3)))
    return Tube(Name, Points, MinorRadius, MaterialName)


Sections = [(-7.6,3.0),(-7.1,3.65),(-5.8,3.9),(-2,3.9),(2,3.85),(5.2,3.5),(7.2,2.65),(8.6,1.55),(9.6,0.35),(9.8,0.03)]
BeginAsset('SM_TrawlerHull')
Vertices=[]
for Longitudinal, Width in Sections:
    for Lateral, Height in [(0,-2.05),(Width*.60,-1.7),(Width,-.55),(Width,.45),(-Width,.45),(-Width,-.55),(-Width*.60,-1.7)]:
        Vertices.append((Longitudinal,Lateral,Height))
Faces=[]
for SectionIndex in range(len(Sections)-1):
    for ProfileIndex in range(7):
        NextProfile=(ProfileIndex+1)%7
        Faces.append((SectionIndex*7+ProfileIndex,SectionIndex*7+NextProfile,(SectionIndex+1)*7+NextProfile,(SectionIndex+1)*7+ProfileIndex))
Faces.extend([tuple(range(6,-1,-1)),tuple((len(Sections)-1)*7+Index for Index in range(7))])
Hull=Mesh('CurvedSteelHull',Vertices,Faces,'HullBlue')
for Polygon in Hull.data.polygons:
    Polygon.use_smooth=True
for Side in [-1,1]:
    Outline=[(Longitudinal,Side*Width,.48) for Longitudinal,Width in Sections]
    Tube('Gunwale',Outline,.065,'Steel')
    Tube('WaterlineTrim',[(Longitudinal,Side*Width*.96,-.65) for Longitudinal,Width in Sections],.045,'SafetyOrange')
    for Longitudinal,Width in Sections[:-2]:
        if Longitudinal < 5.3:
            Cylinder('Stanchion',(Longitudinal,Side*Width,.48),(Longitudinal,Side*Width,1.38),.026,'Steel')
    Tube('Handrail',[(Longitudinal,Side*Width,1.38) for Longitudinal,Width in Sections],.032,'Steel')
    Tube('MiddleRail',[(Longitudinal,Side*Width,.95) for Longitudinal,Width in Sections],.022,'Steel')
    for Longitudinal in [-5.6,-1.0,4.0]:
        Width=min(Sections,key=lambda Entry:abs(Entry[0]-Longitudinal))[1]
        Ring('RubberFender',(Longitudinal,Side*(Width+.05),-.10),.32,'Rubber',.075,'Y')
        Tube('FenderRope',[(Longitudinal,Side*Width,.75),(Longitudinal,Side*(Width+.06),.20)],.018,'Rope')

BeginAsset('SM_TrawlerDeck','deck')
Outline=[(Longitudinal,Width,.46) for Longitudinal,Width in Sections]
Outline.extend((Longitudinal,-Width,.46) for Longitudinal,Width in reversed(Sections))
VertexCount=len(Outline)
Vertices=Outline+[(Longitudinal,Lateral,.34) for Longitudinal,Lateral,Height in Outline]
Faces=[tuple(reversed(range(VertexCount))),tuple(range(VertexCount,VertexCount*2))]
for VertexIndex in range(VertexCount):
    NextVertex=(VertexIndex+1)%VertexCount
    Faces.append((VertexIndex,NextVertex,NextVertex+VertexCount,VertexIndex+VertexCount))
Mesh('DeckSurface',Vertices,Faces,'DeckDark')
for Longitudinal in [-6,-4,-2,0,2,4,6]:
    Width=min(Sections,key=lambda Entry:abs(Entry[0]-Longitudinal))[1]-.15
    Box('DeckSeam',(Longitudinal,0,.465),(.012,Width*2,.006),'DarkSteel',0)

BeginAsset('SM_Wheelhouse')
Box('Cabin',(3.6,0,1.65),(4.7,4.45,2.4),'Ivory',.20)
Box('CabinRoof',(3.6,0,2.94),(5.1,4.85,.23),'Ivory',.16)
Box('RoofBand',(3.6,0,2.78),(4.85,4.6,.12),'HullBlue',.06)
for Lateral in [-1.45,0,1.45]:
    Frame=Box('WindowFrame',(1.225,Lateral,2.12),(.13,1.29,.96),'DarkSteel',.09)
    Box('WindowGlass',(1.148,Lateral,2.12),(.035,1.12,.79),'WindowGlass',.07)
    Cylinder('Wiper',(1.10,Lateral-.4,1.78),(1.10,Lateral+.30,2.37),.012,'Rubber',10)
Box('CabinStripe',(1.20,0,1.05),(.065,4.1,.27),'HullBlue',.02)
for Side in [-1,1]:
    for Longitudinal in [2.3,3.75,5.1]:
        Box('SideWindowFrame',(Longitudinal,Side*2.24,2.12),(1.06,.10,.90),'DarkSteel',.07)
        Box('SideWindow',(Longitudinal,Side*2.30,2.12),(.91,.025,.74),'WindowGlass',.04)
    Box('SideStripe',(3.7,Side*2.24,1.05),(4.2,.05,.27),'HullBlue',.01)
    Tube('GrabRail',[(1.4,Side*2.37,.8),(1.4,Side*2.37,1.8)],.023,'Steel')
Cylinder('Mast',(3.9,0,3),(3.9,0,5.5),.07,'Ivory')
Cylinder('MastArm',(3.9,-1.15,4.9),(3.9,1.15,4.9),.035,'Steel')
Cylinder('RadarPedestal',(4.1,0,5.5),(4.1,0,5.85),.08,'DarkSteel')
Box('Radar',(4.1,0,5.94),(.25,1.65,.16),'Ivory',.06)
for Side in [-1,1]:
    Cylinder('Antenna',(4.3,Side*.75,3.05),(4.3,Side*.75,4.75),.012,'Steel',8)
    Ring('Lifebuoy',(1.10,Side*1.45,1.03),.31,'SafetyOrange',.065,'Y')
    Box('DeckLamp',(1.03,Side*2.1,2.72),(.18,.19,.23),'DarkSteel',.025)
    Box('LampGlass',(.93,Side*2.1,2.72),(.03,.14,.17),'FloatYellow',.01)

BeginAsset('SM_SternFrame')
for Side in [-1,1]:
    Tube('AFrame',[(-6.7,Side*2.8,.5),(-6.65,Side*2.8,1),(-6.9,Side*2.25,3.6),(-6.9,Side*1.9,3.85)],.15,'SafetyOrange')
    Box('FrameFoot',(-6.7,Side*2.8,.60),(.65,.65,.20),'DarkSteel',.04)
    Cylinder('PivotPin',(-6.7,Side*2.95,.95),(-6.7,Side*2.65,.95),.19,'Steel')
    Ring('Fairlead',(-6.95,Side*1.25,3.55),.24,'DarkSteel',.065,'Y')
    Cylinder('FrameBrace',(-6.7,Side*2.8,.9),(-5.4,Side*2.8,.5),.065,'DarkSteel')
Tube('AFrameCrossbar',[(-6.9,-1.95,3.85),(-6.9,1.95,3.85)],.15,'SafetyOrange')

BeginAsset('SM_WinchBase')
Box('WinchBed',(0,0,.1),(1.35,1.8,.20),'DarkSteel',.06)
for Side in [-1,1]:
    Box('WinchBearing',(0,Side*.77,.57),(.70,.18,1.05),'SafetyOrange',.08)
    Cylinder('BearingHub',(0,Side*.6,.72),(0,Side*.9,.72),.20,'Steel')
Cylinder('Motor',(.40,-.80,.33),(.40,-1.15,.33),.24,'HullBlue')
for Longitudinal in [-.45,.45]:
    for Side in [-1,1]:
        Cylinder('Bolt',(Longitudinal,Side*.66,.2),(Longitudinal,Side*.66,.25),.045,'Steel',6)

BeginAsset('SM_WinchDrum')
Cylinder('DrumCore',(0,-.62,0),(0,.62,0),.36,'DarkSteel')
for Side in [-1,1]:
    Cylinder('DrumFlange',(0,Side*.61-.04,0),(0,Side*.61+.04,0),.64,'Steel')
    Ring('FlangeRim',(0,Side*.66,0),.59,'DarkSteel',.04,'Y')
    Cylinder('Axle',(0,Side*.62,0),(0,Side*.82,0),.14,'Steel')
Points=[]
for Segment in range(640):
    Angle=Segment*math.tau/32
    Points.append((math.cos(Angle)*.40,-.56+Segment/639*1.12,math.sin(Angle)*.40))
Tube('WoundCable',Points,.026,'Rope')

BeginAsset('SM_TrawlNet','none')
RingPoints=[(0,math.cos(Segment*math.tau/64)*1.2,math.sin(Segment*math.tau/64)*.65) for Segment in range(65)]
Tube('MouthRope',RingPoints,.035,'Rope')
for Strand in range(24):
    for Twist in [-1,1]:
        Points=[]
        for Row in range(25):
            Distance=Row/24
            Angle=Strand*math.tau/24+Twist*Distance*math.tau
            Radius=1-Distance*.84
            Points.append((-4.3*Distance,math.cos(Angle)*1.2*Radius,math.sin(Angle)*.65*Radius))
        Tube('NetStrand',Points,.009,'NetGreen')
for Row in [0,6,12,18,24]:
    Distance=Row/24
    Tube('NetBand',[(-4.3*Distance,math.cos(Segment*math.tau/48)*1.2*(1-Distance*.84),math.sin(Segment*math.tau/48)*.65*(1-Distance*.84)) for Segment in range(49)],.015,'NetGreen')
for FloatIndex in range(9):
    Angle=FloatIndex*math.pi/8
    Sphere('NetFloat',(0,math.cos(Angle)*1.23,math.sin(Angle)*.68),.09,'FloatYellow',(.8,1,1.2))
Tube('CodEndTie',[(-4.3,-.2,-.1),(-4.3,.2,.1)],.04,'Rope')

for AssetName,BackMaterial,Length in [('SM_FishCod','FishBack',.50),('SM_FishHaddock','FishBlue',.56)]:
    BeginAsset(AssetName,'fish')
    Sphere('FishBody',(0,0,0),1,'FishSilver',(Length*.5,.075,.085))
    Sphere('FishBack',(0,0,.028),1,BackMaterial,(Length*.46,.071,.060))
    Mesh('Tail',[(-Length*.43,0,0),(-Length*.66,0,.12),(-Length*.66,0,-.12),(-Length*.43,.015,0),(-Length*.66,.015,.12),(-Length*.66,.015,-.12)],[(0,1,2),(3,5,4),(0,3,4,1),(1,4,5,2),(2,5,3,0)],BackMaterial)
    Mesh('DorsalFin',[(-.10,0,.06),(.09,0,.06),(-.02,0,.17),(-.10,.015,.06),(.09,.015,.06),(-.02,.015,.17)],[(0,1,2),(3,5,4),(0,3,4,1),(1,4,5,2),(2,5,3,0)],BackMaterial)
    for Side in [-1,1]:
        Sphere('FishEye',(Length*.32,Side*.053,.025),.018,'Eye')

BeginAsset('SM_FishingKnife','convex')
Box('KnifeHandle',(-.055,0,0),(.16,.034,.032),'Rubber',.013)
for Longitudinal in [-.10,-.055,-.01]:
    Sphere('HandleRivet',(Longitudinal,.018,0),.005,'Steel')
Mesh('Blade',[(.02,-.002,-.018),(.24,-.002,-.006),(.18,-.002,.035),(.02,-.002,.035),(.02,.002,-.018),(.24,.002,-.006),(.18,.002,.035),(.02,.002,.035)],[(0,1,2,3),(4,7,6,5),(0,4,5,1),(1,5,6,2),(2,6,7,3),(3,7,4,0)],'Steel')
BeginAsset('SM_SharpeningStone')
Box('StoneBase',(0,0,-.035),(.43,.20,.05),'Rubber',.02)
Box('Stone',(0,0,0),(.38,.15,.06),'Stone',.018)

BeginAsset('SM_FishingConveyor')
Box('Belt',(0,0,0),(1.45,.78,.08),'Rubber',.025)
for Side in [-1,1]:
    Box('BeltSide',(0,Side*.45,.08),(1.55,.08,.25),'Steel',.035)
    for Longitudinal in [-.55,.55]:
        Cylinder('Leg',(Longitudinal,Side*.37,-.88),(Longitudinal,Side*.37,-.06),.035,'Steel')
for RollerIndex in range(12):
    Longitudinal=-.65+RollerIndex*.118
    Cylinder('Roller',(Longitudinal,-.39,.055),(Longitudinal,.39,.055),.025,'DarkSteel',12)
Cylinder('BeltMotor',(-.55,-.52,-.10),(-.55,-.80,-.10),.12,'SafetyOrange')

BeginAsset('SM_ProcessingBench')
Box('Worktop',(0,0,0),(1.6,.95,.08),'Steel',.04)
for Side in [-1,1]:
    Box('SplashGuard',(0,Side*.46,.055),(1.58,.04,.15),'Steel',.015)
    for Longitudinal in [-.65,.65]:
        Cylinder('BenchLeg',(Longitudinal,Side*.37,-.90),(Longitudinal,Side*.37,-.04),.035,'Steel')
Box('Shelf',(0,0,-.62),(1.4,.75,.04),'DarkSteel',.015)

BeginAsset('SM_FishingFreezer')
Box('FreezerBottom',(0,0,.06),(1.5,1.1,.12),'Ivory',.05)
for Side in [-1,1]:
    Box('FreezerSide',(0,Side*.50,.43),(1.5,.12,.74),'Ivory',.045)
    Box('FreezerEnd',(Side*.69,0,.43),(.12,.94,.74),'Ivory',.045)
Lid=Box('OpenFreezerLid',(0,.73,1.08),(1.5,.12,.80),'Ivory',.05)
Lid.rotation_euler.x=math.radians(-15)
Box('Gasket',(0,.65,1.08),(1.35,.035,.65),'Rubber',.025)
Cylinder('FreezerHandle',(-.25,.65,1.45),(.25,.65,1.45),.025,'Steel')

BeginAsset('SM_FishingCrate','crate')
Box('CrateBottom',(0,0,-.18),(.70,.50,.04),'CrateBlue',.012)
for Side in [-1,1]:
    for Height in [-.10,.01,.12]:
        Box('CrateLongSide',(0,Side*.23,Height),(.70,.04,.08),'CrateBlue',.012)
        Box('CrateShortSide',(Side*.33,0,Height),(.04,.42,.08),'CrateBlue',.012)
    for Corner in [-1,1]:
        Box('CrateCorner',(Side*.32,Corner*.22,0),(.06,.06,.4),'CrateBlue',.012)

BeginAsset('SM_SonarDisplay')
Box('DisplayHousing',(-.04,0,0),(.06,.81,.57),'DarkSteel',.025)
for Side in [-1,1]:
    Box('DisplaySide',(-.01,Side*.395,0),(.04,.025,.57),'Steel',.008)
    Box('DisplayEdge',(-.01,0,Side*.275),(.04,.81,.025),'Steel',.008)
Cylinder('DisplayStand',(-.05,0,-1.60),(-.05,0,-.27),.045,'DarkSteel')
Box('DisplayFoot',(-.05,0,-1.62),(.35,.45,.06),'DarkSteel',.025)

BeginAsset('SM_ControlConsole')
Box('Pedestal',(0,0,-.30),(1.1,.75,.65),'HullBlue',.08)
Box('ControlPanel',(0,0,.04),(1.26,.87,.12),'DarkSteel',.06)
for Longitudinal in [-.53,.53]:
    for Side in [-1,1]:
        Cylinder('PanelBolt',(Longitudinal,Side*.34,.1),(Longitudinal,Side*.34,.13),.025,'Steel',6)
Tube('SafetyGuard',[(-.65,-.43,.12),(-.65,-.43,.3),(.65,-.43,.3),(.65,-.43,.12)],.025,'Steel')

DependencyGraph=bpy.context.evaluated_depsgraph_get()
MaterialText=[]
for Name,Color in Palette.items():
    MaterialText.extend(['newmtl '+Name,'Kd {} {} {}'.format(*Color[:3]),'Ks 0.3 0.3 0.3','Ns 64'])
(OutputDirectory/'FishingMaterials.mtl').write_text('\n'.join(MaterialText),encoding='utf-8')
for AssetName in AssetMetadata:
    Collection=bpy.data.collections[AssetName]
    Lines=['mtllib FishingMaterials.mtl','o '+AssetName]
    VertexOffset=0
    CornerOffset=0
    UsedMaterials=[]
    MaterialFaces={}
    for Object in Collection.objects:
        Evaluated=Object.evaluated_get(DependencyGraph)
        Data=Evaluated.to_mesh()
        Data.calc_loop_triangles()
        for Vertex in Data.vertices:
            Position=Object.matrix_world @ Vertex.co
            Lines.append('v {:.6f} {:.6f} {:.6f}'.format(Position.x*100,Position.y*100,Position.z*100))
        NormalTransform=Object.matrix_world.to_3x3().inverted().transposed()
        for Loop in Data.loops:
            Position=Data.vertices[Loop.vertex_index].co
            Lines.append('vt {:.6f} {:.6f}'.format(Position.x,Position.y+Position.z))
            Normal=(NormalTransform @ Data.corner_normals[Loop.index].vector).normalized()
            Lines.append('vn {:.6f} {:.6f} {:.6f}'.format(*Normal))
        CurrentMaterial=None
        for Triangle in Data.loop_triangles:
            MaterialName=Data.materials[Triangle.material_index].name
            if MaterialName not in UsedMaterials:
                UsedMaterials.append(MaterialName)
                MaterialFaces[MaterialName]=[]
            Face=[]
            for LoopIndex in Triangle.loops:
                VertexIndex=VertexOffset+Data.loops[LoopIndex].vertex_index+1
                CornerIndex=CornerOffset+LoopIndex+1
                Face.append('{}/{}/{}'.format(VertexIndex,CornerIndex,CornerIndex))
            MaterialFaces[MaterialName].append('f '+' '.join(Face))
        VertexOffset+=len(Data.vertices)
        CornerOffset+=len(Data.loops)
        Evaluated.to_mesh_clear()
    for MaterialName,Faces in MaterialFaces.items():
        Lines.append('usemtl '+MaterialName)
        Lines.extend(Faces)
    (OutputDirectory/(AssetName+'.obj')).write_text('\n'.join(Lines),encoding='utf-8')
    AssetMetadata[AssetName]['materials']=UsedMaterials
    print('Exported',AssetName,VertexOffset)
(OutputDirectory/'FishingAssets.json').write_text(json.dumps({'palette':Palette,'assets':AssetMetadata},indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OutputDirectory/'FishingModels.blend'))
print('Fishing model source and meshes saved')
