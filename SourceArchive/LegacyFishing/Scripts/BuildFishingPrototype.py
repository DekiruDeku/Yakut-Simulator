if __name__ == '__main__':
    raise RuntimeError('This legacy generator is retired. Use BuildStationaryWorkstations.py for the active stationary minigames.')

import pathlib
import unreal
import sys

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory/'Scripts'))
from ConfigureProjectBlueprints import GetConfiguredClass
ActorSubsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
LevelSubsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
LevelSubsystem.load_level('/Game/Prototype/Maps/L_Fishing')
Actors = list(ActorSubsystem.get_all_level_actors())


def Material(Name):
    return unreal.load_asset('/Game/Prototype/Materials/' + Name)


def SpawnShape(Name, Position, Scale, Surface, Parent):
    Actor = ActorSubsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*Position))
    Actor.set_actor_label(Name)
    Component = Actor.static_mesh_component
    Component.set_mobility(unreal.ComponentMobility.MOVABLE)
    Component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    Component.set_material(0, Surface)
    Component.set_collision_profile_name('BlockAllDynamic')
    Actor.set_actor_scale3d(unreal.Vector(*Scale))
    Actor.attach_to_actor(Parent, '', unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    Parent.hull.ignore_actor_when_moving(Actor, True)
    return Actor


def CreateCrateMesh(Rebuild=False):
    AssetPath = '/Game/Prototype/Meshes/SM_DeckCrate'
    if unreal.EditorAssetLibrary.does_asset_exist(AssetPath) and not Rebuild:
        return unreal.load_asset(AssetPath)
    Blocks = [((0, 0, -18), (35, 25, 2))]
    for Side in [-1, 1]:
        for Height in [-10, 1, 12]:
            Blocks.append(((0, Side * 23, Height), (35, 2, 4)))
            Blocks.append(((Side * 33, 0, Height), (2, 21, 4)))
        for Corner in [-1, 1]:
            Blocks.append(((Side * 32, Corner * 22, 0), (3, 3, 20)))
    SourceDirectory = ProjectDirectory / 'SourceAssets/Fishing'
    SourceDirectory.mkdir(parents=True, exist_ok=True)
    SourcePath = SourceDirectory / 'SM_DeckCrate.obj'
    Lines = ['o DeckCrate']
    VertexCount = 0
    for Center, Extent in Blocks:
        for Horizontal, Lateral, Vertical in [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]:
            Lines.append('v {} {} {}'.format(Center[0] + Horizontal * Extent[0], Center[1] + Lateral * Extent[1], Center[2] + Vertical * Extent[2]))
        for Face in [(1,4,3,2),(5,6,7,8),(1,2,6,5),(2,3,7,6),(3,4,8,7),(4,1,5,8)]:
            Lines.append('f ' + ' '.join(str(VertexCount + Index) for Index in Face))
        VertexCount += 8
    SourcePath.write_text('\n'.join(Lines), encoding='utf-8')
    Task = unreal.AssetImportTask()
    Task.set_editor_property('filename', str(SourcePath))
    Task.set_editor_property('destination_path', '/Game/Prototype/Meshes')
    Task.set_editor_property('destination_name', 'SM_DeckCrate')
    Task.set_editor_property('automated', True)
    Task.set_editor_property('save', True)
    Task.set_editor_property('replace_existing', Rebuild)
    Options = unreal.FbxImportUI()
    Options.set_editor_property('import_materials', False)
    Options.set_editor_property('import_textures', False)
    Options.static_mesh_import_data.set_editor_property('combine_meshes', True)
    Task.set_editor_property('options', Options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([Task])
    Mesh = unreal.load_asset(AssetPath)
    if not Mesh:
        raise RuntimeError('The deck crate mesh import failed')
    MeshEditor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    MeshEditor.remove_collisions(Mesh)
    MeshEditor.add_simple_collisions(Mesh, unreal.ScriptCollisionShapeType.BOX)
    unreal.EditorAssetLibrary.save_loaded_asset(Mesh)
    return Mesh


if any(isinstance(Actor, unreal.TrawlerVessel) for Actor in Actors):
    raise RuntimeError('The fishing vessel is already configured; preserve the existing map')

Vessel = ActorSubsystem.spawn_actor_from_class(GetConfiguredClass(unreal.TrawlerVessel), unreal.Vector(350, 0, -45))
Vessel.set_actor_label('FishingVessel')
for Actor in Actors:
    if not isinstance(Actor, unreal.StaticMeshActor):
        continue
    Name = Actor.get_actor_label()
    if Name == 'Ocean':
        Actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        continue
    if Name in ['FishCrate', 'CrateRim', 'CrateStack']:
        ActorSubsystem.destroy_actor(Actor)
        continue
    Component = Actor.static_mesh_component
    Component.set_mobility(unreal.ComponentMobility.MOVABLE)
    if Name in ['Hull', 'DeckSeam', 'CabinStripe', 'CabinFrontStripe', 'CabinWindow']:
        Component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    else:
        Component.set_collision_profile_name('BlockAllDynamic')
    Actor.attach_to_actor(Vessel, '', unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    Vessel.hull.ignore_actor_when_moving(Actor, True)

Steel = Material('M_Steel')
Teal = Material('M_Teal')
Orange = Material('M_Orange')
DarkSteel = Material('M_DarkSteel')
SpawnShape('ControlPedestal', (65, -160, 25), (0.8, 1.3, 0.5), Teal, Vessel)
SpawnShape('ControlConsole', (65, -160, 52), (0.9, 1.4, 0.06), DarkSteel, Vessel)

for Name, Position, Rotation, Surface in [
    ('ThrottleLever', (65, -125, 55), unreal.Rotator(yaw=-90), Orange),
    ('SteeringLever', (65, -190, 55), unreal.Rotator(), Teal),
]:
    Lever = ActorSubsystem.spawn_actor_from_class(GetConfiguredClass(unreal.PhysicalLever,Name), unreal.Vector(*Position), Rotation)
    Lever.set_actor_label(Name)
    Lever.foundation.set_material(0, DarkSteel)
    Lever.body.set_material(0, Steel)
    Lever.knob.set_material(0, Surface)
    Lever.attach_to_actor(Vessel, '', unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    Vessel.hull.ignore_actor_when_moving(Lever, True)
    if Name == 'ThrottleLever':
        Lever.set_editor_property('display_name', unreal.Text('Ход судна'))
        Vessel.set_editor_property('throttle_lever', Lever)
    else:
        Lever.set_editor_property('display_name', unreal.Text('Руль'))
        Vessel.set_editor_property('steering_lever', Lever)

Mesh = CreateCrateMesh()
for Position in [(285, 225, 23), (375, 225, 23), (465, 225, 23)]:
    Cargo = ActorSubsystem.spawn_actor_from_class(GetConfiguredClass(unreal.WorkItem), unreal.Vector(*Position))
    Cargo.set_actor_label('PhysicalDeckCrate')
    Cargo.body.set_simulate_physics(False)
    Cargo.body.set_static_mesh(Mesh)
    Cargo.set_actor_scale3d(unreal.Vector(1, 1, 1))
    Cargo.body.set_material(0, Teal)
    Cargo.body.set_mass_override_in_kg('', 8, True)
    Cargo.grip_point.set_relative_location(unreal.Vector(0, 0, 18), False, False)
    Cargo.body.set_simulate_physics(True)
    Cargo.set_editor_property('display_name', unreal.Text('Ящик'))
    Cargo.set_editor_property('item_type', 'DeckCrate')

for Position in [(1800, -1600, -50), (3400, 1200, -50), (-2000, 2300, -50)]:
    Marker = ActorSubsystem.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*Position))
    Marker.set_actor_label('NavigationBuoy')
    Marker.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cylinder'))
    Marker.static_mesh_component.set_material(0, Orange)
    Marker.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    Marker.set_actor_scale3d(unreal.Vector(0.7, 0.7, 2))

LevelSubsystem.save_current_level()
unreal.EditorLevelLibrary.set_level_viewport_camera_info(unreal.Vector(-500, -800, 450), unreal.Rotator(pitch=-20, yaw=43))
print('Fishing vessel, mounted controls, physical cargo and navigation markers saved')
