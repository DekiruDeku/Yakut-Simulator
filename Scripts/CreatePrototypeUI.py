import json
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools
from editor_toolset.toolsets.object import ObjectTools

UIFolder = '/Game/Prototype/UI'


def CallUMG(ToolName, Arguments):
    Result = unreal.ToolsetRegistry.execute_tool('UMGToolSet.UMGToolSet', ToolName, json.dumps(Arguments))
    if Result.error:
        raise RuntimeError(Result.error)
    return json.loads(Result.value)['returnValue']


def ObjectArgument(Object):
    return {'refPath': Object.get_path_name()}


def GetBlueprint(Name, Folder=UIFolder):
    Blueprint = unreal.load_asset(Folder + '/' + Name)
    if not Blueprint:
        Factory = unreal.WidgetBlueprintFactory()
        Factory.set_editor_property('parent_class', unreal.UserWidget.static_class())
        Blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(Name, Folder, unreal.WidgetBlueprint, Factory)
    return Blueprint


def AddWidget(Blueprint, Name, WidgetClass, Parent=None, Variable=False):
    WidgetPath = Blueprint.get_path_name() + ':WidgetTree.' + Name
    Widget = unreal.find_object(None, WidgetPath)
    if not Widget:
        ParentArgument = None
        if Parent:
            ParentArgument = ObjectArgument(Parent)
        Result = CallUMG('AddWidget', {'widgetBlueprint': ObjectArgument(Blueprint), 'widgetClass': ObjectArgument(WidgetClass.static_class()), 'widgetDisplayName': Name, 'parentWidget': ParentArgument, 'childIndex': -1})
        Widget = unreal.find_object(None, Result['widget']['refPath'])
    WidgetProperties = json.loads(ObjectTools.list_properties(Widget))
    ObjectTools.get_properties(Widget, list(WidgetProperties))
    Slot = Widget.get_editor_property('slot')
    if Slot:
        SlotProperties = json.loads(ObjectTools.list_properties(Slot))
        ObjectTools.get_properties(Slot, list(SlotProperties))
    if Variable:
        CallUMG('ToggleWidgetAsVariable', {'widgetBlueprint': ObjectArgument(Blueprint), 'widget': ObjectArgument(Widget), 'bIsVariable': True})
    return Widget


def AddText(Blueprint, Name, Parent, Text='', Size=20):
    Widget = AddWidget(Blueprint, Name, unreal.TextBlock, Parent, True)
    Widget.set_text(Text)
    Font = Widget.get_editor_property('font')
    Font.set_editor_property('size', Size)
    Widget.set_font(Font)
    Widget.set_auto_wrap_text(True)
    return Widget


def BindButton(Blueprint, Name):
    Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
    if 'OnClicked(' + Name + ')' not in BlueprintTools.read_graph_dsl(Graph):
        CallUMG('BindToEventProperty', {'widgetBlueprint': ObjectArgument(Blueprint), 'eventName': 'OnClicked', 'propertyName': Name, 'propertyClass': ObjectArgument(unreal.Button.static_class())})


