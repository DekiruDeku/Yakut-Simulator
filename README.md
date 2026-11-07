# Yakut-Simulator

Запуск проекта: `powershell -ExecutionPolicy Bypass -File .\Start-YakutSimulator.ps1`.

Запуск с проверенным вводом Meta XR Simulator: `powershell -ExecutionPolicy Bypass -File .\Start-YakutSimulator.ps1 -Simulator`. Сначала закройте редактор проекта. При первом запуске загрузится официальный runtime 69; особенности режима описаны в [инструкции](Docs/FishingSceneSetup.ru.md#meta-xr-simulator).

В Rider открывайте `YakutSimulator/YakutSimulator.uproject`. Настройка встроенного .NET 10 описана в инструкции ниже.

Рыболовный прототип: [сборка уровня, управление и проверка](Docs/FishingSceneSetup.ru.md).

Диагностика взаимодействий: категория `LogFishing` в Output Log, команда `Fishing.DumpState`. [Описание событий и причин отказа](Docs/FishingSceneSetup.ru.md#подробный-журнал-событий).
