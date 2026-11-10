import json
import pathlib
import shutil
import sys
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools as Blueprint
from editor_toolset.toolsets.object import ObjectTools

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
import CreatePrototypeUI as UI

BackupDirectory = ProjectDirectory / 'Saved/Backups/InGameSettings'
VerificationDirectory = ProjectDirectory / 'Saved/Verification/InGameSettings'
MixPath = '/Game/Base/Audio/SM_Settings'
MasterPath = '/Engine/EngineSounds/Master.Master'


def Backup(Asset):
    Relative = pathlib.Path('Content') / Asset.get_path_name().split('.')[0].removeprefix('/Game/')
    Source = (ProjectDirectory / Relative).with_suffix('.uasset')
    Destination = (BackupDirectory / Relative).with_suffix('.uasset')
    if Source.exists() and not Destination.exists():
        Destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(Source, Destination)


def Save(Asset):
    Blueprint.compile_blueprint(Asset, True)
    unreal.EditorAssetLibrary.save_loaded_asset(Asset, False)


def Reset(Graph):
    for Node in list(Blueprint.find_nodes(Graph)):
        if not isinstance(Node, unreal.K2Node_FunctionEntry):
            Blueprint.delete_node(Node)


def Function(Asset, Name, Parameters=None):
    Graph = Blueprint.get_graph(Asset, Name)
    if not Graph:
        Graph = Blueprint.add_function_graph(Asset, Name)
        for Parameter, Type in (Parameters or []):
            Blueprint.add_function_param(Graph, Parameter, Type, True)
    Reset(Graph)
    return Graph


def Variable(Asset, Name, Type):
    if Name not in Blueprint.list_variables(Asset):
        if isinstance(Type, str):
            Blueprint.add_variable(Asset, Name, Type)
        else:
            Blueprint.add_object_variable(Asset, Name, Type)


def Write(Graph, Code):
    UI.WriteGraph(Graph, Code)
    Blueprint.arrange_nodes(Blueprint.find_nodes(Graph))


def Connect(OutputNode, OutputName, InputNode, InputName):
    Blueprint.connect_pins(UI.FindPin(OutputNode, OutputName, unreal.EdGraphPinDirection.EGPD_OUTPUT), UI.FindPin(InputNode, InputName, unreal.EdGraphPinDirection.EGPD_INPUT))


def InputEvent(Graph, ActionName, StartedFunction=None, ReleasedFunction=None, ValueFunction=None):
    Node = Blueprint.create_node(Graph, 'Input|EnhancedActionEvents|' + ActionName, unreal.IntPoint(0, 0))
    if StartedFunction:
        Pressed = Blueprint.create_node(Graph, 'CallFunction|' + StartedFunction, unreal.IntPoint(300, 0))
        Blueprint.set_pin_value(UI.FindPin(Pressed, 'bPressed', unreal.EdGraphPinDirection.EGPD_INPUT), 'true')
        Connect(Node, 'Started', Pressed, 'execute')
    if ReleasedFunction:
        Released = Blueprint.create_node(Graph, 'CallFunction|' + ReleasedFunction, unreal.IntPoint(300, 200))
        Connect(Node, 'Completed', Released, 'execute')
        Connect(Node, 'Canceled', Released, 'execute')
    if ValueFunction:
        Process = Blueprint.create_node(Graph, 'CallFunction|' + ValueFunction, unreal.IntPoint(300, 400))
        Connect(Node, 'Triggered', Process, 'execute')
        Connect(Node, 'ActionValue', Process, 'Value')


def ConfigureInput():
    Mapping = unreal.load_asset('/Game/Base/Input/IMC_Interaction')
    Backup(Mapping)
    for Name, KeyName in [('IA_SettingsX', 'OculusTouch_Left_X_Click'), ('IA_SettingsY', 'OculusTouch_Left_Y_Click')]:
        Key = unreal.Key()
        Key.set_editor_property('key_name', KeyName)
        Action = unreal.load_asset('/Game/Base/Input/' + Name)
        if not Action:
            Action = unreal.AssetToolsHelpers.get_asset_tools().create_asset(Name, '/Game/Base/Input', unreal.InputAction, unreal.InputAction_Factory())
        Backup(Action)
        Action.set_editor_property('value_type', unreal.InputActionValueType.BOOLEAN)
        Action.set_editor_property('consume_input', False)
        Mapping.unmap_all_keys_from_action(Action)
        Mapping.map_key(Action, Key)
        AlternateKey = unreal.Key()
        AlternateKey.set_editor_property('key_name', KeyName.replace('OculusTouch', 'PICO'))
        Mapping.map_key(Action, AlternateKey)
        unreal.EditorAssetLibrary.save_loaded_asset(Action, False)
    unreal.EditorAssetLibrary.save_loaded_asset(Mapping, False)


