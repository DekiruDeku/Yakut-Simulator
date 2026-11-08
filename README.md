# Yakut-Simulator

Запуск проекта: `powershell -ExecutionPolicy Bypass -File .\Start-YakutSimulator.ps1`.

Запуск с проверенным вводом Meta XR Simulator: `powershell -ExecutionPolicy Bypass -File .\Start-YakutSimulator.ps1 -Simulator`. Сначала закройте редактор проекта. При первом запуске загрузится официальный runtime 69; особенности режима описаны в [инструкции](Docs/FishingSceneSetup.ru.md#meta-xr-simulator).

В Rider открывайте `YakutSimulator/YakutSimulator.uproject`. Настройка встроенного .NET 10 описана в инструкции ниже.

Рыбалка переведена на общую архитектуру мини-игр. Стартовый уровень — `/Game/MiniGames/Fishing/Maps/L_Fishing`, настройки — `DA_Fishing_Default`, последовательность — `ST_Fishing_Default`.

[Инструкция дизайнера: сборка уровня, меши, параметры, StateTree и управление](Docs/MiniGameDesignerGuide.ru.md).

[Архитектура и критерии проверки](Docs/MiniGameArchitecturePlan.ru.md). Общие классы находятся в `Source/YakutSimulator/{Public,Private}/Common`, механики рыбалки — в `Fishing`, редакторские инструменты и тесты — в модуле `YakutSimulatorEditor`.

Диагностика: `LogMiniGame`, `LogInteraction`, `LogFishing`; команды `MiniGame.DumpState`, `Fishing.DumpState`. Автотесты в Session Frontend → Automation → `Yakut.MiniGames`.

Собранная игра: `Start-YakutSimulator.ps1 -Packaged`; с симулятором: `Start-YakutSimulator.ps1 -Simulator -Packaged`. [Итоги миграции и проверки](Docs/MiniGameArchitectureImplementation.ru.md).
