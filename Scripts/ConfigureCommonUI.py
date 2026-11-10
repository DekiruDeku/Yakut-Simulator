import json
import pathlib
import re
import shutil
import sys
import unreal
import toolset_registry
from editor_toolset.toolsets.blueprint import BlueprintTools
from editor_toolset.toolsets.object import ObjectTools

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
import CreatePrototypeUI as UI

StyleFolder = '/Game/Base/UI/Styles'
ButtonPath = '/Game/Base/UI/WBP_MainButton'
TextDefinitions = {
    'Body': (20, (1, 1, 1, 1)),
    'Caption': (21, (1, 1, 1, 1)),
    'Label': (22, (1, 1, 1, 1)),
    'ControlLabel': (26, (1, 1, 1, 1)),
    'Section': (28, (1, 1, 1, 1)),
    'Title': (30, (.8, .88, .9, 1)),
    'SettingsTitle': (38, (1, 1, 1, 1)),
    'Equipment': (24, (.7, .83, .82, 1)),
    'Command': (24, (.94, .82, .43, 1)),
    'CompactCommand': (23, (.94, .82, .43, 1)),
    'Readout': (36, (.45, .85, .65, 1)),
    'Button': (22, (1, 1, 1, 1)),
}


def TextStyle(Role):
    return unreal.load_class(None, StyleFolder + '/BPS_Text_' + Role + '.BPS_Text_' + Role + '_C')


def GetTextRole(Blueprint, Name, Size=20):
    if Name in ['MapName', 'RestartLabel', 'CloseSettingsText'] or Name.endswith(('ButtonText', 'ButtonLabel')):
        return 'Button'
    if Name == 'SettingsTitle':
        return 'SettingsTitle'
    if Name == 'Title':
        return 'Title'
    if Name == 'AudioTitle':
        return 'Section'
    if Name in ['VolumeLabel', 'HeightLabel']:
        return 'ControlLabel'
    if Name == 'EquipmentReadout':
        return 'Equipment'
    if Name == 'CommandText':
        if Blueprint.get_name() == 'WBP_WinchReadout':
            return 'CompactCommand'
        return 'Command'
    if Name.startswith(('Port', 'Starboard')):
        if Name.endswith(('Length', 'Tension', 'Speed')):
            return 'Readout'
        return 'Label'
    if Name in ['HeightSaveStatus', 'LocalFloorTitle', 'LocalFloorReadout', 'SettingsHint'] or Size == 21:
        return 'Caption'
    return 'Body'


def Backup(Asset):
    Package = Asset.get_path_name().split('.')[0]
    Source = ProjectDirectory / ('Content' + Package.removeprefix('/Game') + '.uasset')
    Destination = ProjectDirectory / 'Saved/Backups/CommonUI' / Source.relative_to(ProjectDirectory)
    if Source.exists() and not Destination.exists():
        Destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(Source, Destination)


def InspectProperties(Object):
    Properties = json.loads(ObjectTools.list_properties(Object))
    ObjectTools.get_properties(Object, list(Properties))


def Save(Blueprint):
    with toolset_registry.tool_raising_exceptions():
        BlueprintTools.compile_blueprint(Blueprint, True)
    Package = Blueprint.get_path_name().split('.')[0]
    FilePath = ProjectDirectory / ('Content' + Package.removeprefix('/Game') + '.uasset')
    if FilePath.exists():
        FilePath.chmod(0o666)
    if not unreal.EditorAssetLibrary.save_loaded_asset(Blueprint, False):
        raise RuntimeError('Failed to save ' + Blueprint.get_path_name())


