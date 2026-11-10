import difflib
import decimal
import json
import pathlib
import re
import sys
import toolset_registry
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
import ConfigureCommonUI as CommonUI
import CreatePrototypeUI as UI

Directory = ProjectDirectory / 'Saved/Verification/CommonUI'
Baseline = json.loads((Directory / 'Before.json').read_text(encoding='utf-8'))
EditorState = unreal.ToolsetRegistry.execute_tool('EditorToolset.EditorAppToolset', 'IsPIERunning', '{}')
Report = {'PIERunning': json.loads(EditorState.value)['returnValue'], 'GameplayExecuted': False, 'Assets': [], 'Styles': []}


def GraphSections(Code):
    def NormalizeNumber(Match):
        Value = Match.group(0)
        if Value.startswith('"'):
            return Value
        return str(decimal.Decimal(Value).normalize())
    Code = re.sub(r'"(?:\\.|[^"\\])*"|(?<![\w.])-?\d+(?:\.\d+)?(?![\w.])', NormalizeNumber, Code)
    return sorted(Section.strip() for Section in re.split(r'(?m)(?=^\((?:event|fn) )', Code) if Section.strip())


with toolset_registry.tool_raising_exceptions():
    for Definition in Baseline:
        Blueprint = unreal.load_asset(Definition['Path'])
        BlueprintTools.compile_blueprint(Blueprint, True)
        Entries = UI.CallUMG('GetWidgets', {'widgetBlueprint': UI.ObjectArgument(Blueprint)})['widgets']
        Widgets = {Entry['widgetName']: unreal.find_object(None, Entry['widget']['refPath']) for Entry in Entries}
        Verification = {'Path': Definition['Path'], 'Compilation': 'Passed', 'Widgets': [], 'Graphs': {}, 'GraphDifferences': {}}
        for Previous in Definition['Widgets']:
            Widget = Widgets[Previous['Name']]
            Information = {'Name': Widget.get_name(), 'Class': Widget.get_class().get_path_name()}
            if Previous['Class'] == '/Script/UMG.TextBlock':
                assert isinstance(Widget, unreal.CommonTextBlock)
                assert str(Widget.get_text()) == Previous['Text']
                Style = Widget.get_editor_property('style')
                assert Style.get_path_name().startswith(CommonUI.StyleFolder)
                Information.update({'Style': Style.get_path_name(), 'TextPreserved': True})
            if Previous['Class'] == '/Script/UMG.Button':
                assert isinstance(Widget, unreal.CommonButtonBase)
                assert Widget.get_class().get_path_name() == CommonUI.ButtonPath + '.WBP_MainButton_C'
                assert Widget.get_editor_property('style').get_path_name() == CommonUI.StyleFolder + '/BPS_Button_Default.BPS_Button_Default_C'
                Information['Style'] = Widget.get_editor_property('style').get_path_name()
            Verification['Widgets'].append(Information)
        for Graph in BlueprintTools.list_graphs(Blueprint):
            Code = BlueprintTools.read_graph_dsl(Graph)
            Verification['Graphs'][Graph.get_name()] = Code
            PreviousCode = Definition['Graphs'].get(Graph.get_name(), '')
            BeforeSections = GraphSections(PreviousCode)
            AfterSections = GraphSections(Code)
            Missing = [Section for Section in BeforeSections if Section not in AfterSections]
            if Missing:
                Verification['GraphDifferences'][Graph.get_name()] = list(difflib.unified_diff(PreviousCode.splitlines(), Code.splitlines()))
        assert not any('OnClicked(None)' in Code for Code in Verification['Graphs'].values())
        assert not Verification['GraphDifferences'], Verification['GraphDifferences']
        CommonUI.Save(Blueprint)
        Report['Assets'].append(Verification)
    for AssetData in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(CommonUI.StyleFolder, True):
        Asset = AssetData.get_asset()
        BlueprintTools.compile_blueprint(Asset, True)
        Report['Styles'].append(Asset.get_path_name())
    Button = unreal.load_asset(CommonUI.ButtonPath)
    BlueprintTools.compile_blueprint(Button, True)
    Report['ButtonGraph'] = BlueprintTools.read_graph_dsl(BlueprintTools.get_graph(Button, 'EventGraph'))
    CommonUI.Save(Button)
    for Path in ['/Game/Base/Blueprints/BP_MainHUD', '/Game/Base/MainMenu/BP_MainMenuPlacementWidget', '/Game/Base/Blueprints/BP_WorkstationScenario', '/Game/Minigames/Fishing/Workstations/Blueprints/BP_TrawlWinchScenario', '/Game/Minigames/Fishing/Workstations/Blueprints/BP_FishSortingScenario', '/Game/Minigames/Fishing/Workstations/Blueprints/BP_FishCuttingScenario']:
        BlueprintTools.compile_blueprint(unreal.load_asset(Path), True)
    Report['DependentBlueprintCompilation'] = 'Passed'
(Directory / 'Verification.json').write_text(json.dumps(Report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'Assets': len(Report['Assets']), 'Styles': len(Report['Styles']), 'Differences': {Entry['Path']: Entry['GraphDifferences'] for Entry in Report['Assets'] if Entry['GraphDifferences']}}))
