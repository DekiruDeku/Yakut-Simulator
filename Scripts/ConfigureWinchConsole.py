import json
import math
import pathlib
import sys
import unreal
from editor_toolset.toolsets.actor import ActorTools
from editor_toolset.toolsets.blueprint import BlueprintTools as Blueprint

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
import CreatePrototypeUI as UI
from ImportWinchConsole import Backup, RootPath

BlueprintFolder = '/Game/Minigames/Fishing/Workstations/Blueprints'
ReportDirectory = ProjectDirectory / 'Saved/Verification/WinchConsole'
Actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
Levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def Save(Asset):
    Blueprint.compile_blueprint(Asset, True)
    unreal.EditorAssetLibrary.save_loaded_asset(Asset, False)


def Reset(Graph):
    for Node in list(Blueprint.find_nodes(Graph)):
        if not isinstance(Node, (unreal.K2Node_FunctionEntry, unreal.K2Node_ComponentBoundEvent)):
            Blueprint.delete_node(Node)


def Write(Graph, Code):
    UI.WriteGraph(Graph, Code)
    Blueprint.arrange_nodes(Blueprint.find_nodes(Graph))


def Mesh(Name):
    return unreal.load_asset(RootPath + '/Meshes/' + Name)


def CreateControl(Name, Kind):
    Asset = unreal.load_asset(BlueprintFolder + '/' + Name)
    if not Asset:
        Parent = unreal.load_asset(BlueprintFolder + '/BP_WorkstationLever').generated_class()
        Asset = Blueprint.create(BlueprintFolder, Name, Parent)
    Backup(Asset)
    Defaults = unreal.get_default_object(Asset.generated_class())
    for Component in [Defaults.body, Defaults.foundation, Defaults.knob]:
        Component.set_editor_property('override_materials', [])
        Component.set_relative_scale3d(unreal.Vector(1, 1, 1))
        Component.set_relative_rotation(unreal.Rotator(), False, False)
        Component.set_relative_location(unreal.Vector(), False, False)
    Defaults.use_indicator.set_intensity(0)
    Defaults.hinge.set_relative_location(unreal.Vector(), False, False)
    if Kind == 'Drum':
        Defaults.foundation.set_static_mesh(Mesh('SM_WinchJoystickMount'))
        Defaults.body.set_static_mesh(Mesh('SM_WinchJoystick'))
        Defaults.body.set_relative_location(unreal.Vector(0, 0, 5), False, False)
        Defaults.hinge.set_relative_location(unreal.Vector(0, 0, 5), False, False)
        Defaults.grip_point.set_relative_location(unreal.Vector(0, 0, 14.2), False, False)
        Defaults.knob.set_static_mesh(None)
        Defaults.set_editor_property('MaximumAngle', 35)
    elif Kind == 'Tension':
        Defaults.foundation.set_static_mesh(Mesh('SM_WinchRotaryMount'))
        Defaults.body.set_static_mesh(Mesh('SM_WinchRotaryKnob'))
        Defaults.grip_point.set_relative_location(unreal.Vector(3, 0, 0), False, False)
        Defaults.knob.set_static_mesh(None)
        Defaults.set_editor_property('MaximumAngle', 40)
    else:
        Defaults.foundation.set_static_mesh(Mesh('SM_WinchButtonMount'))
        Defaults.body.set_static_mesh(Mesh('SM_WinchButtonStem'))
        Cap = 'SM_WinchEmergencyCap'
        if Kind == 'Confirm':
            Cap = 'SM_WinchConfirmCap'
        Defaults.knob.set_static_mesh(Mesh(Cap))
        Defaults.knob.set_relative_location(unreal.Vector(3.3, 0, 0), False, False)
        Defaults.grip_point.set_relative_location(unreal.Vector(3.3, 0, 0), False, False)
        Defaults.set_editor_property('MaximumAngle', 5)
        Graph = Blueprint.get_graph(Asset, 'EventGraph')
        Reset(Graph)
        Write(Graph, '''
        (event Item|EventReceiveUseStarted
            (Transformation|SetRelativeLocation :self (Variables|Lever|GetKnob) :NewLocation "(X=2.8,Y=0,Z=0)"))
        (event Item|EventReceiveUseEnded
            (Transformation|SetRelativeLocation :self (Variables|Lever|GetKnob) :NewLocation "(X=3.3,Y=0,Z=0)"))''')
    Save(Asset)
    return Asset