def BuildDesigners():
    Entry = GetBlueprint('WBP_MapEntry')
    Button = AddWidget(Entry, 'SelectMapButton', unreal.Button, Variable=True)
    Button.set_background_color(unreal.LinearColor(0.14, 0.32, 0.46, 1))
    Label = AddText(Entry, 'MapName', Button)
    Label.set_editor_property('margin', unreal.Margin(18, 16, 18, 16))
    if 'MapEntry' not in BlueprintTools.list_variables(Entry):
        BlueprintTools.add_struct_variable(Entry, 'MapEntry', unreal.load_object(None, '/Script/YakutSimulator.ScenarioMapEntry'))
    BlueprintTools.compile_blueprint(Entry)
    BindButton(Entry, 'SelectMapButton')

    Menu = GetBlueprint('WBP_MainMenu')
    Canvas = AddWidget(Menu, 'MenuCanvas', unreal.CanvasPanel)
    Background = AddWidget(Menu, 'Background', unreal.Border, Canvas)
    Background.set_brush_color(unreal.LinearColor(0.018, 0.028, 0.045, 0.97))
    Background.slot.set_anchors(unreal.Anchors(unreal.Vector2D(0, 0), unreal.Vector2D(1, 1)))
    Background.slot.set_offsets(unreal.Margin(0))
    Card = AddWidget(Menu, 'Card', unreal.SizeBox, Canvas)
    Card.set_width_override(560)
    Card.slot.set_anchors(unreal.Anchors(unreal.Vector2D(0.5, 0.5), unreal.Vector2D(0.5, 0.5)))
    Card.slot.set_alignment(unreal.Vector2D(0.5, 0.5))
    Card.slot.set_auto_size(True)
    Surface = AddWidget(Menu, 'Surface', unreal.Border, Card)
    Surface.set_brush_color(unreal.LinearColor(0.045, 0.065, 0.095, 1))
    Surface.set_padding(unreal.Margin(32))
    Content = AddWidget(Menu, 'MenuContent', unreal.VerticalBox, Surface)
    Title = AddText(Menu, 'Title', Content, 'Выбор карты', 30)
    Title.slot.set_padding(unreal.Margin(0, 0, 0, 12))
    Description = AddText(Menu, 'Description', Content, 'Выберите карту.')
    Description.slot.set_padding(unreal.Margin(0, 0, 0, 24))
    AddWidget(Menu, 'MapList', unreal.VerticalBox, Content, True)
    Controls = AddWidget(Menu, 'ScenarioControls', unreal.VerticalBox, Content, True)
    for Name, Text in [('ResumeButton', 'Продолжить'), ('MainMenuButton', 'Главное меню')]:
        Button = AddWidget(Menu, Name, unreal.Button, Controls, True)
        Label = AddText(Menu, Name + 'Label', Button, Text)
        Label.set_editor_property('margin', unreal.Margin(18, 12, 18, 12))
        Button.slot.set_padding(unreal.Margin(0, 8, 0, 12))
    Error = AddText(Menu, 'ErrorMessage', Content)
    Error.set_color_and_opacity(unreal.SlateColor(specified_color=unreal.LinearColor(1, 0.45, 0.3, 1)))
    Error.slot.set_padding(unreal.Margin(0, 16, 0, 0))
    BlueprintTools.compile_blueprint(Menu)
    for Name in ['ResumeButton', 'MainMenuButton']:
        BindButton(Menu, Name)

    Panel = GetBlueprint('WBP_ScenarioPanel')
    Canvas = AddWidget(Panel, 'PanelCanvas', unreal.CanvasPanel)
    Card = AddWidget(Panel, 'PanelCard', unreal.SizeBox, Canvas)
    Card.set_width_override(480)
    Card.slot.set_position(unreal.Vector2D(24, 24))
    Card.slot.set_auto_size(True)
    Surface = AddWidget(Panel, 'Surface', unreal.Border, Card)
    Surface.set_brush_color(unreal.LinearColor(0.025, 0.04, 0.065, 0.94))
    Surface.set_padding(unreal.Margin(22))
    Content = AddWidget(Panel, 'PanelContent', unreal.VerticalBox, Surface)
    for Name, Size in [('Heading', 24), ('Instruction', 20), ('Progress', 20), ('Feedback', 20)]:
        Text = AddText(Panel, Name, Content, Size=Size)
        Text.slot.set_padding(unreal.Margin(0, 0, 0, 12))
    for Blueprint in [Entry, Menu, Panel]:
        BlueprintTools.compile_blueprint(Blueprint)
        unreal.EditorAssetLibrary.save_loaded_asset(Blueprint)
    return Entry, Menu, Panel






def DiscoverNodes(Graph, NodeTypes):
    for NodeType in NodeTypes:
        BlueprintTools.find_node_types(Graph, NodeType.split('|')[-1])
        BlueprintTools.get_node_type_pins(Graph, NodeType)


def WriteGraph(Graph, Code):
    import toolset_registry
    from editor_toolset.toolsets import blueprint_dsl
    with toolset_registry.tool_raising_exceptions():
        blueprint_dsl.Transpiler(
            Graph, BlueprintTools.create_node, BlueprintTools.connect_pins,
            BlueprintTools._get_node_info, BlueprintTools.set_pin_value,
            lambda CurrentGraph: BlueprintTools.find_nodes(CurrentGraph),
            delete_node_fn=BlueprintTools.delete_node,
            find_node_types_fn=lambda Filter: BlueprintTools.find_node_types(Graph, Filter),
        ).transpile(Code)


def FindPin(Node, Name, Direction):
    Information, = BlueprintTools.get_node_infos([Node])
    Pins = Information.input_pins
    if Direction == unreal.EdGraphPinDirection.EGPD_OUTPUT:
        Pins = Information.output_pins
    return next(Pin.pin_id for Pin in Pins if Pin.name == Name)


def BindRefreshFunction(Graph, BindingNodeType, FunctionName):
    DiscoverNodes(Graph, ['EventDispatchers|CreateEvent', BindingNodeType])
    Binding, = [Node for Node in BlueprintTools.find_nodes(Graph)
        if isinstance(Node, unreal.K2Node_AddDelegate)
        and ''.join(Node.get_node_title().split()) == BindingNodeType.rsplit('|', 1)[-1]]
    Event = BlueprintTools.create_node(Graph, 'EventDispatchers|CreateEvent', unreal.IntPoint(320, -180))
    BlueprintTools.connect_pins(FindPin(Event, 'OutputDelegate', unreal.EdGraphPinDirection.EGPD_OUTPUT), FindPin(Binding, 'Delegate', unreal.EdGraphPinDirection.EGPD_INPUT))
    BlueprintTools.set_create_event_function(Event, FunctionName)


