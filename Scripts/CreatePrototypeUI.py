import json
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools
from editor_toolset.toolsets.object import ObjectTools

UIFolder = '/Game/Base/UI'


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
    if WidgetClass == unreal.Button:
        WidgetClass = unreal.load_asset(UIFolder + '/WBP_MainButton').generated_class()
    elif WidgetClass == unreal.TextBlock:
        WidgetClass = unreal.CommonTextBlock
    Class = WidgetClass
    if not isinstance(Class, unreal.Class):
        Class = WidgetClass.static_class()
    WidgetPath = Blueprint.get_path_name() + ':WidgetTree.' + Name
    Widget = unreal.find_object(None, WidgetPath)
    if not Widget:
        ParentArgument = None
        if Parent:
            ParentArgument = ObjectArgument(Parent)
        if isinstance(Parent, unreal.CommonButtonBase):
            Result = CallUMG('SetNamedSlotContent', {'widgetBlueprint': ObjectArgument(Blueprint), 'hostWidget': ParentArgument, 'slotName': 'ButtonContent', 'widgetClass': ObjectArgument(Class), 'widgetName': Name})
        else:
            Result = CallUMG('AddWidget', {'widgetBlueprint': ObjectArgument(Blueprint), 'widgetClass': ObjectArgument(Class), 'widgetDisplayName': Name, 'parentWidget': ParentArgument, 'childIndex': -1})
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
    from ConfigureCommonUI import ApplyTextStyle
    Widget = AddWidget(Blueprint, Name, unreal.TextBlock, Parent, True)
    Widget.set_text(Text)
    ApplyTextStyle(Blueprint, Widget, Size)
    Widget.set_auto_wrap_text(not Name.endswith('Unit'))
    return Widget


def BindButton(Blueprint, Name):
    Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
    if 'OnClicked(' + Name + ')' not in BlueprintTools.read_graph_dsl(Graph):
        Widget = unreal.find_object(None, Blueprint.get_path_name() + ':WidgetTree.' + Name)
        CallUMG('BindToEventProperty', {'widgetBlueprint': ObjectArgument(Blueprint), 'eventName': 'OnClicked', 'propertyName': Name, 'propertyClass': ObjectArgument(Widget.get_class())})



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