def CreateReadout():
    Widget = UI.GetBlueprint('WBP_WinchReadout', RootPath + '/UI')
    Backup(Widget)
    if 'Workstation' not in Blueprint.list_variables(Widget):
        Blueprint.add_object_variable(Widget, 'Workstation', unreal.TrawlWinchScenario.static_class())
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(Widget, 'Workstation', True)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_expose_on_spawn(Widget, 'Workstation', True)
    Surface = UI.AddWidget(Widget, 'Surface', unreal.Border)
    Surface.set_brush_color(unreal.LinearColor(.006, .015, .014, 1))
    Surface.set_padding(unreal.Margin(20, 10, 20, 10))
    Content = UI.AddWidget(Widget, 'ReadoutContent', unreal.VerticalBox, Surface)
    Metrics = UI.AddWidget(Widget, 'Metrics', unreal.HorizontalBox, Content)
    for Hand, Title in [('Port', 'ЛЕВЫЙ'), ('Starboard', 'ПРАВЫЙ')]:
        Group = UI.AddWidget(Widget, Hand + 'Group', unreal.VerticalBox, Metrics)
        Group.slot.set_size(unreal.SlateChildSize(value=1, size_rule=unreal.SlateSizeRule.FILL))
        UI.AddText(Widget, Hand + 'Title', Group, Title, 22)
        Row = UI.AddWidget(Widget, Hand + 'Metrics', unreal.HorizontalBox, Group)
        for Metric, Unit in [('Length', 'м'), ('Tension', 'кН'), ('Speed', 'м/с')]:
            Value = UI.AddText(Widget, Hand + Metric, Row, '0.0', 36)
            Value.slot.set_padding(unreal.Margin(0, 0, 12, 0))
            Label = UI.AddText(Widget, Hand + Metric + 'Unit', Row, Unit, 22)
            Label.slot.set_padding(unreal.Margin(0, 10, 26, 0))
    CommandRow = UI.AddWidget(Widget, 'CommandRow', unreal.HorizontalBox, Content)
    CommandRow.slot.set_padding(unreal.Margin(0, 8, 0, 0))
    Command = UI.AddText(Widget, 'CommandText', CommandRow, '', 23)
    Command.slot.set_size(unreal.SlateChildSize(value=1, size_rule=unreal.SlateSizeRule.FILL))
    Restart = UI.AddWidget(Widget, 'RestartButton', unreal.Button, CommandRow, True)
    UI.AddText(Widget, 'RestartLabel', Restart, 'Новый цикл', 22)
    UI.AddText(Widget, 'StatusText', Content, '', 21)
    Blueprint.compile_blueprint(Widget)
    UI.BindButton(Widget, 'RestartButton')
    Refresh = Blueprint.get_graph(Widget, 'RefreshReadout')
    if not Refresh:
        Refresh = Blueprint.add_function_graph(Widget, 'RefreshReadout')
    Reset(Refresh)
    Statements = ['(fn RefreshReadout ()', '(bind Workstation (Variables|Default|GetWorkstation))']
    for Hand in ['Port', 'Starboard']:
        Statements.append('(bind ' + Hand + ' (Class|TrawlWinchScenario|Get' + Hand + 'Winch :self Workstation))')
        for Metric, Property in [('Length', 'CableLength'), ('Tension', 'Tension'), ('Speed', 'DrumSpeed')]:
            Expression = '(Class|FishingWinch|Get' + Property + ' :self ' + Hand + ')'
            if Metric != 'Tension':
                Expression = '(/ ' + Expression + ' 100.0)'
            Statements.append('(Widget|SetText(Text) :self (Variables|WBP_WinchReadout|Get' + Hand + Metric + ') :InText (Utilities|Text|ToText(Float) :Value ' + Expression + ' :MaximumFractionalDigits 1))')
    Statements.extend(['(Widget|SetText(Text) :self (Variables|WBP_WinchReadout|GetCommandText) :InText (Scenario|GetCurrentInstruction :self Workstation))', '(Widget|SetText(Text) :self (Variables|WBP_WinchReadout|GetStatusText) :InText (Class|ScenarioDirector|GetLastAction :self Workstation)))'])
    Write(Refresh, '\n'.join(Statements))
    Blueprint.compile_blueprint(Widget)
    Graph = Blueprint.get_graph(Widget, 'EventGraph')
    Reset(Graph)
    Write(Graph, '''
    (event UserInterface|EventConstruct
        (Scenario|BindEventtoOnScenarioChanged :self (Variables|Default|GetWorkstation))
        (CallFunction|RefreshReadout))
    (event OnClicked(RestartButton)
        (Workstation|RestartWorkstation :self (Variables|Default|GetWorkstation)))''')
    UI.BindRefreshFunction(Graph, 'Scenario|BindEventtoOnScenarioChanged', 'RefreshReadout')
    Save(Widget)
    return Widget