def BuildEntryGraph(Blueprint):
    Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
    DiscoverNodes(Graph, ['Variables|Default|GetMapEntry', 'Variables|WBP_MapEntry|GetMapName', 'Widget|SetText(Text)', 'Utilities|Struct|BreakScenarioMapEntry', 'Widget|GetOwningPlayer', 'Utilities|Casting|CastToPrototypePlayerController', 'Menu|OpenScenarioMap'])
    WriteGraph(Graph, '''
(event UserInterface|EventConstruct
    (bind (DisplayName Destination) (Utilities|Struct|BreakScenarioMapEntry :ScenarioMapEntry (Variables|Default|GetMapEntry)))
    (Widget|SetText(Text) :self (Variables|WBP_MapEntry|GetMapName) :InText DisplayName))
(event OnClicked(SelectMapButton)
    (bind Player (Utilities|Casting|CastToPrototypePlayerController :Object (Widget|GetOwningPlayer))
        (:then
            (bind (DisplayName Destination) (Utilities|Struct|BreakScenarioMapEntry :ScenarioMapEntry (Variables|Default|GetMapEntry)))
            (Menu|OpenScenarioMap :self Player :Map Destination))
        (:CastFailed)))
''')
    BlueprintTools.compile_blueprint(Blueprint)


def BuildMenuGraph(Blueprint):
    if 'Player' not in BlueprintTools.list_variables(Blueprint):
        BlueprintTools.add_object_variable(Blueprint, 'Player', unreal.PrototypePlayerController.static_class())
    RefreshGraph = BlueprintTools.add_function_graph(Blueprint, 'RefreshMenuError')
    DiscoverNodes(RefreshGraph, ['Variables|Default|GetPlayer', 'Variables|WBP_MainMenu|GetErrorMessage', 'Class|PrototypePlayerController|GetMenuError', 'Widget|SetText(Text)'])
    WriteGraph(RefreshGraph, '''
(fn RefreshMenuError ()
    (Widget|SetText(Text) :self (Variables|WBP_MainMenu|GetErrorMessage)
        :InText (Class|PrototypePlayerController|GetMenuError :self (Variables|Default|GetPlayer))))
''')
    BlueprintTools.compile_blueprint(Blueprint)
    Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
    DiscoverNodes(Graph, ['Widget|GetOwningPlayer', 'Utilities|Casting|CastToPrototypePlayerController', 'Variables|Default|SetPlayer', 'Variables|Default|GetPlayer', 'Menu|BindEventtoOnMenuErrorChanged', 'Menu|IsMainMenuMap', 'Widget|SetVisibility', 'Variables|WBP_MainMenu|GetScenarioControls', 'Game|GetGameMode', 'Utilities|Casting|CastToPrototypeGameMode', 'Class|PrototypeGameMode|GetScenarioMaps', 'UserInterface|CreateWidget', 'Class|WBPMapEntry|SetMapEntry', 'Panel|AddChildtoVerticalBox', 'Variables|WBP_MainMenu|GetMapList', 'Menu|ToggleMenu', 'Menu|ReturntoMainMenu', 'CallFunction|RefreshMenuError', 'Layout|VerticalBoxSlot|SetPadding'])
    WriteGraph(Graph, '''
(event UserInterface|EventOnInitialized
    (bind Controller (Utilities|Casting|CastToPrototypePlayerController :Object (Widget|GetOwningPlayer))
        (:then
            (Variables|Default|SetPlayer :Player Controller)
            (Menu|BindEventtoOnMenuErrorChanged :self Controller)
            (CallFunction|RefreshMenuError)
            (if (Menu|IsMainMenuMap :self Controller)
                (Widget|SetVisibility :self (Variables|WBP_MainMenu|GetScenarioControls) :InVisibility "Collapsed")
                (else
                    (Widget|SetVisibility :self (Variables|WBP_MainMenu|GetScenarioControls) :InVisibility "Visible")))
            (bind GameMode (Utilities|Casting|CastToPrototypeGameMode :Object (Game|GetGameMode))
                (:then
                    (for MapEntry (Class|PrototypeGameMode|GetScenarioMaps :self GameMode)
                        (bind EntryWidget (UserInterface|CreateWidget :Class "/Game/Prototype/UI/WBP_MapEntry.WBP_MapEntry_C" :OwningPlayer Controller))
                        (Class|WBPMapEntry|SetMapEntry :self EntryWidget :MapEntry MapEntry)
                        (bind EntrySlot (Panel|AddChildtoVerticalBox :self (Variables|WBP_MainMenu|GetMapList) :Content EntryWidget))
                        (Layout|VerticalBoxSlot|SetPadding :self EntrySlot :InPadding "(Left=0,Top=0,Right=0,Bottom=12)")))
                (:CastFailed)))
        (:CastFailed)))
(event OnClicked(ResumeButton)
    (Menu|ToggleMenu :self (Variables|Default|GetPlayer)))
(event OnClicked(MainMenuButton)
    (Menu|ReturntoMainMenu :self (Variables|Default|GetPlayer)))
''')
    BindRefreshFunction(Graph, 'Menu|BindEventtoOnMenuErrorChanged', 'RefreshMenuError')
    BlueprintTools.compile_blueprint(Blueprint)


