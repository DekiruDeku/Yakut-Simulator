if __name__ == '__main__':
    raise RuntimeError('This legacy generator is retired. Use BuildStationaryWorkstations.py for the active stationary minigames.')

import importlib.util
import json
import pathlib
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools

def ResetGraph(Graph):
    for Node in list(BlueprintTools.find_nodes(Graph)):
        if Node.get_class().get_name()!='K2Node_FunctionEntry':
            BlueprintTools.delete_node(Node)

ProjectDirectory=pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
Specification=importlib.util.spec_from_file_location('PrototypeUIAuthoring',ProjectDirectory/'Scripts/CreatePrototypeUI.py')
UI=importlib.util.module_from_spec(Specification)
Specification.loader.exec_module(UI)
Widget=UI.GetBlueprint('WBP_FishingConsole','/Game/Fishing/UI')
if 'Scenario' not in BlueprintTools.list_variables(Widget):
    BlueprintTools.add_object_variable(Widget,'Scenario',unreal.FishingScenario.static_class())
Surface=UI.AddWidget(Widget,'Surface',unreal.Border)
Surface.set_brush_color(unreal.LinearColor(.012,.028,.025,1))
Surface.set_padding(unreal.Margin(26))
Content=UI.AddWidget(Widget,'Content',unreal.VerticalBox,Surface)
for Name,Text,Size in [('Title','ПРОМЫСЛОВАЯ КОНСОЛЬ',30),('SonarText','',23),('ConsoleText','',26),('TaskText','',23),('StatusText','',20)]:
    Block=UI.AddText(Widget,Name,Content,Text,Size)
    Block.slot.set_padding(unreal.Margin(0,0,0,14))
    if Name=='Title':
        Block.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(.45,.83,.69,1)))
for Name,Text in [('ScanButton','Сканировать'),('SelectTargetButton','Следующее скопление')]:
    Button=UI.AddWidget(Widget,Name,unreal.Button,Content,True)
    Button.set_background_color(unreal.LinearColor(.06,.16,.13,1))
    TextWidget=UI.AddText(Widget,Name+'Text',Button,Text,23)
    TextWidget.set_editor_property('margin',unreal.Margin(12,10,12,10))
    Button.slot.set_padding(unreal.Margin(0,0,0,10))
BlueprintTools.compile_blueprint(Widget)
for Name in ['ScanButton','SelectTargetButton']:
    UI.BindButton(Widget,Name)
Queries={
    'Widget':(BlueprintTools.get_graph(Widget,'EventGraph'),['GetScenario','GetConsoleText','GetSonarText','GetTaskText','GetStatusText','GetConsoleReadout','GetSonar','GetReadout','BindEventtoOnScenarioChanged','Scan','SelectNextSchool']),
}
Discovery={}
for Owner,(Graph,Searches) in Queries.items():
    Discovery[Owner]={}
    for Search in Searches:
        NodeTypes=[str(Name) for Name in BlueprintTools.find_node_types(Graph,Search)]
        if Search=='Scan':
            NodeTypes=[Name for Name in NodeTypes if Name.endswith('|Scan')]
        if Search=='SetWidget':
            NodeTypes=[Name for Name in NodeTypes if Name=='UserInterface|SetWidget']
        Discovery[Owner][Search]={Name:str(BlueprintTools.get_node_type_pins(Graph,Name)) for Name in NodeTypes}
(ProjectDirectory/'Saved/Verification/FishingScenario').mkdir(parents=True,exist_ok=True)
(ProjectDirectory/'Saved/Verification/FishingScenario/ConsolePins.json').write_text(json.dumps(Discovery,indent=2))
unreal.EditorAssetLibrary.save_loaded_asset(Widget)
print('Fishing console designer saved; Blueprint node discovery recorded')

unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(Widget,'Scenario',True)
unreal.BlueprintEditorLibrary.set_blueprint_variable_expose_on_spawn(Widget,'Scenario',True)
BlueprintTools.compile_blueprint(Widget)
Refresh=BlueprintTools.get_graph(Widget,'RefreshReadout')
if not Refresh:
    Refresh=BlueprintTools.add_function_graph(Widget,'RefreshReadout')
