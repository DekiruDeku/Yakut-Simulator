"""One-time editor preparation when the generated text assets are missing."""
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
unreal.SystemLibrary.execute_console_command(world, "Fishing.PrepareTextAssets")
assert unreal.EditorAssetLibrary.does_asset_exist("/Game/Fishing/UI/F_FishingCyrillic")
assert unreal.EditorAssetLibrary.does_asset_exist("/Game/Fishing/UI/MI_FishingText")
