param([switch]$Simulator, [switch]$Packaged)

$ErrorActionPreference = 'Stop'
$projectFile = Join-Path $PSScriptRoot 'YakutSimulator\YakutSimulator.uproject'
$projectInfo = Get-Content -LiteralPath $projectFile -Raw | ConvertFrom-Json
$engineVersion = [string]$projectInfo.EngineAssociation
$engineRegistry = Get-ItemProperty -LiteralPath "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$engineVersion" -ErrorAction SilentlyContinue
$engineDirectory = if ($engineRegistry.InstalledDirectory) { $engineRegistry.InstalledDirectory } else { Join-Path $env:ProgramFiles "Epic Games\UE_$engineVersion" }
$editorFile = Join-Path $engineDirectory 'Engine\Binaries\Win64\UnrealEditor.exe'
$packagedRoot = Join-Path $PSScriptRoot 'YakutSimulator\Saved\Packaged\MiniGames\Windows'
if ($Packaged) { $editorFile = Join-Path $packagedRoot 'YakutSimulator.exe' }
if (-not (Test-Path -LiteralPath $editorFile -PathType Leaf)) { throw "Приложение не найдено: $editorFile" }
$alreadyOpen = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe' OR Name='YakutSimulator.exe'" | Where-Object {
    ($_.CommandLine -and $_.CommandLine.IndexOf($projectFile, [StringComparison]::OrdinalIgnoreCase) -ge 0) -or
    ($_.ExecutablePath -and $_.ExecutablePath.StartsWith($packagedRoot + '\', [StringComparison]::OrdinalIgnoreCase))
}
if ($alreadyOpen) { throw 'Сначала закройте редактор или собранную игру этого проекта, чтобы не запускать две VR-сессии.' }
$editorArguments = @()
if (-not $Packaged) {
    $editorArguments += ('"{0}"' -f $projectFile), '-ModelContextProtocolStartServer', '-ModelContextProtocolPort=8000'
}
if ($Simulator) {
    # shortcut: runtime 69 restores input here; retest 207.1+ before using newer Meta features.
    $runtimeCache = Join-Path $PSScriptRoot 'YakutSimulator\Saved\MetaXRSimulator\69'
    $runtimeDirectory = Join-Path $runtimeCache 'package\MetaXRSimulator'
    $runtimeManifest = Join-Path $runtimeDirectory 'meta_openxr_simulator.json'
    if (-not (Test-Path -LiteralPath $runtimeManifest -PathType Leaf)) {
        New-Item -ItemType Directory -Path $runtimeCache -Force | Out-Null
        $runtimeArchive = Join-Path $runtimeCache 'simulator-69.0.0.tgz'
        Write-Host 'Downloading Meta XR Simulator 69 (172 MB)...'
        Invoke-WebRequest -UseBasicParsing -Uri 'https://npm.developer.oculus.com/com.meta.xr.simulator/-/com.meta.xr.simulator-69.0.0.tgz' -OutFile $runtimeArchive -TimeoutSec 120
        $expectedHash = 'A27A1B5FFAF5984DBE682F2843152A68530E11CA6ADC998788DBC059EF51DD089412C1CD43C4409364DB27384912390F647AB47BEAC056C707F7D0517B9DFE59'
        if ((Get-FileHash -LiteralPath $runtimeArchive -Algorithm SHA512).Hash -ne $expectedHash) { throw 'Meta XR Simulator archive checksum mismatch.' }
        & tar.exe -xzf $runtimeArchive -C $runtimeCache 'package/MetaXRSimulator'
        if ($LASTEXITCODE -ne 0) { throw 'Meta XR Simulator extraction failed.' }
    }
    foreach ($runtimeFile in @('meta_openxr_simulator.json', 'SIMULATOR.dll', 'config\sim_core_configuration.json')) {
        if (-not (Test-Path -LiteralPath (Join-Path $runtimeDirectory $runtimeFile) -PathType Leaf)) { throw "Missing simulator file: $runtimeFile" }
    }
    # Meta SDK 207 binds xrShareSpacesMETA, which runtime 69 does not provide.
    $editorArguments += '-DisablePlugins=OculusXR'
    if ($Packaged) {
        # shortcut: runtime 69/DX12 rejects speculative PSOs here; retest precaching with newer runtimes/drivers.
        $editorArguments += '-ini:Engine:[SystemSettings]:r.PSOPrecaching=0'
    }
}
$previousRuntime = $env:XR_RUNTIME_JSON
$previousSimulatorConfig = $env:META_XRSIM_CONFIG_JSON
try {
    if ($Simulator) {
        $env:XR_RUNTIME_JSON = $runtimeManifest
        $env:META_XRSIM_CONFIG_JSON = Join-Path $runtimeDirectory 'config\sim_core_configuration.json'
    }
    if ($editorArguments.Count) { Start-Process -FilePath $editorFile -ArgumentList $editorArguments }
    else { Start-Process -FilePath $editorFile }
} finally {
    $env:XR_RUNTIME_JSON = $previousRuntime
    $env:META_XRSIM_CONFIG_JSON = $previousSimulatorConfig
}