def CreateStyles():
    for Role, Definition in TextDefinitions.items():
        Name = 'BPS_Text_' + Role
        Asset = unreal.load_asset(StyleFolder + '/' + Name)
        if Asset:
            continue
        Asset = BlueprintTools.create(StyleFolder, Name, unreal.CommonTextStyle.static_class())
        Defaults = unreal.get_default_object(Asset.generated_class())
        InspectProperties(Defaults)
        Font = unreal.get_default_object(unreal.TextBlock).get_editor_property('font')
        Font.set_editor_property('size', Definition[0])
        Defaults.set_editor_property('font', Font)
        Defaults.set_editor_property('color', unreal.LinearColor(*Definition[1]))
        Defaults.set_editor_property('margin', unreal.Margin(0))
        Defaults.set_editor_property('uses_drop_shadow', False)
        Save(Asset)
    Asset = unreal.load_asset(StyleFolder + '/BPS_Button_Default')
    if Asset:
        return
    Asset = BlueprintTools.create(StyleFolder, 'BPS_Button_Default', unreal.CommonButtonStyle.static_class())
    Defaults = unreal.get_default_object(Asset.generated_class())
    InspectProperties(Defaults)
    BaseStyle = unreal.get_default_object(unreal.Button).get_editor_property('widget_style')
    for Property, Source, Color in [
        ('normal_base', 'normal', (.035, .07, .09, 1)),
        ('normal_hovered', 'hovered', (.065, .15, .2, 1)),
        ('normal_pressed', 'pressed', (.025, .055, .07, 1)),
        ('selected_base', 'normal', (.035, .07, .09, 1)),
        ('selected_hovered', 'hovered', (.065, .15, .2, 1)),
        ('selected_pressed', 'pressed', (.025, .055, .07, 1)),
        ('disabled', 'normal', (.025, .035, .04, 1)),
    ]:
        Brush = BaseStyle.get_editor_property(Source)
        Brush.set_editor_property('tint_color', unreal.SlateColor(specified_color=unreal.LinearColor(*Color)))
        Defaults.set_editor_property(Property, Brush)
    Defaults.set_editor_property('button_padding', unreal.Margin(12, 10, 12, 10))
    Defaults.set_editor_property('min_height', 48)
    for Property in ['normal_text_style', 'normal_hovered_text_style', 'selected_text_style', 'selected_hovered_text_style', 'disabled_text_style']:
        Defaults.set_editor_property(Property, TextStyle('Button'))
    Save(Asset)


def CreateButton():
    Asset = unreal.load_asset(ButtonPath)
    if Asset:
        return Asset
    Factory = unreal.WidgetBlueprintFactory()
    Factory.set_editor_property('parent_class', unreal.CommonButtonBase.static_class())
    Asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset('WBP_MainButton', '/Game/Base/UI', unreal.WidgetBlueprint, Factory)
    UI.AddWidget(Asset, 'ButtonContent', unreal.NamedSlot)
    BlueprintTools.add_event_dispatcher(Asset, 'OnClicked')
    Defaults = unreal.get_default_object(Asset.generated_class())
    InspectProperties(Defaults)
    Defaults.set_editor_property('style', unreal.load_class(None, StyleFolder + '/BPS_Button_Default.BPS_Button_Default_C'))
    Defaults.set_editor_property('is_focusable', True)
    Graph = BlueprintTools.get_graph(Asset, 'EventGraph')
    UI.WriteGraph(Graph, '''
    (event CommonButton|EventOnClicked
        (Default|CallOnClicked))
    ''')
    Save(Asset)
    return Asset


def ApplyTextStyle(Blueprint, Widget, Size=20):
    Role = GetTextRole(Blueprint, Widget.get_name(), Size)
    Widget.set_style(TextStyle(Role))
    if Widget.get_name().endswith('Unit'):
        Widget.set_auto_wrap_text(False)
    if Role == 'Button':
        Widget.set_margin(unreal.Margin(0))
        Widget.set_editor_property('justification', unreal.TextJustify.CENTER)


