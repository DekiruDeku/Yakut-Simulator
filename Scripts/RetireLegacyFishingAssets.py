import json
import pathlib
import stat
import sys
import unreal

ProjectDirectory = pathlib.Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve()
sys.path.append(str(ProjectDirectory / 'Scripts'))
from BuildStationaryWorkstations import Backup, ReportDirectory

Registry = unreal.AssetRegistryHelpers.get_asset_registry()
Options = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True)
Candidates = ['/Game/Maps/L_Fishing', '/Game/Maps/L_Clinic', '/Game/Fishing/UI/WBP_FishingConsole', '/Game/Fishing/Effects/NS_FishSchool']
Candidates += ['/Game/Fishing/Blueprints/BP_' + Name for Name in ['FishingScenario', 'FishingSonar', 'Trawl', 'FishSchool_Cod', 'FishSchool_Haddock', 'FishSchool', 'FishFreezer', 'FishContainerItem', 'KnifeSharpeningStation', 'VesselThrottleLever', 'VesselSteeringLever', 'TrawlControlLever', 'CatchReleaseLever', 'SonarScanLever', 'ConveyorPowerLever']]
Report = {'Removed': [], 'RetainedForReferences': {}}
ReportPath = ReportDirectory / 'RetiredAssets.json'
if ReportPath.exists():
    Report = json.loads(ReportPath.read_text(encoding='utf-8'))
Pending = set(Candidates)
while Pending:
    Progressed = False
    for Package in sorted(Pending):
        if not unreal.EditorAssetLibrary.does_asset_exist(Package):
            Pending.remove(Package)
            continue
        Referencers = [str(Name) for Name in Registry.get_referencers(Package, Options) if str(Name) != Package]
        if Referencers:
            Report['RetainedForReferences'][Package] = Referencers
            continue
        Asset = unreal.load_asset(Package)
        Backup(Asset)
        File = ProjectDirectory / 'Content' / (Package.removeprefix('/Game/') + '.uasset')
        if Package.startswith('/Game/Maps/'):
            File = File.with_suffix('.umap')
        if File.exists():
            File.chmod(File.stat().st_mode | stat.S_IWRITE)
        if not unreal.EditorAssetLibrary.delete_asset(Package):
            raise RuntimeError('Retired package was not removed: ' + Package)
        if File.exists():
            File.resolve().relative_to((ProjectDirectory / 'Content').resolve())
            File.unlink()
        Pending.remove(Package)
        Report['Removed'].append(Package)
        Report['RetainedForReferences'].pop(Package, None)
        Progressed = True
        unreal.SystemLibrary.collect_garbage()
    if not Progressed:
        break
    Registry.scan_paths_synchronous(['/Game/Maps', '/Game/Fishing'], True)
ReportDirectory.mkdir(parents=True, exist_ok=True)
ReportPath.write_text(json.dumps(Report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(Report, ensure_ascii=False))
