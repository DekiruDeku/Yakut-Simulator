# Диалоги и квесты

Yakut Simulator включает исходную версию Narrative 3 из Battle Symphony как проектный плагин [Plugins/Narrative](../Plugins/Narrative/Narrative.uplugin). В нём находятся игровой модуль, редакторы Dialogue Blueprint и Quest Blueprint и общие ассеты по пути `/Narrative`.

Чтобы создать сценарий, добавьте `Narrative Component` в Blueprint контроллера или Pawn, который владеет прогрессом игрока. Создайте `Dialogue Blueprint` либо `Quest Blueprint` и передайте его класс компоненту через `Begin Dialogue` или `Begin Quest`. Для представления реплик доступны события компонента, включая `OnDialogueRepliesAvailable`. Выбранный вариант передаётся через `Select Dialogue Option`. Квесты получают события через `Complete Narrative Data Task` или собственные задачи на основе `Narrative Task`.

Игровые виджеты диалогов и журнала не входят в плагин Yakut. Виджеты и текстуры узлов, нужные редакторам графов, находятся в `/Narrative/NarrativeUI/Widgets/Editor`. Карты и действующие сценарии Yakut не запускают Narrative автоматически.

## Что перенесено из Battle Symphony

- Стандартные условия, события, задачи и поддержка диалоговой камеры. Из проектных ассетов перенесены автономная задача ожидания `BPT_WaitTime`, событие `BPT_MechanismInteract` и необходимый ему интерфейс `I_MechanismInteracting`.
- Событие `NE_CompleteNarrativeTask` использует `Narrative Data Task`. В `NE_RestartQuest` сохранён необязательный ID начального состояния. Пустой ID передаёт в API пустой массив и начинает квест с корневого узла.
- Сюжетные графы Battle Symphony, их локализация, карты, акторы и виджеты остались в исходном проекте. Задача `BPT_OverlapSoftTrigger` относится к отрядам и триггерам Battle Symphony. Редирект `Custom` отсутствует в копии, потому что его целевой пакет `OneOffTask` отсутствует в исходном проекте.

Описание плагина разрешает сборку игрового модуля под целевую платформу проекта. Редакторские модули поддерживают Win64. В UE 5.8.3 проверены сборки Win64 и компиляция ассетов. Cook, Android и выполнение сценариев в игре не проверялись.