def MigrateWidget(Blueprint, BaselineGraphs):
    Backup(Blueprint)
    Entries = UI.CallUMG('GetWidgets', {'widgetBlueprint': UI.ObjectArgument(Blueprint)})['widgets']
    TextCount = 0
    ButtonCount = 0
    for Entry in Entries:
        Widget = unreal.find_object(None, Entry['widget']['refPath'])
        if not isinstance(Widget, unreal.TextBlock):
            continue
        Size = Widget.get_editor_property('font').get_editor_property('size')
        if not isinstance(Widget, unreal.CommonTextBlock):
            Replacement = UI.CallUMG('ReplaceWidgetWithTemplate', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'widgetToReplace': UI.ObjectArgument(Widget), 'templateClass': UI.ObjectArgument(unreal.CommonTextBlock.static_class())})
            if not Replacement['bSuccess']:
                raise RuntimeError('Failed to replace text widget')
            Widget = unreal.find_object(None, Entry['widget']['refPath'])
        InspectProperties(Widget)
        ApplyTextStyle(Blueprint, Widget, Size)
        TextCount += 1
    for Entry in Entries:
        Widget = unreal.find_object(None, Entry['widget']['refPath'])
        if not isinstance(Widget, unreal.Button):
            if isinstance(Widget, unreal.CommonButtonBase):
                ButtonCount += 1
            continue
        Content = Widget.get_child_at(0)
        Parent = Widget.get_parent()
        Wrapper = None
        if not Parent:
            Wrapped = UI.CallUMG('WrapWidgets', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'widgets': [UI.ObjectArgument(Widget)], 'wrapperClass': UI.ObjectArgument(unreal.VerticalBox.static_class())})
            Wrapper = unreal.find_object(None, Wrapped[0]['widget']['refPath'])
            Parent = Wrapper
        if Content:
            UI.CallUMG('MoveWidget', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'widget': UI.ObjectArgument(Content), 'newParent': UI.ObjectArgument(Parent), 'childIndex': -1})
        Replacement = UI.CallUMG('ReplaceWidgetWithTemplate', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'widgetToReplace': UI.ObjectArgument(Widget), 'templateClass': UI.ObjectArgument(unreal.load_asset(ButtonPath).generated_class())})
        if not Replacement['bSuccess']:
            raise RuntimeError('Failed to replace button widget')
        Widget = unreal.find_object(None, Entry['widget']['refPath'])
        InspectProperties(Widget)
        if Content:
            UI.CallUMG('SetNamedSlotContent', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'hostWidget': UI.ObjectArgument(Widget), 'slotName': 'ButtonContent', 'widgetClass': UI.ObjectArgument(Content.get_class()), 'widgetName': Content.get_name()})
            if isinstance(Content, unreal.CommonTextBlock):
                Content.set_margin(unreal.Margin(0))
                Content.set_editor_property('justification', unreal.TextJustify.CENTER)
                Content.set_style(TextStyle('Button'))
        if Wrapper:
            UI.CallUMG('ReplaceWidgetWithChild', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'widgetToReplace': UI.ObjectArgument(Wrapper)})
        ButtonCount += 1
    RepairButtonEvents(Blueprint, BaselineGraphs)
    Save(Blueprint)
    return {'Path': Blueprint.get_path_name(), 'TextCount': TextCount, 'ButtonCount': ButtonCount}


def RepairButtonEvents(Blueprint, BaselineGraphs):
    for GraphName, BaselineCode in BaselineGraphs.items():
        Names = re.findall(r'\(event OnClicked\(([^)]+)\)', BaselineCode)
        if not Names:
            continue
        Graph = BlueprintTools.get_graph(Blueprint, GraphName)
        Information = BlueprintTools.get_node_infos(BlueprintTools.find_nodes(Graph))
        Existing = {Name for Name in Names if any(Entry.type_id == 'AddEvent|OnClicked(' + Name + ')' for Entry in Information)}
        Missing = [Name for Name in Names if Name not in Existing]
        Invalid = [Entry for Entry in Information if Entry.type_id == 'AddEvent|OnClicked(None)']
        if len(Missing) != len(Invalid):
            raise RuntimeError('Button event recovery is ambiguous: ' + Blueprint.get_path_name())
        for Name, Previous in zip(Missing, Invalid):
            Widget = unreal.find_object(None, Blueprint.get_path_name() + ':WidgetTree.' + Name)
            UI.CallUMG('BindToEventProperty', {'widgetBlueprint': UI.ObjectArgument(Blueprint), 'eventName': 'OnClicked', 'propertyName': Name, 'propertyClass': UI.ObjectArgument(Widget.get_class())})
            Current, = [Entry for Entry in BlueprintTools.get_node_infos(BlueprintTools.find_nodes(Graph)) if Entry.type_id == 'AddEvent|OnClicked(' + Name + ')']
            for PreviousPin in Previous.output_pins:
                CurrentPin, = [Pin for Pin in Current.output_pins if Pin.name == PreviousPin.name]
                for Destination in PreviousPin.connected_pins:
                    BlueprintTools.connect_pins(CurrentPin.pin_id, Destination)
            BlueprintTools.delete_node(Previous.node)


