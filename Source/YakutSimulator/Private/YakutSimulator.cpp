#include "Modules/ModuleManager.h"

#if WITH_EDITOR
#include "Minigames/Clinic/ClinicCaseDetails.h"
#endif

class FYakutSimulatorModule : public FDefaultGameModuleImpl
{
#if WITH_EDITOR
    virtual void StartupModule() override
    {
        RegisterClinicCaseDetails();
    }

    virtual void ShutdownModule() override
    {
        UnregisterClinicCaseDetails();
    }
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FYakutSimulatorModule, YakutSimulator, "YakutSimulator");