def ConfigureWidget():
    Widget = UI.GetBlueprint('WBP_Settings')
    Backup(Widget)
    Variable(Widget, 'SettingsState', unreal.MainGameInstance.static_class())
    unreal.BlueprintEditorLibrary.set_blueprint_variable_instance_editable(Widget, 'SettingsState', True)
    unreal.BlueprintEditorLibrary.set_blueprint_variable_expose_on_spawn(Widget, 'SettingsState', True)
    if not Blueprint.get_graph(Widget, 'CloseRequested'):
        Blueprint.add_event_dispatcher(Widget, 'CloseRequested')
    Surface = unreal.find_object(None, Widget.get_path_name() + ':WidgetTree.Surface')
    Surface.set_padding(unreal.Margin(32))
    Content = UI.AddWidget(Widget, 'SettingsContent', unreal.VerticalBox, Surface)
    for Name, Text, Size in [('SettingsTitle', 'Настройки', 38), ('AudioTitle', 'Звук', 28), ('VolumeLabel', 'Общая громкость', 26)]:
        Label = UI.AddText(Widget, Name, Content, Text, Size)
        Label.slot.set_padding(unreal.Margin(0, 0, 0, 20))
    Slider = UI.AddWidget(Widget, 'MasterVolumeSlider', unreal.Slider, Content, True)
    Slider.set_value(1)
    Slider.set_step_size(.05)
    Slider.set_slider_bar_color(unreal.LinearColor(.35, .42, .5, 1))
    Slider.set_slider_handle_color(unreal.LinearColor(.8, .86, .95, 1))
    Style = Slider.get_editor_property('widget_style')
    Style.set_editor_property('bar_thickness', 10)
    for Property in ['normal_thumb_image', 'hovered_thumb_image', 'disabled_thumb_image']:
        Brush = Style.get_editor_property(Property)
        ImageSize = Brush.get_editor_property('image_size')
        ImageSize.import_text('(X=28,Y=42)')
        Brush.set_editor_property('image_size', ImageSize)
        Style.set_editor_property(Property, Brush)
    Slider.set_editor_property('widget_style', Style)
    Slider.slot.set_padding(unreal.Margin(0, 12, 0, 28))
    Note = UI.AddText(Widget, 'SettingsHint', Content, 'Удерживайте X и Y 0,6 секунды, чтобы открыть или закрыть настройки.', 21)
    Note.slot.set_padding(unreal.Margin(0, 0, 0, 24))
    Button = UI.AddWidget(Widget, 'CloseSettingsButton', unreal.Button, Content, True)
    Label = UI.AddText(Widget, 'CloseSettingsText', Button, 'Закрыть', 26)
    Label.set_editor_property('margin', unreal.Margin(16, 12, 16, 12))
    Blueprint.compile_blueprint(Widget)
    UI.BindButton(Widget, 'CloseSettingsButton')
    Graph = Blueprint.get_graph(Widget, 'EventGraph')
    if 'OnValueChanged(MasterVolumeSlider)' not in Blueprint.read_graph_dsl(Graph):
        UI.CallUMG('BindToEventProperty', {'widgetBlueprint': UI.ObjectArgument(Widget), 'eventName': 'OnValueChanged', 'propertyName': 'MasterVolumeSlider', 'propertyClass': UI.ObjectArgument(unreal.Slider.static_class())})
    Refresh = Function(Widget, 'ApplyVolume', [('Value', 'float')])
    Write(Refresh, f'''(fn ApplyVolume (Value)
        (Class|MainGameInstance|SetMasterVolume :self (Variables|Default|GetSettingsState) :MasterVolume Value)
        (Audio|SetSoundMixClassOverride :InSoundMixModifier "{MixPath}" :InSoundClass "{MasterPath}" :Volume Value :Pitch 1 :FadeInTime 0.05 :bApplyToChildren true))''')
    Blueprint.compile_blueprint(Widget)
    for Node in list(Blueprint.find_nodes(Graph)):
        if not isinstance(Node, unreal.K2Node_ComponentBoundEvent):
            Blueprint.delete_node(Node)
    Write(Graph, '''
    (event UserInterface|EventConstruct
        (Class|Slider|SetValue :self (Variables|WBP_Settings|GetMasterVolumeSlider) :Value (Class|MainGameInstance|GetMasterVolume :self (Variables|Default|GetSettingsState)))
        (CallFunction|ApplyVolume :Value (Class|MainGameInstance|GetMasterVolume :self (Variables|Default|GetSettingsState))))
    (event OnValueChanged(MasterVolumeSlider) (Value)
        (CallFunction|ApplyVolume :Value Value))
    (event OnClicked(CloseSettingsButton)
        (Default|CallCloseRequested))''')
    Save(Widget)
    return Widget


