import json
import pathlib
import unreal


def VerifyTrawlerWheelhouse():
    Editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    World = Editor.get_game_world()
    if not World:
        World = Editor.get_editor_world()
    Vessels = unreal.GameplayStatics.get_all_actors_of_class(World,unreal.TrawlerVessel)
    if len(Vessels)!=1:
        raise RuntimeError('Exactly one trawler is required for the wheelhouse verification')
    Vessel = Vessels[0]
    Transform = Vessel.get_actor_transform()
    Results = {'world':World.get_path_name(),'checks':[]}
    Components = {Component.get_name():Component for Component in Vessel.get_components_by_class(unreal.StaticMeshComponent)}
    for Name in ['WheelhouseVisual','WheelhouseGlassVisual']:
        Component = Components[Name]
        if not Component.static_mesh or str(Component.get_collision_profile_name())!='BlockAllDynamic':
            raise RuntimeError('Wheelhouse component configuration is invalid: '+Name)
    for Lateral in [-25,0,25]:
        Hit = unreal.SystemLibrary.capsule_trace_single(World,Transform.transform_location(unreal.Vector(60,Lateral,146)),Transform.transform_location(unreal.Vector(360,Lateral,146)),30,90,unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE)
        Results['checks'].append({'name':'EntranceCapsule_'+str(Lateral),'passed':Hit is None,'hit':str(Hit)})
    for Name,Start,End in [
        ('InteriorCapsule',(200,0,146),(520,0,146)),
        ('PortWall',(360,0,146),(360,-300,146)),
        ('StarboardWall',(360,0,146),(360,300,146)),
        ('BowWall',(360,0,146),(650,0,146))
    ]:
        Hit = unreal.SystemLibrary.capsule_trace_single(World,Transform.transform_location(unreal.Vector(*Start)),Transform.transform_location(unreal.Vector(*End)),30,90,unreal.TraceTypeQuery.ECC_VISIBILITY,False,[],unreal.DrawDebugTrace.NONE)
        bExpectedHit = Name!='InteriorCapsule'
        Results['checks'].append({'name':Name,'passed':bool(Hit)==bExpectedHit,'hit':str(Hit)})
    for Name,Start,End in [
        ('BowGlass',(360,0,211),(650,0,211)),
        ('SternGlass',(360,140,211),(60,140,211)),
        ('PortGlass',(360,0,211),(360,-300,211)),
        ('StarboardGlass',(360,0,211),(360,300,211))
    ]:
        Hit = unreal.SystemLibrary.line_trace_single(World,Transform.transform_location(unreal.Vector(*Start)),Transform.transform_location(unreal.Vector(*End)),unreal.TraceTypeQuery.ECC_VISIBILITY,True,[],unreal.DrawDebugTrace.NONE)
        bGlassHit = False
        if Hit:
            Details = Hit.to_tuple()
            bGlassHit = Details[10]==Components['WheelhouseGlassVisual']
        Results['checks'].append({'name':Name,'passed':bGlassHit,'hit':str(Hit)})
    GlassMaterial = Components['WheelhouseGlassVisual'].get_material(0)
    Results['checks'].append({'name':'TranslucentTwoSidedGlass','passed':GlassMaterial.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_TRANSLUCENT and GlassMaterial.get_editor_property('two_sided')})
    Results['passed'] = all(Entry['passed'] for Entry in Results['checks'])
    Directory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))/'Saved/Verification/TrawlerWheelhouse'
    Directory.mkdir(parents=True,exist_ok=True)
    FileName = 'EditorChecks.json'
    if Editor.get_game_world():
        FileName = 'PIEChecks.json'
    (Directory/FileName).write_text(json.dumps(Results,indent=2),encoding='utf-8')
    print(json.dumps(Results))
    if not Results['passed']:
        raise RuntimeError('Wheelhouse verification failed')


if __name__=='__main__':
    VerifyTrawlerWheelhouse()
