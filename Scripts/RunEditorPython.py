import argparse
import importlib.util
import json
import pathlib
import time


ArgumentParser = argparse.ArgumentParser()
ArgumentParser.add_argument("--code")
ArgumentParser.add_argument("--file")
Arguments = ArgumentParser.parse_args()
ModuleSpecification = importlib.util.spec_from_file_location(
    "UnrealRemoteExecution",
    "M:/UE/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python/remote_execution.py",
)
RemoteModule = importlib.util.module_from_spec(ModuleSpecification)
ModuleSpecification.loader.exec_module(RemoteModule)
Connection = RemoteModule.RemoteExecution()
Connection.start()
try:
    Deadline = time.monotonic() + 12.0
    TargetNode = None
    while time.monotonic() < Deadline:
        for Candidate in Connection.remote_nodes:
            if Candidate.get("project_name") == "YakutSimulator":
                TargetNode = Candidate
                break
        if TargetNode is not None:
            break
        time.sleep(0.2)
    if TargetNode is None:
        raise RuntimeError("The YakutSimulator Python endpoint was not found")
    Connection.open_command_connection(TargetNode["node_id"])
    Command = Arguments.code
    if Arguments.file:
        ScriptPath = pathlib.Path(Arguments.file).resolve()
        Command = "exec(compile(" + repr(ScriptPath.read_text(encoding="utf-8")) + ", " + repr(str(ScriptPath)) + ", 'exec'))"
    Result = Connection.run_command(Command, unattended=True)
    Result.pop("command", None)
    print(json.dumps(Result, ensure_ascii=True))
    if not Result.get("success", False):
        raise SystemExit(1)
finally:
    Connection.stop()
