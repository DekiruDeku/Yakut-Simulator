import json
import pathlib
import sys
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
import CreatePrototypeUI as UI
from BuildStationaryWorkstations import Backup, Load, InterfaceFolder, BlueprintFolder


def Reset(Graph):
    for Node in list(BlueprintTools.find_nodes(Graph)):
        if Node.get_class().get_name() not in ['K2Node_FunctionEntry', 'K2Node_ComponentBoundEvent']:
            BlueprintTools.delete_node(Node)


def CreateWorkstationConsole():
    Widget = UI.GetBlueprint('WBP_WorkstationConsole', InterfaceFolder)
    Backup(Widget)
    if 'Workstation' not in BlueprintTools.list_variables(Widget):
        BlueprintTools.add_object_variable(Widget, 'Workstation', unreal.WorkstationScenario.static_class())
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(Widget, 'Workstation', True)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_expose_on_spawn(Widget, 'Workstation', True)
    Surface = UI.AddWidget(Widget, 'Surface', unreal.Border)
    Surface.set_brush_color(unreal.LinearColor(.014, .022, .028, 1))
    Surface.set_padding(unreal.Margin(22))
    Content = UI.AddWidget(Widget, 'Content', unreal.VerticalBox, Surface)
    for Name, Size, Color in [('Title', 30, (.8, .88, .9, 1)), ('EquipmentReadout', 24, (.7, .83, .82, 1)), ('CommandText', 24, (.94, .82, .43, 1)), ('StatusText', 20, (.9, .9, .9, 1))]:
        Text = UI.AddText(Widget, Name, Content, '', Size)
        Text.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(*Color)))
        Text.slot.set_padding(unreal.Margin(0, 0, 0, 12))
    for Name, Title in [('ConfirmButton', 'Подтвердить команду'), ('RestartButton', 'Начать новый цикл')]:
        Button = UI.AddWidget(Widget, Name, unreal.Button, Content, True)
        Button.set_background_color(unreal.LinearColor(.05, .12, .16, 1))
        Label = UI.AddText(Widget, Name + 'Text', Button, Title, 22)
        Label.set_editor_property('margin', unreal.Margin(12, 10, 12, 10))
        Button.slot.set_padding(unreal.Margin(0, 6, 0, 0))
    BlueprintTools.compile_blueprint(Widget)
    for Name in ['ConfirmButton', 'RestartButton']:
        UI.BindButton(Widget, Name)
    Refresh = BlueprintTools.get_graph(Widget, 'RefreshReadout')
    if not Refresh:
        Refresh = BlueprintTools.add_function_graph(Widget, 'RefreshReadout')
    Reset(Refresh)
    UI.DiscoverNodes(Refresh, ['Variables|Default|GetWorkstation', 'Variables|WBP_WorkstationConsole|GetTitle', 'Variables|WBP_WorkstationConsole|GetEquipmentReadout', 'Variables|WBP_WorkstationConsole|GetCommandText', 'Variables|WBP_WorkstationConsole|GetStatusText', 'Class|ScenarioDirector|GetScenarioTitle', 'Workstation|GetReadout', 'Scenario|GetCurrentInstruction', 'Class|ScenarioDirector|GetLastAction', 'Widget|SetText(Text)'])
    UI.WriteGraph(Refresh, '''
(fn RefreshReadout ()
    (bind Workstation (Variables|Default|GetWorkstation))
    (Widget|SetText(Text) :self (Variables|WBP_WorkstationConsole|GetTitle) :InText (Class|ScenarioDirector|GetScenarioTitle :self Workstation))
    (Widget|SetText(Text) :self (Variables|WBP_WorkstationConsole|GetEquipmentReadout) :InText (Workstation|GetReadout :self Workstation))
    (Widget|SetText(Text) :self (Variables|WBP_WorkstationConsole|GetCommandText) :InText (Scenario|GetCurrentInstruction :self Workstation))
    (Widget|SetText(Text) :self (Variables|WBP_WorkstationConsole|GetStatusText) :InText (Class|ScenarioDirector|GetLastAction :self Workstation)))
''')
    BlueprintTools.compile_blueprint(Widget)
    Graph = BlueprintTools.get_graph(Widget, 'EventGraph')
    Reset(Graph)
    UI.DiscoverNodes(Graph, ['Scenario|BindEventtoOnScenarioChanged', 'CallFunction|RefreshReadout', 'Workstation|ConfirmCommand', 'Workstation|RestartWorkstation'])
    UI.WriteGraph(Graph, '''
(event UserInterface|EventConstruct
    (Scenario|BindEventtoOnScenarioChanged :self (Variables|Default|GetWorkstation))
    (CallFunction|RefreshReadout))
(event OnClicked(ConfirmButton)
    (Workstation|ConfirmCommand :self (Variables|Default|GetWorkstation)))
(event OnClicked(RestartButton)
    (Workstation|RestartWorkstation :self (Variables|Default|GetWorkstation)))
''')
    UI.BindRefreshFunction(Graph, 'Scenario|BindEventtoOnScenarioChanged', 'RefreshReadout')
    BlueprintTools.compile_blueprint(Widget)
    unreal.EditorAssetLibrary.save_loaded_asset(Widget, False)
    for Name in ['WorkstationScenario', 'TrawlWinchScenario', 'FishSortingScenario', 'FishCuttingScenario']:
        Folder = BlueprintFolder
        if Name == 'WorkstationScenario':
            Folder = '/Game/Base/Blueprints'
        Blueprint = Load('BP_' + Name, Folder)
        Backup(Blueprint)
        Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
        Reset(Graph)
        UI.DiscoverNodes(Graph, ['AddEvent|Workstation|EventInitializeConsole', 'UserInterface|CreateWidget', 'Variables|Workstation|GetConsoleDisplay', 'UserInterface|SetWidget', 'Variables|Getareferencetoself'])
        UI.WriteGraph(Graph, '''
(event Workstation|EventInitializeConsole (Player)
    (bind Screen (UserInterface|CreateWidget :Class "/Game/Base/UI/WBP_WorkstationConsole.WBP_WorkstationConsole_C" :OwningPlayer Player))
    (UserInterface|SetWidget :self (Variables|Workstation|GetConsoleDisplay) :Widget Screen))
''')
        Creation, = [Node for Node in BlueprintTools.find_nodes(Graph) if BlueprintTools.get_node_infos([Node])[0].type_id == 'UserInterface|CreateWidget']
        Self = BlueprintTools.create_node(Graph, 'Variables|Getareferencetoself', unreal.IntPoint(-200, -180))
        BlueprintTools.connect_pins(UI.FindPin(Self, 'self', unreal.EdGraphPinDirection.EGPD_OUTPUT), UI.FindPin(Creation, 'Workstation', unreal.EdGraphPinDirection.EGPD_INPUT))
        BlueprintTools.compile_blueprint(Blueprint)
        BlueprintTools.arrange_nodes(BlueprintTools.find_nodes(Graph))
        unreal.EditorAssetLibrary.save_loaded_asset(Blueprint, False)
    print('Workstation console UMG and explicit owner bindings saved')


if __name__ == '__main__':
    CreateWorkstationConsole()