def AddComponent(Asset, Name, ComponentClass, Parent=None):
    Component = unreal.find_object(None, Asset.generated_class().get_path_name() + ':' + Name + '_GEN_VARIABLE')
    if not Component:
        Owner = Asset
        if Parent:
            Owner = Parent
        Component = ActorTools.add_component(Owner, ComponentClass.static_class(), Name)
    return Component


def ConfigureScenario(Widget):
    Asset = unreal.load_asset(BlueprintFolder + '/BP_TrawlWinchScenario')
    Backup(Asset)
    for Hand, Lateral in [('Port', -44.5), ('Starboard', 44.5)]:
        Pivot = AddComponent(Asset, Hand + 'GaugePivot', unreal.SceneComponent)
        Pivot.set_relative_location(unreal.Vector(95.5, Lateral, 116.26), False, False)
        Pivot.set_relative_rotation(unreal.Rotator(yaw=90, roll=17.744), False, False)
        Needle = AddComponent(Asset, Hand + 'GaugeNeedle', unreal.StaticMeshComponent, Pivot)
        Needle.set_static_mesh(Mesh('SM_WinchGaugeNeedle'))
        Needle.set_collision_profile_name('NoCollision')
        Needle.set_relative_location(unreal.Vector(), False, False)
        Needle.set_relative_rotation(unreal.Rotator(yaw=50), False, False)
    Refresh = Blueprint.get_graph(Asset, 'RefreshInstruments')
    if not Refresh:
        Refresh = Blueprint.add_function_graph(Asset, 'RefreshInstruments')
    Reset(Refresh)
    Write(Refresh, '''(fn RefreshInstruments ()
        (Transformation|SetRelativeRotation :self (Variables|Default|GetPortGaugeNeedle)
            :NewRotation (Math|Rotator|MakeRotator :Yaw (+ 50 (* (Math|Float|Clamp(Float) :Value (Class|FishingWinch|GetTension :self (Variables|Equipment|GetPortWinch)) :Min 0 :Max 100) 2.6))))
        (Transformation|SetRelativeRotation :self (Variables|Default|GetStarboardGaugeNeedle)
            :NewRotation (Math|Rotator|MakeRotator :Yaw (+ 50 (* (Math|Float|Clamp(Float) :Value (Class|FishingWinch|GetTension :self (Variables|Equipment|GetStarboardWinch)) :Min 0 :Max 100) 2.6)))))''')
    Blueprint.compile_blueprint(Asset)
    Graph = Blueprint.get_graph(Asset, 'EventGraph')
    Reset(Graph)
    Write(Graph, f'''(event Workstation|EventInitializeConsole (Player)
        (bind Screen (UserInterface|CreateWidget :Class "{Widget.generated_class().get_path_name()}" :OwningPlayer Player))
        (UserInterface|SetWidget :self (Variables|Workstation|GetConsoleDisplay) :Widget Screen)
        (Scenario|BindEventtoOnScenarioChanged)
        (CallFunction|RefreshInstruments))''')
    Creation = next(Node for Node in Blueprint.find_nodes(Graph) if Blueprint.get_node_infos([Node])[0].type_id == 'UserInterface|CreateWidget')
    Self = Blueprint.create_node(Graph, 'Variables|Getareferencetoself', unreal.IntPoint(-250, -200))
    Blueprint.connect_pins(UI.FindPin(Self, 'self', unreal.EdGraphPinDirection.EGPD_OUTPUT), UI.FindPin(Creation, 'Workstation', unreal.EdGraphPinDirection.EGPD_INPUT))
    UI.BindRefreshFunction(Graph, 'Scenario|BindEventtoOnScenarioChanged', 'RefreshInstruments')
    Save(Asset)
    return Asset


