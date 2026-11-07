@echo off
setlocal
set "MetaDirectory=M:\Tools\MetaXR"
set "SimulatorDirectory=%MetaDirectory%\Legacy76"
set "EditorExecutable=M:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Win64-DebugGame.exe"
set "XR_RUNTIME_JSON=%SimulatorDirectory%\meta_openxr_simulator.json"
set "META_XRSIM_CONFIG_JSON=%SimulatorDirectory%\config\sim_core_configuration.json"

if not exist "%XR_RUNTIME_JSON%" (
    echo Meta XR Simulator runtime was not found: %XR_RUNTIME_JSON%
    pause
    exit /b 1
)

if not exist "%EditorExecutable%" (
    echo Unreal Editor was not found: %EditorExecutable%
    pause
    exit /b 1
)

start "" "%EditorExecutable%" "%~dp0YakutSimulator.uproject" %*
endlocal