def ConfigureHud(Widget):
    Hud = unreal.load_asset('/Game/Base/Blueprints/BP_MainHUD')
    if not Hud:
        Hud = Blueprint.create('/Game/Base/Blueprints', 'BP_MainHUD', unreal.MainHUD.static_class())
    Backup(Hud)
    for Name in ['bSettingsOpen', 'bSettingsXPressed', 'bSettingsYPressed', 'bLeftTriggerPressed', 'bRightTriggerPressed']:
        Variable(Hud, Name, 'bool')
    Variable(Hud, 'SettingsState', unreal.MainGameInstance.static_class())
    Blueprint.compile_blueprint(Hud)
    Close = Function(Hud, 'CloseSettings')
    Write(Close, '''(fn CloseSettings ()
        (Collision|SetCollisionEnabled :self (Variables|Settings|GetSettingsDisplay) :NewType "NoCollision")
        (Rendering|SetVisibility :self (Variables|Settings|GetSettingsDisplay) :bNewVisibility false)
        (Variables|Default|SetSettingsOpen false))''')
    Toggle = Function(Hud, 'ToggleSettings')
    Write(Toggle, '''(fn ToggleSettings ()
        (if (Variables|Default|GetSettingsOpen)
            (CallFunction|CloseSettings)
            (else
                (bind Camera (Class|PlayerController|GetPlayerCameraManager :self (HUD|GetOwningPlayerController)))
                (bind Rotation (Camera|GetCameraRotation :self Camera))
                (Transformation|SetWorldLocationAndRotation :self (Variables|Settings|GetSettingsDisplay)
                    :NewLocation (+ (Camera|GetCameraLocation :self Camera) (* (Math|Vector|GetForwardVector :InRot Rotation) 100))
                    :NewRotation (Math|Rotator|MakeRotator :Pitch 0 :Yaw (+ (.yaw Rotation) 180) :Roll 0))
                (Rendering|SetVisibility :self (Variables|Settings|GetSettingsDisplay) :bNewVisibility true)
                (Collision|SetCollisionEnabled :self (Variables|Settings|GetSettingsDisplay) :NewType "QueryOnly")
                (Variables|Default|SetSettingsOpen true))))''')
    Chord = Function(Hud, 'UpdateSettingsChord')
    Write(Chord, '''(fn UpdateSettingsChord ()
        (if (and (Variables|Default|GetSettingsXPressed) (Variables|Default|GetSettingsYPressed))
            (Utilities|Time|SetTimerbyFunctionName :Object self :FunctionName "ToggleSettings" :Time 0.6 :bLooping false)
            (else (Utilities|Time|ClearTimerbyFunctionName :Object self :FunctionName "ToggleSettings"))))''')
    for ButtonName in ['X', 'Y']:
        Graph = Function(Hud, 'SetSettings' + ButtonName, [('bPressed', 'bool')])
        Write(Graph, f'''(fn SetSettings{ButtonName} (bPressed)
            (Variables|Default|SetSettings{ButtonName}Pressed bPressed)
            (CallFunction|UpdateSettingsChord))''')
    for Hand in ['Left', 'Right']:
        Graph = Function(Hud, 'Process' + Hand + 'Pointer', [('Value', 'float')])
        Write(Graph, f'''(fn Process{Hand}Pointer (Value)
            (bind Pawn (Utilities|Casting|CastToVirtualRealityPawn :Object (Pawn|GetControlledPawn :self (HUD|GetOwningPlayerController)))
                (:then
                    (bind Pointer (Class|VirtualRealityPawn|Get{Hand}Pointer :self Pawn))
                    (if (and (>= Value 0.65) (not (Variables|Default|Get{Hand}TriggerPressed)))
                        (Variables|Default|Set{Hand}TriggerPressed true)
                        (Utilities|IsValid :InputObject (Class|VirtualRealityPawn|Get{Hand}Item :self Pawn)
                            (:"Is Valid")
                            (:"Is Not Valid"
                                (if (Interaction|IsOverHitTestVisibleWidget :self Pointer)
                                    (Interaction|PressPointerKey :self Pointer :Key "LeftMouseButton"))))
                        (elif (and (<= Value 0.35) (Variables|Default|Get{Hand}TriggerPressed))
                            (Variables|Default|Set{Hand}TriggerPressed false)
                            (Interaction|ReleasePointerKey :self Pointer :Key "LeftMouseButton"))))
                (:CastFailed)))''')
        Release = Function(Hud, 'Release' + Hand + 'Pointer')
        Write(Release, f'''(fn Release{Hand}Pointer () (CallFunction|Process{Hand}Pointer :Value 0))''')
    Blueprint.compile_blueprint(Hud)
    Graph = Blueprint.get_graph(Hud, 'EventGraph')
    Reset(Graph)
    Write(Graph, f'''(event EventBeginPlay
        (Input|EnableInput :PlayerController (HUD|GetOwningPlayerController))
        (bind State (Utilities|Casting|CastToMainGameInstance :Object (Game|GetGameInstance))
            (:then
                (Variables|Default|SetSettingsState State)
                (Audio|PushSoundMixModifier :InSoundMixModifier "{MixPath}")
                (bind Screen (UserInterface|CreateWidget :Class "/Game/Base/UI/WBP_Settings.WBP_Settings_C" :OwningPlayer (HUD|GetOwningPlayerController)))
                (UserInterface|SetWidget :self (Variables|Settings|GetSettingsDisplay) :Widget Screen)
                (Default|BindEventtoCloseRequested :self Screen))
            (:CastFailed)))
        (event AddEvent|EventEndPlay (EndPlayReason)
            (CallFunction|ReleaseLeftPointer)
            (CallFunction|ReleaseRightPointer)
            (Utilities|Time|ClearTimerbyFunctionName :Object self :FunctionName "ToggleSettings")
            (Audio|PopSoundMixModifier :InSoundMixModifier "{MixPath}"))''')
    Creation = next(Node for Node in Blueprint.find_nodes(Graph) if Blueprint.get_node_infos([Node])[0].type_id == 'UserInterface|CreateWidget')
    State = Blueprint.create_node(Graph, 'Variables|Default|GetSettingsState', unreal.IntPoint(-200, -180))
    Connect(State, 'SettingsState', Creation, 'SettingsState')
    UI.BindRefreshFunction(Graph, 'Default|BindEventtoCloseRequested', 'CloseSettings')
    for ButtonName in ['X', 'Y']:
        InputEvent(Graph, 'IA_Settings' + ButtonName, 'SetSettings' + ButtonName, 'SetSettings' + ButtonName)
    for Hand in ['Left', 'Right']:
        InputEvent(Graph, 'IA_Use' + Hand, ReleasedFunction='Release' + Hand + 'Pointer', ValueFunction='Process' + Hand + 'Pointer')
    Blueprint.arrange_nodes(Blueprint.find_nodes(Graph))
    Save(Hud)
    return Hud


