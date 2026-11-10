import json
import pathlib
import shutil
import sys
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as Blueprint

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
import CreatePrototypeUI as UI

AnimationPath = '/Game/Base/Hands/Animation/ABP_VirtualRealityHands'
MeshRoot = '/Game/Meshes/XRMannequins'
BackupDirectory = ProjectDirectory / 'Saved/Backups/VirtualRealityHands'
ReportDirectory = ProjectDirectory / 'Saved/Verification/VirtualRealityHands'


def Backup(Asset):
    Package = Asset.get_path_name().split('.')[0]
    Relative = pathlib.Path('Content') / Package.removeprefix('/Game/')
    Source = (ProjectDirectory / Relative).with_suffix('.uasset')
    Destination = (BackupDirectory / Relative).with_suffix('.uasset')
    if Source.exists() and not Destination.exists():
        Destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(Source, Destination)


def PrepareAnimation():
    Animation = unreal.load_asset(AnimationPath)
    if not Animation:
        Source = '/Game/XRMannequins/Meshes/ABP_MannequinsXR'
        Animation = unreal.EditorAssetLibrary.duplicate_asset(Source, AnimationPath)
    Backup(Animation)
    Animation.set_editor_property('target_skeleton', unreal.load_asset(MeshRoot + '/Meshes/SK_MannequinsXR'))
    for Node in Blueprint.find_nodes(Blueprint.get_graph(Animation, 'AnimGraph')):
        if isinstance(Node, unreal.AnimGraphNode_SequencePlayer):
            Settings = Node.get_editor_property('node')
            Sequence = Settings.get_editor_property('sequence')
            Settings.set_editor_property('sequence', unreal.load_asset(MeshRoot + '/Animations/' + Sequence.get_name()))
            Node.set_editor_property('node', Settings)
        elif isinstance(Node, unreal.AnimGraphNode_Mirror):
            Settings = Node.get_editor_property('node')
            Settings.set_editor_property('mirror_data_table', unreal.load_asset(MeshRoot + '/Animations/MDT_MannequinsXR'))
            Node.set_editor_property('node', Settings)
    Graph = Blueprint.get_graph(Animation, 'EventGraph')
    for Node in list(Blueprint.find_nodes(Graph)):
        Blueprint.delete_node(Node)
    UI.WriteGraph(Graph, '''(event EventBlueprintInitializeAnimation
        (Variables|Default|SetMirror (Components|ComponentHasTag :self (Animation|GetOwningComponent) :Tag "LeftHand")))''')
    Blueprint.compile_blueprint(Animation, True)
    unreal.EditorAssetLibrary.save_loaded_asset(Animation, False)
    print('Template hand animation graph copied and remapped to existing project assets')
    return Animation


def ConfigureVirtualRealityHands():
    Animation = unreal.load_asset(AnimationPath)
    Graph = Blueprint.get_graph(Animation, 'EventGraph')
    for Node in list(Blueprint.find_nodes(Graph)):
        Blueprint.delete_node(Node)
    UI.WriteGraph(Graph, '''
    (event EventBlueprintInitializeAnimation
        (Variables|Default|SetMirror (Components|ComponentHasTag :self (Animation|GetOwningComponent) :Tag "LeftHand")))
    (event EventBlueprintUpdateAnimation (DeltaTimeX)
        (bind Pawn (Utilities|Casting|CastToVirtualRealityPawn :Object (Animation|TryGetPawnOwner))
            (:then
                (if (Variables|Default|GetMirror)
                    (Variables|Default|SetPoseAlphaGrasp (Class|VirtualRealityPawn|GetLeftGripAmount :self Pawn))
                    (Variables|Default|SetPoseAlphaIndexCurl (Class|VirtualRealityPawn|GetLeftTriggerAmount :self Pawn))
                    (else
                        (Variables|Default|SetPoseAlphaGrasp (Class|VirtualRealityPawn|GetRightGripAmount :self Pawn))
                        (Variables|Default|SetPoseAlphaIndexCurl (Class|VirtualRealityPawn|GetRightTriggerAmount :self Pawn))))
                (Variables|Default|SetPoseAlphaPoint 0)
                (Variables|Default|SetPoseAlphaThumbUp 0)
                (Variables|Default|SetCurrentPoseAlphaPoint (Math|Interpolation|FInterpTo :Current (Variables|Default|GetCurrentPoseAlphaPoint) :Target (Variables|Default|GetPoseAlphaPoint) :DeltaTime DeltaTimeX :InterpSpeed 13))
                (Variables|Default|SetCurrentPoseAlphaThumbUp (Math|Interpolation|FInterpTo :Current (Variables|Default|GetCurrentPoseAlphaThumbUp) :Target (Variables|Default|GetPoseAlphaThumbUp) :DeltaTime DeltaTimeX :InterpSpeed 13)))
            (:CastFailed)))''')
    Blueprint.arrange_nodes(Blueprint.find_nodes(Graph))
    Blueprint.compile_blueprint(Animation, True)
    unreal.EditorAssetLibrary.save_loaded_asset(Animation, False)
    Pawn = unreal.load_asset('/Game/Base/Blueprints/BP_VirtualRealityPawn')
    Backup(Pawn)
    Defaults = unreal.get_default_object(Pawn.generated_class())
    for Hand in ['Left', 'Right']:
        Component = Defaults.get_editor_property(Hand + 'HandMesh')
        Component.set_skeletal_mesh_asset(unreal.load_asset(MeshRoot + '/Meshes/SKM_MannyXR_' + Hand.lower()))
        Component.set_anim_instance_class(Animation.generated_class())
        Component.set_editor_property('component_tags', [unreal.Name(Hand + 'Hand')])
        if Hand == 'Left':
            Component.set_relative_location(unreal.Vector(-2.98126, -3.5, 4.561753), False, False)
            Component.set_relative_rotation(unreal.Rotator(pitch=-25, yaw=-180, roll=90), False, False)
        else:
            Component.set_relative_location(unreal.Vector(-2.98126, 3.5, 4.561753), False, False)
            Component.set_relative_rotation(unreal.Rotator(pitch=25, yaw=0, roll=90), False, False)
        Component.set_relative_scale3d(unreal.Vector(1, 1, 1))
        Component.set_collision_profile_name('NoCollision')
        Component.set_editor_property('override_materials', [])
    Blueprint.compile_blueprint(Pawn, True)
    unreal.EditorAssetLibrary.save_loaded_asset(Pawn, False)
    ReportDirectory.mkdir(parents=True, exist_ok=True)
    (ReportDirectory / 'AnimationEventGraph.txt').write_text(Blueprint.read_graph_dsl(Graph), encoding='utf-8')
    unreal.EditorAssetLibrary.save_loaded_asset(Animation, False)
    print('Manny hands and template animation configured on the project VR pawn')


if __name__ == '__main__':
    ConfigureVirtualRealityHands()