def ConfigureWinchConsole():
    Controls = {Kind: CreateControl('BP_Winch' + Kind + 'Control', Kind) for Kind in ['Drum', 'Tension', 'Emergency', 'Confirm']}
    Widget = CreateReadout()
    ScenarioBlueprint = ConfigureScenario(Widget)
    if not Levels.load_level('/Game/Maps/L_WinchStation'):
        raise RuntimeError('Could not load the winch workstation map')
    World = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    Backup(World)
    Existing = {Actor.get_actor_label(): Actor for Actor in Actors.get_all_level_actors()}
    Scenario = Existing['WinchWorkstation']
    Housing = Existing['WinchConsoleHousing']
    Housing.set_actor_location(unreal.Vector(70, 0, 0), False, False)
    Housing.set_actor_rotation(unreal.Rotator(yaw=90), False)
    Housing.set_actor_scale3d(unreal.Vector(1, 1, 1))
    Housing.static_mesh_component.set_static_mesh(Mesh('SM_WinchControlConsole'))
    Housing.static_mesh_component.set_editor_property('override_materials', [])
    Specifications = [('PortDrumControl', 'Drum', -.27, 0), ('StarboardDrumControl', 'Drum', .27, 0), ('PortTensionControl', 'Tension', -.515, .165), ('StarboardTensionControl', 'Tension', .515, .165), ('EmergencyStop', 'Emergency', 0, -.255), ('CommandConfirmation', 'Confirm', 0, .015)]
    NewControls = {}
    for Name, Kind, Horizontal, Depth in Specifications:
        Rotation = unreal.Rotator(yaw=90, roll=17.744)
        if Kind != 'Drum':
            Rotation = unreal.Rotator(pitch=107.744)
        Actor = Existing[Name]
        if Actor.get_class() != Controls[Kind].generated_class():
            Actor = Actors.spawn_actor_from_class(Controls[Kind].generated_class(), unreal.Vector(70 - Depth * 100, Horizontal * 100, 104 - .32 * Depth * 100 + .8), Rotation)
            Actor.set_actor_label(Name + '_Replacement')
        else:
            Actor.set_actor_location(unreal.Vector(70 - Depth * 100, Horizontal * 100, 104 - .32 * Depth * 100 + .8), False, False)
            Actor.set_actor_rotation(Rotation, False)
        Actor.set_editor_property('DisplayName', Existing[Name].get_editor_property('DisplayName'))
        NewControls[Name] = Actor
    for Hand in ['Port', 'Starboard']:
        Scenario.get_editor_property(Hand + 'Winch').set_editor_property('ControlLever', NewControls[Hand + 'DrumControl'])
        Scenario.set_editor_property(Hand + 'TensionControl', NewControls[Hand + 'TensionControl'])
    Scenario.set_editor_property('EmergencyControl', NewControls['EmergencyStop'])
    Scenario.set_editor_property('CommandControl', NewControls['CommandConfirmation'])
    for Name, Actor in NewControls.items():
        if Existing[Name] != Actor:
            Actors.destroy_actor(Existing[Name])
        Actor.set_actor_label(Name)
    Removed = []
    for Actor in Actors.get_all_level_actors():
        for Component in Actor.get_components_by_class(unreal.TextRenderComponent):
            Removed.append(Actor.get_actor_label())
        if isinstance(Actor, unreal.TextRenderActor):
            Actors.destroy_actor(Actor)
    Display = Scenario.console_display
    Display.set_draw_size(unreal.Vector2D(1400, 240))
    Display.set_relative_location(unreal.Vector(41.3, 0, 97.42), False, False)
    Display.set_relative_rotation(unreal.Rotator(pitch=72.256, yaw=180), False, False)
    Display.set_relative_scale3d(unreal.Vector(.06, .06, .06))
    Display.set_editor_property('tick_when_offscreen', False)
    for Asset in [Widget, ScenarioBlueprint] + list(Controls.values()):
        Save(Asset)
    Levels.save_current_level()
    ReportDirectory.mkdir(parents=True, exist_ok=True)
    (ReportDirectory / 'Authoring.json').write_text(json.dumps({'RemovedTextRender': Removed, 'Controls': {Name: Actor.get_class().get_path_name() for Name, Actor in NewControls.items()}, 'Map': World.get_path_name(), 'GameplayExecuted': False}, ensure_ascii=False, indent=2), encoding='utf-8')
    print('Winch control console configured and saved without gameplay execution')


if __name__ == '__main__':
    ConfigureWinchConsole()