def CreateInGameSettings():
    ConfigureInput()
    Mix = unreal.load_asset(MixPath)
    if not Mix:
        Mix = unreal.AssetToolsHelpers.get_asset_tools().create_asset('SM_Settings', '/Game/Base/Audio', unreal.SoundMix, unreal.SoundMixFactory())
        Mix.set_editor_property('duration', -1)
        unreal.EditorAssetLibrary.save_loaded_asset(Mix, False)
    Widget = ConfigureWidget()
    Hud = ConfigureHud(Widget)
    GameMode = unreal.load_asset('/Game/Base/Blueprints/BP_MainGameMode')
    Backup(GameMode)
    unreal.get_default_object(GameMode.generated_class()).set_editor_property('hud_class', Hud.generated_class())
    Save(GameMode)
    for Path in ['/Game/Base/Blueprints/BP_VirtualRealityPawn', '/Game/Base/Blueprints/BP_MainGameInstance']:
        Asset = unreal.load_asset(Path)
        Backup(Asset)
        Save(Asset)
    VerificationDirectory.mkdir(parents=True, exist_ok=True)
    Graphs = {}
    for Asset in [Hud, Widget]:
        Graphs[Asset.get_name()] = {Graph.get_name(): Blueprint.read_graph_dsl(Graph) for Graph in Blueprint.list_graphs(Asset)}
    (VerificationDirectory / 'BlueprintGraphs.json').write_text(json.dumps(Graphs, ensure_ascii=False, indent=2), encoding='utf-8')
    for Asset in [Hud, Widget]:
        unreal.EditorAssetLibrary.save_loaded_asset(Asset, False)
    print('In-game settings saved; Blueprint compilation passed; no gameplay sessions were started')


if __name__ == '__main__':
    CreateInGameSettings()