ResetGraph(Refresh)
UI.DiscoverNodes(Refresh,['Variables|Default|GetScenario','Variables|WBP_FishingConsole|GetConsoleText','Variables|WBP_FishingConsole|GetSonarText','Variables|WBP_FishingConsole|GetTaskText','Variables|WBP_FishingConsole|GetStatusText','Fishing|GetConsoleReadout','Class|FishingScenario|GetSonar','Sonar|GetReadout','Scenario|GetCurrentInstruction','Class|ScenarioDirector|GetLastAction','Widget|SetText(Text)'])
UI.WriteGraph(Refresh, '''
(fn RefreshReadout ()
    (bind Scenario (Variables|Default|GetScenario))
    (Widget|SetText(Text) :self (Variables|WBP_FishingConsole|GetConsoleText) :InText (Fishing|GetConsoleReadout :self Scenario))
    (Widget|SetText(Text) :self (Variables|WBP_FishingConsole|GetSonarText) :InText (Sonar|GetReadout :self (Class|FishingScenario|GetSonar :self Scenario)))
    (Widget|SetText(Text) :self (Variables|WBP_FishingConsole|GetTaskText) :InText (Scenario|GetCurrentInstruction :self Scenario))
    (Widget|SetText(Text) :self (Variables|WBP_FishingConsole|GetStatusText) :InText (Class|ScenarioDirector|GetLastAction :self Scenario)))
''')
BlueprintTools.compile_blueprint(Widget)
Graph=BlueprintTools.get_graph(Widget,'EventGraph')
ResetGraph(Graph)
UI.DiscoverNodes(Graph,['Scenario|BindEventtoOnScenarioChanged','CallFunction|RefreshReadout','Sonar|Scan','Sonar|SelectNextSchool'])
UI.WriteGraph(Graph, '''
(event UserInterface|EventConstruct
    (Scenario|BindEventtoOnScenarioChanged :self (Variables|Default|GetScenario))
    (CallFunction|RefreshReadout))
(event OnClicked(ScanButton)
    (Sonar|Scan :self (Class|FishingScenario|GetSonar :self (Variables|Default|GetScenario))))
(event OnClicked(SelectTargetButton)
    (Sonar|SelectNextSchool :self (Class|FishingScenario|GetSonar :self (Variables|Default|GetScenario))))
''')
UI.BindRefreshFunction(Graph,'Scenario|BindEventtoOnScenarioChanged','RefreshReadout')
BlueprintTools.compile_blueprint(Widget)
unreal.EditorAssetLibrary.save_loaded_asset(Widget)

for CurrentGraph in BlueprintTools.list_graphs(Widget):
    BlueprintTools.arrange_nodes(BlueprintTools.find_nodes(CurrentGraph))
unreal.EditorAssetLibrary.save_loaded_asset(Widget)
print('Fishing console UMG compiled and saved')

Scenario=unreal.load_asset('/Game/Fishing/Blueprints/BP_FishingScenario')
Graph=BlueprintTools.get_graph(Scenario,'EventGraph')
ResetGraph(Graph)
UI.DiscoverNodes(Graph,['AddEvent|Fishing|EventInitializeConsole','UserInterface|CreateWidget','Variables|Fishing|GetConsoleDisplay','UserInterface|SetWidget','Variables|Getareferencetoself'])
UI.WriteGraph(Graph, '''
(event Fishing|EventInitializeConsole (Player)
    (bind Screen (UserInterface|CreateWidget :Class "/Game/Fishing/UI/WBP_FishingConsole.WBP_FishingConsole_C" :OwningPlayer Player))
    (UserInterface|SetWidget :self (Variables|Fishing|GetConsoleDisplay) :Widget Screen))
''')
Creation,=[Node for Node in BlueprintTools.find_nodes(Graph) if BlueprintTools.get_node_infos([Node])[0].type_id=='UserInterface|CreateWidget']
Self=BlueprintTools.create_node(Graph,'Variables|Getareferencetoself',unreal.IntPoint(-200,-180))
BlueprintTools.connect_pins(UI.FindPin(Self,'self',unreal.EdGraphPinDirection.EGPD_OUTPUT),UI.FindPin(Creation,'Scenario',unreal.EdGraphPinDirection.EGPD_INPUT))
BlueprintTools.compile_blueprint(Scenario)
BlueprintTools.arrange_nodes(BlueprintTools.find_nodes(Graph))
unreal.EditorAssetLibrary.save_loaded_asset(Scenario)
print('Fishing console owner Blueprint compiled and saved')