def ConfigureInputText():
    Blueprint = unreal.load_asset('/Game/Base/UI/WBP_Settings')
    Graph = BlueprintTools.get_graph(Blueprint, 'EventGraph')
    if 'BPS_Text_Section_C' in BlueprintTools.read_graph_dsl(Graph):
        return
    UI.WriteGraph(Graph, '(event UserInterface|EventPreConstruct (IsDesignTime))')
    Event, = [Entry.node for Entry in BlueprintTools.get_node_infos(BlueprintTools.find_nodes(Graph)) if Entry.type_id == 'AddEvent|UserInterface|EventPreConstruct']
    Defaults = BlueprintTools.create_node(Graph, 'Class|GetClassDefaults', unreal.IntPoint(-500, -500))
    BlueprintTools.set_pin_value(UI.FindPin(Defaults, 'Class', unreal.EdGraphPinDirection.EGPD_INPUT), TextStyle('Section').get_path_name())
    InspectProperties(Defaults)
    Values = json.loads(ObjectTools.get_properties(Defaults, ['showPinForProperties']))
    for Entry in Values['showPinForProperties']:
        Entry['bShowPin'] = Entry['propertyName'] == 'Font'
    ObjectTools.set_properties(Defaults, json.dumps(Values))
    Font = BlueprintTools.create_node(Graph, 'Class|SpinBox|SetFont', unreal.IntPoint(100, -600))
    InputWidget = BlueprintTools.create_node(Graph, 'Variables|WBP_Settings|GetCameraHeightInput', unreal.IntPoint(-500, -700))
    BlueprintTools.connect_pins(UI.FindPin(Defaults, 'Font', unreal.EdGraphPinDirection.EGPD_OUTPUT), UI.FindPin(Font, 'Font', unreal.EdGraphPinDirection.EGPD_INPUT))
    BlueprintTools.connect_pins(UI.FindPin(InputWidget, 'CameraHeightInput', unreal.EdGraphPinDirection.EGPD_OUTPUT), UI.FindPin(Font, 'self', unreal.EdGraphPinDirection.EGPD_INPUT))
    BlueprintTools.connect_pins(UI.FindPin(Event, 'then', unreal.EdGraphPinDirection.EGPD_OUTPUT), UI.FindPin(Font, 'execute', unreal.EdGraphPinDirection.EGPD_INPUT))
    Input = unreal.find_object(None, Blueprint.get_path_name() + ':WidgetTree.CameraHeightInput')
    Input.set_editor_property('font', unreal.get_default_object(TextStyle('Section')).get_editor_property('font'))
    Save(Blueprint)


def Configure():
    CreateStyles()
    CreateButton()
    Results = []
    for AssetData in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path('/Game', True):
        if AssetData.asset_class_path.asset_name != 'WidgetBlueprint' or str(AssetData.package_name) == ButtonPath:
            continue
        Blueprint = AssetData.get_asset()
        Graphs = {Graph.get_name(): BlueprintTools.read_graph_dsl(Graph) for Graph in BlueprintTools.list_graphs(Blueprint)}
        Results.append(MigrateWidget(Blueprint, Graphs))
    ConfigureInputText()
    Directory = ProjectDirectory / 'Saved/Verification/CommonUI'
    Directory.mkdir(parents=True, exist_ok=True)
    (Directory / 'Migration.json').write_text(json.dumps(Results, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps(Results))


if __name__ == '__main__':
    Configure()