def BuildPanelGraph(Blueprint):
    if 'Director' not in BlueprintTools.list_variables(Blueprint):
        BlueprintTools.add_object_variable(Blueprint, 'Director', unreal.ScenarioDirector.static_class())
    RefreshGraph = BlueprintTools.add_function_graph(Blueprint, 'RefreshScenario')
    DiscoverNodes(RefreshGraph, ['Variables|Default|GetDirector', 'Variables|WBP_ScenarioPanel|GetHeading', 'Variables|WBP_ScenarioPanel|GetInstruction', 'Variables|WBP_ScenarioPanel|GetFeedback', 'Variables|WBP_ScenarioPanel|GetProgress', 'Class|ScenarioDirector|GetScenarioTitle', 'Class|ScenarioDirector|GetLastAction', 'Class|ScenarioDirector|GetSteps', 'Scenario|GetCurrentInstruction', 'Scenario|GetCompletedStepCount', 'Widget|SetText(Text)', 'Utilities|Array|Length', 'Utilities|Text|ToText(String)', 'Utilities|String|Append'])
    WriteGraph(RefreshGraph, '''
(fn RefreshScenario ()
    (bind Director (Variables|Default|GetDirector))
    (Widget|SetText(Text) :self (Variables|WBP_ScenarioPanel|GetHeading) :InText (Class|ScenarioDirector|GetScenarioTitle :self Director))
    (Widget|SetText(Text) :self (Variables|WBP_ScenarioPanel|GetInstruction) :InText (Scenario|GetCurrentInstruction :self Director))
    (Widget|SetText(Text) :self (Variables|WBP_ScenarioPanel|GetProgress)
        :InText (Utilities|Text|ToText(String) :InString
            (Utilities|String|Append
                :A (Utilities|String|Append :A "Выполнено шагов: " :B (Scenario|GetCompletedStepCount :self Director))
                :B (Utilities|String|Append :A " из " :B (Utilities|Array|Length :TargetArray (Class|ScenarioDirector|GetSteps :self Director))))))
    (Widget|SetText(Text) :self (Variables|WBP_ScenarioPanel|GetFeedback) :InText (Class|ScenarioDirector|GetLastAction :self Director)))
''')
    BlueprintTools.compile_blueprint(Blueprint)
    Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
    DiscoverNodes(Graph, ['Widget|GetOwningPlayer', 'Utilities|Casting|CastToPrototypePlayerController', 'Class|PrototypePlayerController|GetScenarioDirector', 'Variables|Default|SetDirector', 'Scenario|BindEventtoOnScenarioChanged', 'CallFunction|RefreshScenario'])
    WriteGraph(Graph, '''
(event UserInterface|EventOnInitialized
    (bind Controller (Utilities|Casting|CastToPrototypePlayerController :Object (Widget|GetOwningPlayer))
        (:then
            (bind Director (Class|PrototypePlayerController|GetScenarioDirector :self Controller))
            (Variables|Default|SetDirector :Director Director)
            (Scenario|BindEventtoOnScenarioChanged :self Director)
            (CallFunction|RefreshScenario))
        (:CastFailed)))
''')
    BindRefreshFunction(Graph, 'Scenario|BindEventtoOnScenarioChanged', 'RefreshScenario')
    BlueprintTools.compile_blueprint(Blueprint)


def BuildPrototypeInterface():
    EntryBlueprint, MainMenuBlueprint, ScenarioPanelBlueprint = BuildDesigners()
    BuildEntryGraph(EntryBlueprint)
    BuildMenuGraph(MainMenuBlueprint)
    BuildPanelGraph(ScenarioPanelBlueprint)
    for Blueprint in [EntryBlueprint, MainMenuBlueprint, ScenarioPanelBlueprint]:
        for Graph in BlueprintTools.list_graphs(Blueprint):
            BlueprintTools.arrange_nodes(BlueprintTools.find_nodes(Graph))
        unreal.EditorAssetLibrary.save_loaded_asset(Blueprint)
    print('UMG graphs compiled and saved')


if __name__ == '__main__':
    BuildPrototypeInterface()




