import copy
import json

Folder = '/Game/Minigames/Clinic'
CreatedAssets = []
ActionFolders = {'Question':'Actions/Questions', 'Examination':'Actions/Examinations', 'Investigation':'Actions/Investigations'}


def Metadata(Path):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.get_metadata_tags', json.dumps({'asset_path': Path}))['returnValue']


def SetMetadata(Path, Tags):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.update_metadata_tags',
        json.dumps({'asset_path': Path, 'set_tags': Tags}))


def HasMetadata(Path, Name, Value):
    Tags = Metadata(Path)
    return Name in Tags and Tags[Name] == Value

Sources = {
    'Respiratory': 'https://www.cdc.gov/antibiotic-use/hcp/clinical-care/adult-outpatient.html',
    'Pneumonia': 'https://www.mediasphera.ru/issues/respiratornaya-meditsina/2025/3/1303448672025031006',
    'Iron': 'https://www.hematology.org/resources/patients/anemia/iron-deficiency',
    'Diabetes': 'https://www.niddk.nih.gov/health-information/diabetes/overview/tests-diagnosis',
    'Injury': 'https://www.nhs.uk/conditions/broken-arm-or-wrist/',
}


def Exists(Path):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.exists', json.dumps({'path': Path}))['returnValue']


def Load(Path):
    return execute_tool('editor_toolset.toolsets.asset.AssetTools.load_asset', json.dumps({'asset_path': Path}))['returnValue']


def Compile(Blueprint):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.compile_blueprint',
        json.dumps({'blueprint': Blueprint, 'warnings_as_errors': True}))


def SetProperties(Instance, Values):
    Changed = execute_tool('editor_toolset.toolsets.object.ObjectTools.set_properties',
        json.dumps({'instance': Instance, 'values': json.dumps(Values, ensure_ascii=False)}))['returnValue']
    if not Changed:
        raise RuntimeError('Failed to set properties: ' + Instance['refPath'])


def GetProperties(Instance, Properties):
    Result = execute_tool('editor_toolset.toolsets.object.ObjectTools.get_properties',
        json.dumps({'instance': Instance, 'properties': Properties}))['returnValue']
    return json.loads(Result)


def Save(Asset):
    Saved = execute_tool('editor_toolset.toolsets.asset.AssetTools.save_assets',
        json.dumps({'asset_paths': [Asset['refPath']]}))['returnValue']
    if not Saved:
        raise RuntimeError('Failed to save asset: ' + Asset['refPath'])


def Blueprint(Name, Parent, Subfolder='Blueprints'):
    Path = Folder + '/' + Subfolder + '/' + Name
    if Exists(Path):
        return Load(Path), False
    Asset = execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.create', json.dumps({
        'folder_path': Folder + '/' + Subfolder, 'asset_name': Name, 'asset_type': {'refPath': Parent}}))['returnValue']
    CreatedAssets.append(Asset['refPath'])
    Compile(Asset)
    Save(Asset)
    return Asset, True


def DataAsset(Name, Parent, Subfolder):
    Path = Folder + '/Data/' + Subfolder + '/' + Name
    if Exists(Path):
        return Load(Path), False
    Asset = execute_tool('editor_toolset.toolsets.data_asset.DataAssetTools.create', json.dumps({
        'folder_path': Folder + '/Data/' + Subfolder, 'asset_name': Name, 'asset_type': {'refPath': Parent}}))['returnValue']
    CreatedAssets.append(Asset['refPath'])
    return Asset, True


def Indicators():
    Path = Folder + '/Data/Indicators/DT_ClinicIndicators'
    if Exists(Path):
        return Load(Path)
    Table = execute_tool('editor_toolset.toolsets.data_table.DataTableTools.create', json.dumps({
        'folder_path': Folder + '/Data/Indicators', 'asset_name': 'DT_ClinicIndicators',
        'schema': {'refPath': '/Script/YakutSimulator.ClinicIndicatorRow'}}))['returnValue']
    Numbers = {
        'Duration': ('Давность жалоб', 'дней'), 'MaximumTemperature': ('Максимальная температура со слов пациента', '°C'),
        'Temperature': ('Температура', '°C'), 'Pulse': ('Пульс', 'уд/мин'), 'Oxygen': ('SpO₂', '%'),
        'SystolicPressure': ('Систолическое АД', 'мм рт. ст.'), 'DiastolicPressure': ('Диастолическое АД', 'мм рт. ст.'),
        'RespiratoryRate': ('Частота дыхания', 'в минуту'), 'WeightLoss': ('Снижение веса за время болезни', 'кг'),
        'Hemoglobin': ('Гемоглобин', 'г/л'), 'Leukocytes': ('Лейкоциты', '×10⁹/л'),
        'Neutrophils': ('Нейтрофилы', '%'), 'MeanCellVolume': ('MCV', 'фл'), 'Platelets': ('Тромбоциты', '×10⁹/л'),
        'ReactiveProtein': ('С-реактивный белок', 'мг/л'), 'Ferritin': ('Ферритин', 'мкг/л'),
        'SerumIron': ('Сывороточное железо', 'мкмоль/л'), 'Transferrin': ('Трансферрин', 'г/л'),
        'IronSaturation': ('Насыщение трансферрина железом', '%'), 'Glucose': ('Глюкоза венозной плазмы натощак', 'ммоль/л'),
        'GlycatedHemoglobin': ('HbA1c', '%'),
    }
    Choices = {
        'FeverHistory': ('Лихорадка в анамнезе', ['Present', 'Absent']),
        'RespiratorySymptoms': ('Респираторные симптомы', ['Present', 'Absent']),
        'Breathlessness': ('Одышка', ['Absent', 'Exertional', 'AtRest']),
        'Fatigue': ('Слабость', ['Present', 'Absent']),
        'ThirstUrination': ('Жажда и учащённое мочеиспускание', ['Present', 'Absent']),
        'BloodLoss': ('Сведения о кровопотерях', ['Present', 'Absent']),
        'InjuryMechanism': ('Сведения о травме', ['Present', 'Absent']),
        'HistoryReview': ('Дополнительный анамнез', ['Reviewed']),
        'Medication': ('Приём лекарств', ['Present', 'Absent']),
        'LungSounds': ('Аускультация лёгких', ['Normal', 'Crackles']),
        'InjuryInspection': ('Осмотр левого предплечья', ['Normal', 'InjurySigns']),
        'Neurovascular': ('Чувствительность и состояние пальцев', ['Intact', 'Impaired']),
        'ChestRadiograph': ('Рентгенография грудной клетки', ['Normal', 'Infiltrate']),
        'BoneRadiograph': ('Рентгенография левого предплечья и запястья', ['Normal', 'Fracture']),
    }
    Rows = {}
    for Name, (DisplayName, Unit) in Numbers.items():
        Rows[Name] = {'displayName': DisplayName, 'valueType': 'Number', 'unit': Unit, 'categories': []}
    for Name, (DisplayName, Categories) in Choices.items():
        Rows[Name] = {'displayName': DisplayName, 'valueType': 'Choice', 'unit': '', 'categories': Categories}
    execute_tool('editor_toolset.toolsets.data_table.DataTableTools.add_rows', json.dumps({'data_table': Table, 'row_names': list(Rows)}))
    execute_tool('editor_toolset.toolsets.data_table.DataTableTools.set_rows',
        json.dumps({'data_table': Table, 'values': json.dumps(Rows, ensure_ascii=False)}))
    CreatedAssets.append(Table['refPath'])
    Save(Table)
    return Table


ActionSpecs = [
    ('AskOnset', 'Когда начались жалобы и как менялись?', 'Question', []),
    ('AskFever', 'Была ли температура и какая максимальная?', 'Question', []),
    ('AskRespiratorySymptoms', 'Есть ли насморк, боль в горле, кашель и мокрота?', 'Question', []),
    ('AskBreathlessness', 'Когда возникает одышка?', 'Question', []),
    ('AskFatigue', 'Как давно появилась слабость, как переносится нагрузка?', 'Question', []),
    ('AskThirstAndUrination', 'Изменились ли жажда, мочеиспускание и вес?', 'Question', []),
    ('AskBleedingAndDiet', 'Были ли кровопотери, изменилось ли питание?', 'Question', []),
    ('AskInjury', 'Как произошла травма, где болит и как двигается рука?', 'Question', []),
    ('AskHistory', 'Хронические болезни, похожие эпизоды, аллергии, курение?', 'Question', []),
    ('AskMedication', 'Какие лекарства принимали и когда?', 'Question', []),
    ('ReadTriageCard', 'Получить данные предварительного осмотра', 'Examination', []),
    ('MeasureTemperature', 'Измерить температуру', 'Examination', [{'poseIdentifier':'Mouth', 'areas':['Mouth']}]),
    ('MeasurePulseAndOxygen', 'Измерить пульс и SpO₂', 'Examination', [{'poseIdentifier':'Hand', 'areas':['LeftFinger']}]),
    ('MeasureBloodPressure', 'Измерить артериальное давление', 'Examination', [{'poseIdentifier':'Hand', 'areas':['LeftUpperArm']}]),
    ('AuscultateChest', 'Прослушать лёгкие', 'Examination', [
        {'poseIdentifier':'ChestFront', 'areas':['LeftChest', 'RightChest']},
        {'poseIdentifier':'ChestBack', 'areas':['LeftBack', 'RightBack']}]),
    ('InspectInjury', 'Осмотреть левое предплечье', 'Examination', [{'poseIdentifier':'Hand', 'areas':['LeftForearm']}]),
    ('OrderBloodCount', 'Общий анализ крови', 'Investigation', []),
    ('OrderCRP', 'С-реактивный белок', 'Investigation', []),
    ('OrderIronStudies', 'Показатели обмена железа', 'Investigation', []),
    ('OrderPlasmaGlucose', 'Глюкоза венозной плазмы натощак', 'Investigation', []),
    ('OrderHbA1c', 'HbA1c', 'Investigation', []),
    ('OrderChestRadiograph', 'Рентгенография грудной клетки', 'Investigation', []),
    ('OrderForearmRadiograph', 'Рентгенография левого предплечья и запястья', 'Investigation', []),
]

DiseaseSpecs = [
    ('ViralInfection', 'ОРВИ', 'Насморк, боль в горле, кашель и недомогание. Сопоставьте историю и осмотр; оцените признаки осложнений. Конкретный вирус в этом прототипе не определяется.', 'Respiratory'),
    ('Pneumonia', 'Внебольничная пневмония', 'Сопоставьте кашель, лихорадку, дыхательные показатели, локальные изменения при осмотре и инфильтрацию на рентгенограмме. Один показатель или цвет мокроты не устанавливает диагноз.', 'Pneumonia'),
    ('IronDeficiency', 'Железодефицитная анемия', 'Слабость и снижение переносимости нагрузки требуют исследования. Снижение гемоглобина выявляет анемию; для дефицита железа нужны показатели обмена железа с учётом воспаления и истории кровопотерь.', 'Iron'),
    ('Diabetes', 'Впервые выявленный сахарный диабет', 'Жажда, учащённое мочеиспускание, похудение и слабость требуют лабораторной проверки. Диабет подтверждают принятыми лабораторными критериями; в этом прототипе подготовлены глюкоза натощак и HbA1c. Тип диабета по внешности не определяется.', 'Diabetes'),
    ('Contusion', 'Ушиб мягких тканей предплечья', 'Оцените механизм травмы, локализацию боли, функцию, кожу и состояние пальцев. Исследование назначают по клиническому подозрению. Нормальный снимок сам по себе не исключает все повреждения.', 'Injury'),
    ('RadiusFracture', 'Закрытый перелом дистального отдела лучевой кости', 'Оцените механизм травмы, локальную боль, отёк, функцию, целостность кожи и состояние пальцев. Костные изменения оцениваются на снимке; требуется профильная помощь. Вправление и лечение в прототип не входят.', 'Injury'),
]

Profiles = [
    dict(Identifier='ViralInfection', Name='Алексей', Age=24, Sex='мужской', Days=2, Temperature=37.3, MaximumTemperature=37.5, Pulse=82, Oxygen=98, Pressure=[118,74], Rate=16, Blood=[142,6.4,55,89,245], Protein=3, Iron=[65,17,2.5,24], Glucose=5.0, Glycated=5.2,
         Complaint='Насморк, першит в горле, кашляю и чувствую слабость.',
         Respiratory='Два дня насморк, першение в горле и сухой кашель. За последние сутки хуже не стало.',
         Breathlessness='Absent', Fatigue=True, Medication=False),
    dict(Identifier='Pneumonia', Name='Михаил', Age=42, Sex='мужской', Days=5, Temperature=38.4, MaximumTemperature=39.0, Pulse=102, Oxygen=94, Pressure=[116,72], Rate=22, Blood=[145,13.2,82,89,280], Protein=85, Iron=[120,17,2.6,23], Glucose=5.3, Glycated=5.3,
         Complaint='Кашель, слабость, несколько дней поднимается температура.',
         Respiratory='Пятый день кашляю с мокротой. Держится температура, за последние два дня стало хуже.',
         Breathlessness='Exertional', Fatigue=True, Medication=True),
    dict(Identifier='IronDeficiency', Name='Мария', Age=29, Sex='женский', Days=60, Temperature=36.6, MaximumTemperature=36.8, Pulse=96, Oxygen=98, Pressure=[112,70], Rate=18, Blood=[96,6.3,56,70,350], Protein=2, Iron=[8,5,3.8,7], Glucose=5.0, Glycated=5.2,
         Complaint='Последние месяцы устаю, трудно подниматься по лестнице.',
         Respiratory='Насморка, боли в горле и кашля нет.',
         Breathlessness='Exertional', Fatigue=True, Medication=False),
    dict(Identifier='Diabetes', Name='Сергей', Age=38, Sex='мужской', Days=42, Temperature=36.7, MaximumTemperature=36.9, Pulse=88, Oxygen=98, Pressure=[126,80], Rate=17, Blood=[148,6.7,58,88,260], Protein=2, Iron=[75,18,2.5,26], Glucose=8.6, Glycated=7.4,
         Complaint='Стало меньше сил, часто чувствую себя уставшим.',
         Respiratory='Насморка, боли в горле и кашля нет.',
         Breathlessness='Absent', Fatigue=True, Medication=False),
    dict(Identifier='Contusion', Name='Ольга', Age=31, Sex='женский', Days=1, Temperature=36.6, MaximumTemperature=36.8, Pulse=78, Oxygen=99, Pressure=[120,76], Rate=16, Blood=[134,6.0,54,88,240], Protein=2, Iron=[55,16,2.6,23], Glucose=4.9, Glycated=5.1,
         Complaint='После падения болит левое предплечье.',
         Respiratory='Кашля, насморка и боли в горле нет.',
         Breathlessness='Absent', Fatigue=False, Medication=False),
    dict(Identifier='RadiusFracture', Name='Николай', Age=46, Sex='мужской', Days=1, Temperature=36.6, MaximumTemperature=36.8, Pulse=92, Oxygen=99, Pressure=[124,78], Rate=18, Blood=[146,7.0,60,89,250], Protein=3, Iron=[90,18,2.7,25], Glucose=5.1, Glycated=5.2,
         Complaint='Упал вчера, болит левая рука возле запястья.',
         Respiratory='Кашля, насморка и боли в горле нет.',
         Breathlessness='Absent', Fatigue=False, Medication=False),
]


def Observation(Table, Identifier, Indicator, Action, Area='', Number=0, Category='', Description='', Reference=None):
    Value = {'ObservationIdentifier': Identifier, 'Indicator': {'DataTable': Table['refPath'], 'RowName': Indicator},
        'NumericValue': float(Number), 'CategoryValue': Category, 'AreaIdentifier': Area, 'SourceAction': Action, 'Description': Description}
    if Reference is not None:
        Value['bHasReferenceRange'] = True
        Value['ReferenceMinimum'] = Reference[0]
        Value['ReferenceMaximum'] = Reference[1]
    return Value


def CaseOutcomes(Table, Profile):
    Outcomes = []
    Identifier = Profile['Identifier']
    IsRespiratory = Identifier in ['ViralInfection', 'Pneumonia']
    IsInjury = Identifier in ['Contusion', 'RadiusFracture']

    def Add(Action, Response, Values, Area=''):
        Outcome = {'ActionIdentifier': Action, 'AreaIdentifier': Area, 'Response': Response, 'Observations': Values}
        Necessary = {
            'ViralInfection': [],
            'Pneumonia': ['OrderBloodCount', 'OrderCRP', 'OrderChestRadiograph'],
            'IronDeficiency': ['OrderBloodCount', 'OrderIronStudies', 'OrderCRP'],
            'Diabetes': ['OrderPlasmaGlucose', 'OrderHbA1c', 'OrderBloodCount'],
            'Contusion': ['OrderForearmRadiograph'],
            'RadiusFracture': ['OrderForearmRadiograph'],
        }
        Unjustified = Action.startswith('Order') and Action not in Necessary[Identifier]
        Outcome['bUnjustified'] = Unjustified
        if Unjustified:
            Outcome['Rationale'] = 'В подготовленном случае это исследование не обосновано полученными жалобами и осмотром.'
        Outcomes.append(Outcome)

    def Number(Key, Action, Value, Area='', Indicator=None, Reference=None, Description=''):
        return Observation(Table, Key, Indicator or Key, Action, Area, Number=Value, Reference=Reference, Description=Description)

    def Choice(Key, Action, Category, Description, Area='', Indicator=None):
        return Observation(Table, Key, Indicator or Key, Action, Area, Category=Category, Description=Description)

    Add('AskOnset', 'Жалобы появились примерно ' + str(Profile['Days']) + ' дней назад.',
        [Number('OnsetDays', 'AskOnset', Profile['Days'], Indicator='Duration')])
    Fever = Identifier == 'Pneumonia'
    Add('AskFever', 'Максимальная температура: ' + str(Profile['MaximumTemperature']) + ' °C.',
        [Choice('FeverHistory', 'AskFever', 'Present' if Fever else 'Absent', 'Лихорадка была.' if Fever else 'Лихорадку не отмечает.'),
         Number('MaximumTemperature', 'AskFever', Profile['MaximumTemperature'])])
    Add('AskRespiratorySymptoms', Profile['Respiratory'],
        [Choice('RespiratorySymptoms', 'AskRespiratorySymptoms', 'Present' if IsRespiratory else 'Absent', Profile['Respiratory'])])
    Breathlessness = 'При подъёме по лестнице появляется одышка, в покое её нет.'
    if Profile['Breathlessness'] == 'Absent':
        Breathlessness = 'Одышки в покое и при обычной нагрузке не замечаю.'
    Add('AskBreathlessness', Breathlessness,
        [Choice('Breathlessness', 'AskBreathlessness', Profile['Breathlessness'], Breathlessness)])
    Fatigue = 'Устаю быстрее обычного, обычная нагрузка даётся труднее.' if Profile['Fatigue'] else 'Общей слабости до травмы не было.'
    Add('AskFatigue', Fatigue, [Choice('Fatigue', 'AskFatigue', 'Present' if Profile['Fatigue'] else 'Absent', Fatigue)])
    Thirst = 'Постоянно хочется пить, чаще хожу в туалет, за последние недели потерял около 4 кг.' if Identifier == 'Diabetes' else 'Жажда и мочеиспускание не менялись, заметного снижения веса нет.'
    Add('AskThirstAndUrination', Thirst,
        [Choice('ThirstUrination', 'AskThirstAndUrination', 'Present' if Identifier == 'Diabetes' else 'Absent', Thirst),
         Number('WeightLoss', 'AskThirstAndUrination', 4 if Identifier == 'Diabetes' else 0)])
    Bleeding = 'Менструации длительные и обильные. Последние месяцы питаюсь нерегулярно, мяса ем мало.' if Identifier == 'IronDeficiency' else 'Необычных кровопотерь не замечаю. Питание существенно не менялось.'
    Add('AskBleedingAndDiet', Bleeding, [Choice('BloodLoss', 'AskBleedingAndDiet', 'Present' if Identifier == 'IronDeficiency' else 'Absent', Bleeding)])
    Injury = 'Травмы руки не было, движения не ограничены.'
    if Identifier == 'Contusion':
        Injury = 'Вчера упала и ударилась серединой левого предплечья о край скамьи. Болит место удара, кистью двигаю, пальцы чувствую.'
    if Identifier == 'RadiusFracture':
        Injury = 'Вчера упал на вытянутую левую руку. Болит возле запястья, двигать кистью больно. Пальцы чувствую.'
    Add('AskInjury', Injury, [Choice('InjuryMechanism', 'AskInjury', 'Present' if IsInjury else 'Absent', Injury)])
    History = 'Ранее таких жалоб не было. Известных хронических заболеваний и аллергий нет. Не курю.'
    Add('AskHistory', History, [Choice('HistoryReview', 'AskHistory', 'Reviewed', History)])
    Medication = 'Около шести часов назад принимал парацетамол.' if Profile['Medication'] else 'До приёма лекарств от этих жалоб не принимал.'
    Add('AskMedication', Medication, [Choice('Medication', 'AskMedication', 'Present' if Profile['Medication'] else 'Absent', Medication)])
    Add('ReadTriageCard', 'Данные предварительного осмотра.',
        [Number('RespiratoryRate', 'ReadTriageCard', Profile['Rate'])])
    Add('MeasureBloodPressure', 'Измерение артериального давления завершено.',
        [Number('SystolicPressure', 'MeasureBloodPressure', Profile['Pressure'][0], Area='LeftUpperArm'),
         Number('DiastolicPressure', 'MeasureBloodPressure', Profile['Pressure'][1], Area='LeftUpperArm')], Area='LeftUpperArm')
    Add('MeasureTemperature', 'Измерение температуры завершено.',
        [Number('Temperature', 'MeasureTemperature', Profile['Temperature'], Area='Mouth')], Area='Mouth')
    Add('MeasurePulseAndOxygen', 'Измерение пульса и SpO₂ завершено.',
        [Number('Pulse', 'MeasurePulseAndOxygen', Profile['Pulse'], Area='LeftFinger'),
         Number('Oxygen', 'MeasurePulseAndOxygen', Profile['Oxygen'], Area='LeftFinger')], Area='LeftFinger')
    for Area in ['LeftChest', 'RightChest', 'LeftBack', 'RightBack']:
        Abnormal = Identifier == 'Pneumonia' and Area == 'RightBack'
        Description = 'Справа сзади в нижних отделах локальные дополнительные дыхательные звуки.' if Abnormal else 'В этой области дополнительных дыхательных шумов не выявлено.'
        AreaLabels = {'LeftChest': 'Слева спереди', 'RightChest': 'Справа спереди', 'LeftBack': 'Слева сзади', 'RightBack': 'Справа сзади'}
        Add('AuscultateChest', Description,
            [Choice('Lung' + Area, 'AuscultateChest', 'Crackles' if Abnormal else 'Normal', AreaLabels[Area] + ': ' + Description, Area=Area, Indicator='LungSounds')], Area=Area)
    Inspection = 'Кожа цела, отёка и локальной болезненности нет, движения сохранены.'
    if Identifier == 'Contusion':
        Inspection = 'Небольшой отёк и кровоподтёк в средней трети предплечья, болезненны мягкие ткани в месте удара. Движения кистью сохранены. Кожа цела.'
    if Identifier == 'RadiusFracture':
        Inspection = 'Отёк возле запястья, локальная болезненность над дистальным отделом лучевой кости, движения кистью ограничены болью. Кожа цела.'
    Add('InspectInjury', Inspection,
        [Choice('InjuryInspection', 'InspectInjury', 'InjurySigns' if IsInjury else 'Normal', Inspection, Area='LeftForearm'),
         Choice('Neurovascular', 'InspectInjury', 'Intact', 'Пальцы тёплые, обычного цвета, чувствительность сохранена.', Area='LeftForearm')], Area='LeftForearm')
    Blood = [Number(Key, 'OrderBloodCount', Value, Reference=Reference) for Key, Value, Reference in zip(
        ['Hemoglobin', 'Leukocytes', 'Neutrophils', 'MeanCellVolume', 'Platelets'], Profile['Blood'],
        [(120,160) if Profile['Sex']=='женский' else (130,170), (4,9), (40,75), (80,100), (150,400)])]
    Add('OrderBloodCount', 'Общий анализ крови: лабораторный бланк.', Blood)
    Add('OrderCRP', 'С-реактивный белок исследован отдельно от ОАК.',
        [Number('ReactiveProtein', 'OrderCRP', Profile['Protein'], Reference=(0,5))])
    Add('OrderIronStudies', 'Показатели обмена железа интерпретируются вместе с ОАК и контекстом воспаления.',
        [Number(Key, 'OrderIronStudies', Value, Reference=Reference) for Key, Value, Reference in zip(
            ['Ferritin','SerumIron','Transferrin','IronSaturation'], Profile['Iron'], [(15,150),(10,30),(2,3.6),(20,45)])])
    Add('OrderPlasmaGlucose', 'Исследована венозная плазма после ночного голодания.',
        [Number('Glucose', 'OrderPlasmaGlucose', Profile['Glucose'], Reference=(3.9,6.0), Description='Венозная плазма, натощак.')])
    GlycatedResponse = 'HbA1c. Метод и состояние пациента учитываются при интерпретации.'
    if Identifier == 'IronDeficiency':
        GlycatedResponse += ' При анемии результат HbA1c может быть ненадёжен; он не заменяет оценку глюкозы.'
    Add('OrderHbA1c', GlycatedResponse, [Number('GlycatedHemoglobin', 'OrderHbA1c', Profile['Glycated'], Reference=(4.0,5.6))])
    Chest = 'Очаговых и инфильтративных изменений не выявлено.'
    if Identifier == 'Pneumonia':
        Chest = 'В нижней доле правого лёгкого очаговая инфильтрация. Требуется сопоставление с клиническими данными.'
    Add('OrderChestRadiograph', Chest,
        [Choice('ChestRadiograph', 'OrderChestRadiograph', 'Infiltrate' if Identifier == 'Pneumonia' else 'Normal', Chest)])
    Bone = 'Левое предплечье и запястье, две проекции: костно-травматических изменений не выявлено. При сохраняющемся клиническом подозрении требуется дальнейшая оценка.'
    if Identifier == 'RadiusFracture':
        Bone = 'Левое предплечье и запястье, две проекции: линия перелома дистального отдела лучевой кости без значимого смещения.'
    Add('OrderForearmRadiograph', Bone,
        [Choice('BoneRadiograph', 'OrderForearmRadiograph', 'Fracture' if Identifier == 'RadiusFracture' else 'Normal', Bone)])
    return Outcomes


def CaseField(Data, Name, Default=None):
    return Data.get(Name, Data.get(Name[0].lower() + Name[1:], Default))


def CaseName(Value):
    if Value == 'None' or Value is None:
        return ''
    return Value


def CaseAssetPath(Value):
    if isinstance(Value, dict):
        return Value['refPath']
    return CaseName(Value)


def CaseValueKey(Fact):
    Indicator = CaseField(Fact, 'Indicator')
    return (CaseAssetPath(CaseField(Indicator, 'DataTable')), CaseField(Indicator, 'RowName'),
        CaseName(CaseField(Fact, 'AreaIdentifier')))


def ConvertClinicCase(Source, Actions, Bindings):
    Data = {'values':{}, 'answers':{}, 'specialResults':{}}
    FactIdentifiers = {}
    Initial = []
    Unjustified = []

    def AddValue(Observation, Area, Response):
        Indicator = CaseField(Observation, 'Indicator')
        Fact = {'indicator':{'dataTable':CaseAssetPath(CaseField(Indicator, 'DataTable')),
            'rowName':CaseField(Indicator, 'RowName')}, 'areaIdentifier':Area}
        Key = CaseValueKey(Fact)
        Description = CaseField(Observation, 'Description', '')
        Value = {'fact':Fact, 'numericValue':CaseField(Observation, 'NumericValue', 0),
            'categoryValue':CaseName(CaseField(Observation, 'CategoryValue')),
            'description':Description, 'image':CaseAssetPath(CaseField(Observation, 'Image')),
            'referenceRange':'Hidden', 'referenceMinimum':0, 'referenceMaximum':0}
        if Value['categoryValue'] and Description == Response:
            Value['description'] = ''
        if CaseField(Observation, 'bHasReferenceRange', False):
            Value.update(referenceRange='Custom', referenceMinimum=CaseField(Observation, 'ReferenceMinimum'),
                referenceMaximum=CaseField(Observation, 'ReferenceMaximum'))
        Identifier = CaseField(Observation, 'ObservationIdentifier')
        if Key in Data['values'] or Identifier in FactIdentifiers:
            raise RuntimeError('Cannot merge duplicate legacy fact in ' + Source['CaseIdentifier'] + ': ' + str(Key))
        Data['values'][Key] = Value
        FactIdentifiers[Identifier] = Fact
        return Fact

    for Observation in Source['InitialObservations']:
        Initial.append(AddValue(Observation, CaseName(CaseField(Observation, 'AreaIdentifier')), ''))
    Seen = set()
    for Outcome in Source['Outcomes']:
        Identifier = CaseField(Outcome, 'ActionIdentifier')
        Area = CaseName(CaseField(Outcome, 'AreaIdentifier'))
        Action, Kind, Areas = Actions[Identifier]
        Key = (Action, Area)
        if Key in Seen or Area not in (Areas or ['']):
            raise RuntimeError('Invalid legacy action/area in ' + Source['CaseIdentifier'] + ': ' + Identifier + '/' + Area)
        Seen.add(Key)
        Response = CaseField(Outcome, 'Response', '')
        Observations = CaseField(Outcome, 'Observations', [])
        Outputs = [AddValue(Value, Area, Response)['indicator'] for Value in Observations]
        if Identifier in Bindings and Bindings[Identifier] != Outputs:
            raise RuntimeError('Legacy action has different output indicators across cases or areas: ' + Identifier)
        Bindings[Identifier] = Outputs
        if CaseField(Outcome, 'bUnjustified', False):
            Unjustified.append({'action':Action, 'areaIdentifier':Area, 'rationale':CaseField(Outcome, 'Rationale', '')})
        Field = 'answers' if Kind == 'Question' else 'specialResults'
        Data[Field][Key] = {'action':Action, 'areaIdentifier':Area, 'response':Response,
            'sound':CaseAssetPath(CaseField(Outcome, 'Sound')), 'image':''}
        if Field == 'specialResults' and len(Observations) == 1:
            Value = Data['values'][CaseValueKey({'indicator':Outputs[0], 'areaIdentifier':Area})]
            Data[Field][Key]['image'] = Value['image']
            Value['image'] = ''
    if len(Seen) != sum(max(1, len(Areas)) for Action, Kind, Areas in Actions.values()):
        raise RuntimeError('Case is missing legacy results: ' + Source['CaseIdentifier'])
    return Data, {'InitialFacts':Initial, 'InitialPose':CaseName(CaseField(Source['Presentation'], 'PoseIdentifier')),
        'EvidenceSets':[{'facts':[FactIdentifiers[Identifier] for Identifier in CaseField(Group, 'Observations')]}
            for Group in Source['SufficientEvidenceSets']], 'UnjustifiedActions':Unjustified}


def SharedClinicCaseData(Plans):
    Shared = {'values':[], 'answers':[], 'specialResults':[]}
    for Field in Shared:
        for Key in Plans[0][Field]:
            if all(Key in Plan[Field] for Plan in Plans):
                Candidates = [Plan[Field][Key] for Plan in Plans]
                Default = max(Candidates, key=Candidates.count)
                if Field == 'values' or Candidates.count(Default) > 1:
                    Shared[Field].append(Default)
    return Shared


def ClinicCaseOverrides(Data, Shared, ActionIndicators):
    Overrides = {}
    BaselineCategories = {CaseValueKey(Entry['fact']):CaseName(Entry['categoryValue']) for Entry in Shared['values']}
    for Field, Entries in Data.items():
        Defaults = {}
        for Entry in Shared.get(Field, []):
            if Field == 'values':
                Key = CaseValueKey(Entry['fact'])
                Default = dict(Entry)
                Default['fact'] = {'indicator':{'dataTable':Key[0], 'rowName':Key[1]}, 'areaIdentifier':Key[2]}
                Default['categoryValue'] = CaseName(Default['categoryValue'])
                Default['image'] = CaseAssetPath(Default.get('image'))
            else:
                Key = (CaseAssetPath(Entry['action']), CaseName(Entry['areaIdentifier']))
                Default = dict(Entry, action=Key[0], areaIdentifier=Key[1])
                Default['sound'] = CaseAssetPath(Default.get('sound'))
                Default['image'] = CaseAssetPath(Default.get('image'))
            Defaults[Key] = Default
        if any(Key not in Entries for Key in Defaults):
            raise RuntimeError('The baseline would add a fact or response that this legacy case did not contain')
        Overrides[Field] = []
        for Key, Value in Entries.items():
            Default = Defaults.get(Key)
            if Value == Default:
                if Field == 'values':
                    continue
                Facts = [(Indicator['dataTable'], Indicator['rowName'], Key[1]) for Indicator in ActionIndicators[Key[0]]]
                if not any(Fact in Data['values'] and Data['values'][Fact]['categoryValue']
                    and Data['values'][Fact]['categoryValue'] != BaselineCategories.get(Fact) for Fact in Facts):
                    continue
            if Field == 'values' and Default and all(Value[Name] == Default[Name]
                for Name in ('referenceRange', 'referenceMinimum', 'referenceMaximum')):
                Value = dict(Value, referenceRange='Inherit', referenceMinimum=0, referenceMaximum=0)
            Overrides[Field].append(Value)
    return Overrides


def ClinicCaseStorage(Data):
    # Omit null assets on fresh array elements; MCP expects a valid object path for set object fields.
    return {Field:[{Name:Value for Name, Value in Entry.items() if Value or Name not in ('image','sound')}
        for Entry in Entries] for Field, Entries in Data.items()}


def ReadClinicPatientCalibration(Placement, PatientBlueprint):
    Fields = ['Body', 'Areas', 'PoseDefinitions']
    Source = GetProperties(Placement, Fields)
    Default = GetProperties(DefaultObject(PatientBlueprint), Fields)
    Transforms = ['RelativeLocation','RelativeRotation','RelativeScale3D']
    Parts = [(Source['Body'], Default['Body'], 'Body', Transforms + ['SkeletalMeshAsset','OverrideMaterials','AnimClass'])]
    DefaultAreas = {Area['areaIdentifier']: Area for Area in Default['Areas']}
    for Area in Source['Areas']:
        Name = Area['areaIdentifier']
        Template = DefaultAreas[Name]
        Parts.append((Area['zone'], Template['zone'], Name, Transforms + ['BoxExtent']))
        if isinstance(Area['highlight'], dict) and isinstance(Template['highlight'], dict):
            Parts.append((Area['highlight'], Template['highlight'], Name + 'Highlight', Transforms + ['StaticMesh','OverrideMaterials']))
    Overrides = {}
    for Component, Template, Name, Properties in Parts:
        Actual = GetProperties(Component, Properties)
        Inherited = GetProperties(Template, Properties)
        Changed = {Key:Value for Key,Value in Actual.items() if Value != Inherited[Key]}
        if Changed:
            Overrides[Name] = Changed
    Poses = {}
    if Source['PoseDefinitions'] != Default['PoseDefinitions']:
        Poses['PoseDefinitions'] = Source['PoseDefinitions']
    return Overrides, Poses


def UpgradeClinicCases(Scenario=None, NewCases=None, Seeds=None):
    if execute_tool('EditorToolset.EditorAppToolset.IsPIERunning', '{}')['returnValue']:
        raise RuntimeError('Clinic case authoring requires the editor outside PIE')
    if Scenario is None:
        Scenario = Load(Folder + '/Blueprints/BP_ClinicScenario')
    Catalog = GetProperties(Scenario, ['ActionDefinitions','PatientCases','Visits'])
    if Catalog['Visits']:
        return {'visits':len(Catalog['Visits']), 'converted':0}
    Cases = Catalog['PatientCases'] or NewCases
    if not Cases:
        raise RuntimeError('No clinic cases to migrate')
    Seeds = Seeds or {}
    Actions = {}
    ActionValues = {}
    for Ref in Catalog['ActionDefinitions']:
        Values = GetProperties(Ref, ['ActionIdentifier','ActionType','PresentationStages','RevealedIndicators'])
        Identifier = Values['ActionIdentifier']
        if Identifier in Actions:
            raise RuntimeError('Duplicate clinic action: ' + Identifier)
        Actions[Identifier] = (Ref['refPath'], Values['ActionType'],
            [Area for Stage in Values['PresentationStages'] for Area in Stage['areas']])
        ActionValues[Identifier] = Values['RevealedIndicators']
    Bindings = {}
    Plans = []
    Fields = ['CaseIdentifier','PatientName','Age','Sex','Presentation','InitialObservations','Outcomes','SufficientEvidenceSets']
    for Case in Cases:
        Source = Seeds.get(Case['refPath'])
        if Source is None:
            Source = GetProperties(Case, Fields)
        Data, Properties = ConvertClinicCase(Source, Actions, Bindings)
        if Case['refPath'] in Seeds:
            Properties.update({Name:Source[Name] for Name in ('Complaint','ExpectedDisease','Debrief')})
        Plans.append((Case, Source, Data, Properties))

    for Identifier, Old in ActionValues.items():
        Existing = [{'dataTable':CaseAssetPath(Row['dataTable']), 'rowName':Row['rowName']} for Row in Old]
        if Existing and Existing != Bindings[Identifier]:
            raise RuntimeError('The authored action result differs from its legacy cases: ' + Identifier)

    PatientBlueprint = Load(Folder + '/Blueprints/BP_ClinicPatient')
    ComponentOverrides, PatientOverrides = {}, {}
    if NewCases is None:
        Level = execute_tool('editor_toolset.toolsets.scene.SceneTools.get_current_level', '{}')['returnValue']
        if Level != '/Game/Maps/L_Clinic':
            raise RuntimeError('Open L_Clinic to preserve the placed patient calibration during migration')
        PlacedScenario, = FindActors(ActorClass='/Script/YakutSimulator.ClinicScenario')
        Placement = GetProperties(PlacedScenario, ['Patient'])['Patient']
        ComponentOverrides, PatientOverrides = ReadClinicPatientCalibration(Placement, PatientBlueprint)

    BaselineBlueprint, _ = Blueprint('BP_ClinicCaseBaseline', '/Script/YakutSimulator.ClinicCaseBaseline')
    ProfileBlueprint, _ = Blueprint('BP_ClinicPatientProfile', '/Script/YakutSimulator.ClinicPatientProfile')
    Baseline, IsNew = DataAsset('DA_ClinicCaseBaseline', BaselineBlueprint['refPath'] + '_C', 'Cases')
    if IsNew:
        Shared = SharedClinicCaseData([Plan[2] for Plan in Plans])
    else:
        Shared = GetProperties(Baseline, ['Data'])['Data']
    ActionIndicators = {Actions[Identifier][0]:Outputs for Identifier, Outputs in Bindings.items()}
    Deltas = [ClinicCaseOverrides(Plan[2], Shared, ActionIndicators) for Plan in Plans]
    if IsNew:
        SetProperties(Baseline, {'Data':ClinicCaseStorage(Shared)})
        Save(Baseline)
    for Identifier, Outputs in Bindings.items():
        if Outputs and not ActionValues[Identifier]:
            Action = {'refPath':Actions[Identifier][0]}
            SetProperties(Action, {'RevealedIndicators':Outputs})
            Save(Action)

    PatientNames = {'Алексей':'Alexey','Михаил':'Mikhail','Мария':'Maria','Сергей':'Sergey','Ольга':'Olga','Николай':'Nikolay'}
    UsedNames = set()
    Visits = []
    for (Case, Source, Data, Properties), Delta in zip(Plans, Deltas):
        Suffix = PatientNames.get(Source['PatientName'], Source['CaseIdentifier'])
        if Suffix in UsedNames:
            Suffix = Source['CaseIdentifier']
        UsedNames.add(Suffix)
        Profile, IsNew = DataAsset('DA_Patient_' + Suffix, ProfileBlueprint['refPath'] + '_C', 'Patients')
        if IsNew:
            SetProperties(Profile, {Name:Source[Name] for Name in ('PatientName','Age','Sex')})
            Save(Profile)
        Patient, IsNew = Blueprint('BP_ClinicPatient_' + Suffix, PatientBlueprint['refPath'] + '_C', 'Blueprints/Patients')
        if IsNew:
            SetProperties(Patient, dict(PatientOverrides, PatientProfile=Profile['refPath']))
            Parts = copy.deepcopy(ComponentOverrides)
            Mesh = CaseAssetPath(CaseField(Source['Presentation'], 'Mesh'))
            Material = CaseAssetPath(CaseField(Source['Presentation'], 'Material'))
            if Mesh:
                Parts.setdefault('Body', {})['SkeletalMeshAsset'] = Mesh
            if Material:
                Body = GetProperties(Patient, ['Body'])['Body']
                MaterialMesh = Mesh or CaseAssetPath(Parts.get('Body', {}).get('SkeletalMeshAsset'))
                if not MaterialMesh:
                    MaterialMesh = CaseAssetPath(GetProperties(Body, ['SkeletalMeshAsset'])['SkeletalMeshAsset'])
                if MaterialMesh:
                    Slots = GetProperties(Load(MaterialMesh), ['Materials'])['Materials']
                    Parts.setdefault('Body', {})['OverrideMaterials'] = [Material for Slot in Slots]
            if Parts:
                ComponentsByName = GetProperties(Patient, list(Parts))
                for Name, Values in Parts.items():
                    SetProperties(ComponentsByName[Name], Values)
            Compile(Patient)
            Save(Patient)
        SetProperties(Case, dict(Properties, Baseline=Baseline['refPath'], Overrides=ClinicCaseStorage(Delta)))
        Save(Case)
        Visits.append({'case':Case['refPath'], 'patientClass':Patient['refPath'] + '_C'})
    SetProperties(Scenario, {'Visits':Visits})
    Compile(Scenario)
    Save(Scenario)
    return {'visits':len(Visits), 'converted':len(Plans), 'baseline':Baseline['refPath']}


def run():
    Playing = execute_tool('EditorToolset.EditorAppToolset.IsPIERunning', '{}')['returnValue']
    if Playing:
        raise RuntimeError('PIE is running; clinic authoring is prohibited during play')
    ClassBlueprints = {}
    for ClassName in ['ClinicActionDefinition', 'ClinicDiseaseDefinition', 'ClinicPatientCaseDefinition', 'ClinicScenario', 'ClinicPatient']:
        Asset, IsNew = Blueprint('BP_' + ClassName, '/Script/YakutSimulator.' + ClassName)
        ClassBlueprints[ClassName] = Asset['refPath'] + '_C'
        if ClassName == 'ClinicPatient' and IsNew:
            Poses = ['Idle']
            for Identifier, DisplayName, Kind, Stages in ActionSpecs:
                for Stage in Stages:
                    if Stage['poseIdentifier'] not in Poses:
                        Poses.append(Stage['poseIdentifier'])
            SetProperties(Asset, {'PoseDefinitions':[{'poseIdentifier':Pose,
                'yawOffset':180 if Pose == 'ChestBack' else 0,
                'transitionSeconds':1.0 if Pose in ['Idle','ChestBack'] else 0.35} for Pose in Poses]})
            Compile(Asset)
            Save(Asset)
    Table = Indicators()
    Actions = []
    ActionBlueprint = ClassBlueprints['ClinicActionDefinition']
    for Identifier, DisplayName, Kind, Stages in ActionSpecs:
        Asset, IsNew = DataAsset('DA_Action_' + Identifier, ActionBlueprint, ActionFolders[Kind])
        if IsNew:
            SetProperties(Asset, {'ActionIdentifier': Identifier, 'DisplayName': DisplayName, 'ActionType': Kind, 'PresentationStages': Stages,
                'bUsesInstrument': Identifier in ['MeasureTemperature', 'MeasurePulseAndOxygen', 'MeasureBloodPressure', 'AuscultateChest'],
                'DurationSeconds': 2.0, 'InvestigationDelaySeconds': 5.0,
                'Instruction': DisplayName + '. При использовании прибора удерживайте Trigger и рабочую часть в нужной области.'})
            Save(Asset)
        Actions.append(Asset)
    Diseases = {}
    for Identifier, DisplayName, ReferenceText, SourceKey in DiseaseSpecs:
        Asset, IsNew = DataAsset('DA_Disease_' + Identifier, ClassBlueprints['ClinicDiseaseDefinition'], 'Diseases')
        if IsNew:
            SetProperties(Asset, {'DisplayName': DisplayName, 'ReferenceText': ReferenceText,
                'Sources': [Sources[SourceKey]]})
            Save(Asset)
        Diseases[Identifier] = Asset
    Presentation = {}
    Evidence = {
        'ViralInfection': [['OnsetDays','RespiratorySymptoms','Temperature','Oxygen','LungLeftChest','LungRightChest','RespiratoryRate']],
        'Pneumonia': [['FeverHistory','LungRightBack','ChestRadiograph'], ['FeverHistory','RespiratorySymptoms','ChestRadiograph','RespiratoryRate']],
        'IronDeficiency': [['Fatigue','Hemoglobin','MeanCellVolume','Ferritin','ReactiveProtein']],
        'Diabetes': [['ThirstUrination','Glucose','GlycatedHemoglobin']],
        'Contusion': [['InjuryMechanism','InjuryInspection','Neurovascular','BoneRadiograph']],
        'RadiusFracture': [['InjuryMechanism','InjuryInspection','Neurovascular','BoneRadiograph']],
    }
    Debriefs = {
        'ViralInfection': 'В данном учебном случае история и осмотр соответствуют ОРВИ: симптомы верхних дыхательных путей, сохранённые дыхательные показатели и отсутствие локальных изменений. Лабораторное определение конкретного вируса не требуется.',
        'Pneumonia': 'В данном случае лихорадка и кашель сочетаются с локальными изменениями справа сзади и инфильтрацией на снимке. Оценка жизненных показателей нужна для дальнейшего ведения. По снимку нельзя определить возбудителя.',
        'IronDeficiency': 'В данном случае снижены гемоглобин и MCV, ферритин низкий при отсутствии выраженного воспаления; история кровопотерь требует дальнейшей оценки. Одного снижения гемоглобина недостаточно для установления причины анемии.',
        'Diabetes': 'В данном случае характерные жалобы сочетаются с повышенной лабораторной глюкозой венозной плазмы натощак и повышенным HbA1c. Определение типа диабета и лечение требуют дальнейшего обследования.',
        'Contusion': 'В данном случае механизм прямого удара, осмотр мягких тканей, сохранённая функция и подготовленный снимок согласуются с ушибом. При сохраняющейся боли или подозрении на скрытое повреждение нужен повторный осмотр; нормальный снимок не исключает все травмы.',
        'RadiusFracture': 'В данном случае механизм падения на вытянутую руку, локальная болезненность, отёк и рентгенограмма подтверждают перелом лучевой кости. Кожа цела, чувствительность и состояние пальцев проверены. Требуется профильная помощь.',
    }
    Scenario = Load(Folder + '/Blueprints/BP_ClinicScenario')
    Existing = GetProperties(Scenario, ['ActionDefinitions','PatientCases','Visits'])
    bSeedCases = not Existing['PatientCases'] and not Existing['Visits']
    Cases = []
    Seeds = {}
    for Profile in Profiles:
        Identifier = Profile['Identifier']
        Asset, _ = DataAsset('DA_Case_' + Identifier, ClassBlueprints['ClinicPatientCaseDefinition'], 'Cases')
        if bSeedCases:
            Seeds[Asset['refPath']] = {'CaseIdentifier': Identifier, 'PatientName': Profile['Name'], 'Age': Profile['Age'], 'Sex': Profile['Sex'],
                'Complaint': Profile['Complaint'], 'ExpectedDisease': Diseases[Identifier]['refPath'],
                'Presentation': Presentation, 'InitialObservations': [], 'Outcomes': CaseOutcomes(Table, Profile),
                'SufficientEvidenceSets': [{'Observations': Set} for Set in Evidence[Identifier]],
                'Debrief': Debriefs[Identifier]}
        Cases.append(Asset)
    if not Existing['ActionDefinitions']:
        SetProperties(Scenario, {'ActionDefinitions': [Asset['refPath'] for Asset in Actions],
            'DiseaseDefinitions': [Asset['refPath'] for Asset in Diseases.values()],
            'InitialCaseIndex': 1})
        Save(Scenario)
    Report = {'actions': len(Actions), 'diseases': len(Diseases), 'cases': len(Cases),
        'createdAssets': CreatedAssets, 'gameplayTestsRun': False}
    Report['actors'] = PrepareActors()
    Report['highlights'] = ConfigureExaminationHighlights()
    Report['layout'] = BuildConsoleLayout()
    Report['bindings'] = BuildDynamicClinicConsole()
    ConnectClinicConsole(Scenario)
    Report['scene'] = BuildScene()
    Report['mapRegistration'] = RegisterClinicMap()
    Report['tonometer'] = ConfigureTonometer()
    if bSeedCases:
        Report['caseAuthoring'] = UpgradeClinicCases(Scenario, Cases, Seeds)
    else:
        Report['caseAuthoring'] = {'configured':bool(Existing['Visits'])}
    Report['referenceBook'] = ConfigureReferenceBook()
    return Report


def ListProperties(Instance):
    return json.loads(execute_tool('editor_toolset.toolsets.object.ObjectTools.list_properties',
        json.dumps({'instance': Instance}))['returnValue'])


def InspectWidget(Info):
    Widget = Info['widget']
    Properties = ListProperties(Widget)
    GetProperties(Widget, list(Properties))
    Slot = Info.get('slot')
    if isinstance(Slot, dict):
        SlotProperties = ListProperties(Slot)
        GetProperties(Slot, list(SlotProperties))
    return Properties


def GetWidgets(WidgetBlueprint):
    Tree = execute_tool('UMGToolSet.UMGToolSet.GetWidgets', json.dumps({'widgetBlueprint': WidgetBlueprint}))['returnValue']
    return {Info['widgetName']: Info for Info in Tree['widgets']}


def Widget(WidgetBlueprint, Cache, Name, ClassPath, Parent=None, NamedSlot=False):
    if Name in Cache:
        Info = Cache[Name]
    elif NamedSlot:
        Info = execute_tool('UMGToolSet.UMGToolSet.SetNamedSlotContent', json.dumps({
            'widgetBlueprint': WidgetBlueprint, 'hostWidget': Parent, 'slotName': 'ButtonContent',
            'widgetClass': {'refPath': ClassPath}, 'widgetName': Name}))['returnValue']
    else:
        Info = execute_tool('UMGToolSet.UMGToolSet.AddWidget', json.dumps({
            'widgetBlueprint': WidgetBlueprint, 'widgetClass': {'refPath': ClassPath},
            'widgetDisplayName': Name, 'parentWidget': Parent, 'childIndex': -1}))['returnValue']
    Cache[Name] = Info
    InspectWidget(Info)
    return Info


def ExposeWidget(WidgetBlueprint, Info):
    return execute_tool('UMGToolSet.UMGToolSet.ToggleWidgetAsVariable', json.dumps({
        'widgetBlueprint': WidgetBlueprint, 'widget': Info['widget'], 'bIsVariable': True}))


def Text(WidgetBlueprint, Cache, Name, Parent, Caption, Style='Body', NamedSlot=False):
    Info = Widget(WidgetBlueprint, Cache, Name, '/Script/CommonUI.CommonTextBlock', Parent, NamedSlot)
    ExposeWidget(WidgetBlueprint, Info)
    SetProperties(Info['widget'], {'text': Caption, 'autoWrapText': True,
        'style': {'refPath': '/Game/Base/UI/Styles/BPS_Text_' + Style + '.BPS_Text_' + Style + '_C'}})
    if isinstance(Info.get('slot'), dict):
        SlotProperties = ListProperties(Info['slot'])
        if 'padding' in SlotProperties:
            SetProperties(Info['slot'], {'padding': {'left': 4, 'top': 5, 'right': 4, 'bottom': 5}})
    return Info


def Button(WidgetBlueprint, Cache, Name, Parent, Caption):
    Info = Widget(WidgetBlueprint, Cache, Name, '/Game/Base/UI/WBP_MainButton.WBP_MainButton_C', Parent)
    ExposeWidget(WidgetBlueprint, Info)
    Text(WidgetBlueprint, Cache, Name + 'Label', Info['widget'], Caption, 'Button', True)
    execute_tool('UMGToolSet.UMGToolSet.BindToEventProperty', json.dumps({
        'widgetBlueprint': WidgetBlueprint, 'eventName': 'OnClicked',
        'propertyName': Name, 'propertyClass': {'refPath': '/Game/Base/UI/WBP_MainButton.WBP_MainButton_C'}}))
    return Info


def ConfigureExaminationHighlights():
    Path = Folder + '/Materials/M_ClinicAreaHighlight'
    if not Exists(Path):
        Material = execute_tool('editor_toolset.toolsets.material.MaterialTools.create_material',
            json.dumps({'folder_path':Folder+'/Materials','asset_name':'M_ClinicAreaHighlight'}))['returnValue']
        Properties = ListProperties(Material)
        GetProperties(Material, [Name for Name in Properties if Name.lower() in ['blendmode','shadingmodel','twosided']])
        SetProperties(Material, {'BlendMode':'BLEND_Translucent','ShadingModel':'MSM_Unlit','TwoSided':True})
        for Search in ['VectorParameter','ScalarParameter']:
            execute_tool('editor_toolset.toolsets.material.MaterialTools.list_expression_classes',
                json.dumps({'material_or_function':Material,'search':Search}))
        for Class, Values, Output in [
            ('MaterialExpressionVectorParameter',{'ParameterName':'Color','DefaultValue':{'r':.05,'g':.65,'b':1,'a':1}},'MP_EmissiveColor'),
            ('MaterialExpressionScalarParameter',{'ParameterName':'Opacity','DefaultValue':.55},'MP_Opacity')]:
            Expression = execute_tool('editor_toolset.toolsets.material.MaterialTools.add_expression', json.dumps({
                'material_or_function':Material,'expression_class':{'refPath':'/Script/Engine.'+Class}}))['returnValue']
            Properties = ListProperties(Expression)
            GetProperties(Expression, [Name for Name in Properties if Name.lower() in ['parametername','defaultvalue','group']])
            Values['Group'] = 'ClinicGuidance'
            SetProperties(Expression, Values)
            execute_tool('editor_toolset.toolsets.material.MaterialTools.connect_to_output',
                json.dumps({'expression':Expression,'output_name':'','material_property':Output}))
        execute_tool('editor_toolset.toolsets.material.MaterialTools.recompile', json.dumps({'material_or_function':Material}))
        Save(Material)
    Blueprint = Load(Folder+'/Blueprints/BP_ClinicPatient')
    Configured, Changed = ConfigurePatientHighlights(DefaultObject(Blueprint), Path)
    if Changed:
        Compile(Blueprint)
        Save(Blueprint)
    if Exists('/Game/Maps/L_Clinic'):
        CurrentLevel = execute_tool('editor_toolset.toolsets.scene.SceneTools.get_current_level', '{}')['returnValue']
        if CurrentLevel != '/Game/Maps/L_Clinic':
            execute_tool('editor_toolset.toolsets.scene.SceneTools.load_level', json.dumps({'level_path':'/Game/Maps/L_Clinic'}))
        LevelChanged = False
        for Patient in FindActors('', '/Script/YakutSimulator.ClinicPatient'):
            _, Changed = ConfigurePatientHighlights(Patient, Path)
            LevelChanged |= Changed
        if LevelChanged:
            Save(Load('/Game/Maps/L_Clinic'))
    return {'areas':Configured,'material':Path}


def ConfigurePatientHighlights(Patient, MaterialPath):
    Areas = GetProperties(Patient, ['Areas'])['Areas']
    if not Areas:
        raise RuntimeError('Expected patient examination areas')
    OwnedComponents = Components(Patient)
    Configured = []
    Linked = False
    MeshAssigned = False
    for Area in Areas:
        Highlight = Area['highlight']
        if Highlight == 'None':
            Highlight = next(Component for Component in OwnedComponents
                if Component['refPath'].rsplit(':',1)[-1].rsplit('.',1)[-1] == Area['areaIdentifier']+'Highlight')
            Area['highlight'] = Highlight
            Linked = True
        Current = GetProperties(Highlight, ['StaticMesh','OverrideMaterials'])
        if Current['StaticMesh'] == 'None':
            Extent = GetProperties(Area['zone'], ['BoxExtent'])['BoxExtent']
            # The engine sphere has a 50 cm radius; fit the existing contact box.
            SetProperties(Highlight, {'StaticMesh':'/Engine/BasicShapes/Sphere.Sphere',
                'OverrideMaterials':[MaterialPath+'.M_ClinicAreaHighlight'],
                'RelativeScale3D':{Axis:Extent[Axis]/50 for Axis in ['x','y','z']}})
            MeshAssigned = True
        Configured.append(Area['areaIdentifier'])
    if Linked:
        SetProperties(Patient, {'Areas':Areas})
    return Configured, Linked or MeshAssigned


def BuildConsoleLayout():
    Path = Folder + '/UI/WBP_ClinicConsole'
    if Exists(Path):
        WidgetBlueprint = Load(Path)
        if 'ActionsColumn' in GetWidgets(WidgetBlueprint):
            return {'widget': WidgetBlueprint, 'preserved': True}
    else:
        WidgetBlueprint = execute_tool('UMGToolSet.UMGToolSet.CreateWidgetBlueprint', json.dumps({
            'folderPath': Folder + '/UI', 'assetName': 'WBP_ClinicConsole',
            'parentClass': {'refPath': '/Script/UMG.UserWidget'}}))['returnValue']
    Cache = GetWidgets(WidgetBlueprint)
    if 'ProbeText' in Cache:
        execute_tool('UMGToolSet.UMGToolSet.RemoveWidget', json.dumps({'widgetBlueprint': WidgetBlueprint, 'widget': Cache['ProbeText']['widget']}))
        del Cache['ProbeText']
    Surface = Widget(WidgetBlueprint, Cache, 'Surface', '/Script/UMG.Border')
    SetProperties(Surface['widget'], {'brushColor': {'r': 0.018, 'g': 0.027, 'b': 0.035, 'a': 1},
        'padding': {'left': 20, 'top': 20, 'right': 20, 'bottom': 20}})
    Content = Widget(WidgetBlueprint, Cache, 'Content', '/Script/UMG.HorizontalBox', Surface['widget'])
    Left = Widget(WidgetBlueprint, Cache, 'PatientColumn', '/Script/UMG.VerticalBox', Content['widget'])
    SetProperties(Left['slot'], {'size': {'value': 0.52, 'sizeRule': 'Fill'}, 'padding': {'left': 0,'top': 0,'right': 20,'bottom': 0}})
    Right = Widget(WidgetBlueprint, Cache, 'ActionsScroll', '/Script/UMG.ScrollBox', Content['widget'])
    SetProperties(Right['slot'], {'size': {'value': 0.48, 'sizeRule': 'Fill'}})
    Actions = Widget(WidgetBlueprint, Cache, 'ActionsColumn', '/Script/UMG.VerticalBox', Right['widget'])
    Text(WidgetBlueprint, Cache, 'Title', Left['widget'], 'Первичный приём', 'Title')
    Text(WidgetBlueprint, Cache, 'Instruction', Left['widget'], 'Исходные данные пациента', 'Command')
    Text(WidgetBlueprint, Cache, 'Feedback', Left['widget'], '', 'Caption')
    KnownScroll = Widget(WidgetBlueprint, Cache, 'KnownScroll', '/Script/UMG.ScrollBox', Left['widget'])
    SetProperties(KnownScroll['slot'], {'size': {'value': 1, 'sizeRule': 'Fill'}})
    Text(WidgetBlueprint, Cache, 'Readout', KnownScroll['widget'], 'Данные появятся после начала приёма.', 'Body')
    Button(WidgetBlueprint, Cache, 'FinishButton', Left['widget'], 'Завершить обследование и выбрать диагноз')
    Button(WidgetBlueprint, Cache, 'RestartButton', Left['widget'], 'Повторить приём')
    Button(WidgetBlueprint, Cache, 'NextButton', Left['widget'], 'Следующий пациент')
    ReferenceBox = Widget(WidgetBlueprint, Cache, 'ReferenceSize', '/Script/UMG.SizeBox', Left['widget'])
    SetProperties(ReferenceBox['widget'], {'bOverride_HeightOverride': True, 'heightOverride': 280})
    ReferenceScroll = Widget(WidgetBlueprint, Cache, 'ReferenceScroll', '/Script/UMG.ScrollBox', ReferenceBox['widget'])
    Text(WidgetBlueprint, Cache, 'ReferenceArticleText', ReferenceScroll['widget'], 'Справочник: выберите заболевание в списке справа.', 'Body')
    Variables = execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.list_variables',
        json.dumps({'blueprint': WidgetBlueprint}))['returnValue']
    if 'Scenario' not in Variables:
        execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_object_variable', json.dumps({
            'blueprint': WidgetBlueprint, 'name': 'Scenario', 'object_class': {'refPath': '/Script/YakutSimulator.ClinicScenario'}}))
        execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.set_variable_instance_editable', json.dumps({
            'blueprint': WidgetBlueprint, 'variable_name': 'Scenario', 'instance_editable': True}))
    if 'VisitIdentifier' not in Variables:
        execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_variable',
            json.dumps({'blueprint': WidgetBlueprint, 'name': 'VisitIdentifier', 'type_name': 'int'}))
    return {'widget': WidgetBlueprint, 'widgetCount': len(Cache)}


def Components(Actor):
    return execute_tool('editor_toolset.toolsets.actor.ActorTools.get_components', json.dumps({'actor': Actor}))['returnValue']


def DefaultObject(Blueprint):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.get_default_object',
        json.dumps({'blueprint': Blueprint}))['returnValue']


def AddPart(Blueprint, Name, Shape, Location, Scale):
    Component = execute_tool('editor_toolset.toolsets.actor.ActorTools.add_component', json.dumps({
        'owner': Blueprint, 'component_type': {'refPath': '/Script/Engine.StaticMeshComponent'}, 'name': Name}))['returnValue']
    SetProperties(Component, {'StaticMesh': '/Engine/BasicShapes/' + Shape + '.' + Shape,
        'RelativeLocation': {'x': Location[0], 'y': Location[1], 'z': Location[2]},
        'RelativeScale3D': {'x': Scale[0], 'y': Scale[1], 'z': Scale[2]},
        'Mobility': 'Movable'})
    return Component


def PrepareActors():
    PatientBlueprint = Load(Folder + '/Blueprints/BP_ClinicPatient')
    PatientDefaults = DefaultObject(PatientBlueprint)
    Body = GetProperties(PatientDefaults, ['Body'])['Body']
    HasMesh = CaseAssetPath(GetProperties(Body, ['SkeletalMeshAsset'])['SkeletalMeshAsset'])
    if not HasMesh and not any('PatientHead' in Part['refPath'] for Part in Components(PatientDefaults)):
        AddPart(PatientBlueprint, 'PatientHead', 'Sphere', [0,0,164], [.24,.24,.24])
        AddPart(PatientBlueprint, 'PatientTorso', 'Cylinder', [0,0,128], [.46,.46,.52])
        AddPart(PatientBlueprint, 'PatientWaist', 'Cube', [0,0,96], [.34,.32,.25])
        for Side in [-1,1]:
            Suffix = 'Left'
            if Side == 1:
                Suffix = 'Right'
            AddPart(PatientBlueprint, 'PatientUpperArm' + Suffix, 'Cylinder', [0,Side*30,132], [.12,.12,.32])
            AddPart(PatientBlueprint, 'PatientForearm' + Suffix, 'Cylinder', [0,Side*35,103], [.10,.10,.26])
            AddPart(PatientBlueprint, 'PatientHand' + Suffix, 'Sphere', [5,Side*36,88], [.12,.12,.15])
            AddPart(PatientBlueprint, 'PatientLeg' + Suffix, 'Cylinder', [0,Side*12,46], [.16,.16,.68])
            AddPart(PatientBlueprint, 'PatientFoot' + Suffix, 'Cube', [7,Side*12,6], [.29,.15,.12])
        Compile(PatientBlueprint)
        Save(PatientBlueprint)
    InstrumentSpecs = [
        ('ClinicThermometer','MeasureTemperature','Термометр','Cylinder',[.035,.035,.12],[0,0,50]),
        ('ClinicPulseOximeter','MeasurePulseAndOxygen','Пульсоксиметр','Cube',[.10,.07,.05],[35,0,0]),
        ('ClinicStethoscope','AuscultateChest','Стетоскоп','Sphere',[.075,.075,.035],[0,0,45]),
        ('ClinicTonometer','MeasureBloodPressure','Тонометр','Cube',[.18,.12,.07],[0,0,50]),
    ]
    InstrumentBlueprints = []
    for Name, Action, Caption, Shape, Scale, WorkingPoint in InstrumentSpecs:
        Asset, IsNew = Blueprint('BP_' + Name, '/Script/YakutSimulator.ClinicInstrument')
        if IsNew:
            SetProperties(Asset, {'ActionDefinition': Folder + '/Data/' + ActionFolders['Examination'] + '/DA_Action_' + Action + '.DA_Action_' + Action,
                'DisplayName': Caption, 'ItemType': Name})
            Defaults = GetProperties(Asset, ['Body', 'WorkingPoint'])
            SetProperties(Defaults['Body'], {'StaticMesh': '/Engine/BasicShapes/' + Shape + '.' + Shape,
                'RelativeScale3D': {'x': Scale[0], 'y': Scale[1], 'z': Scale[2]}})
            SetProperties(Defaults['WorkingPoint'], {'RelativeLocation': {'x': WorkingPoint[0], 'y': WorkingPoint[1], 'z': WorkingPoint[2]}})
            Compile(Asset)
            Save(Asset)
        InstrumentBlueprints.append(Asset)
    Scenario = Load(Folder + '/Blueprints/BP_ClinicScenario')
    if not HasMetadata(Scenario['refPath'], 'ClinicDisplayConfigured', '1'):
        Display = GetProperties(Scenario, ['ConsoleDisplay'])['ConsoleDisplay']
        SetProperties(Display, {'RelativeLocation': {'x': 85, 'y': -105, 'z': 145},
            'RelativeRotation': {'pitch': 0, 'yaw': 130, 'roll': 0},
            'DrawSize': {'x': 2200, 'y': 1400}, 'RelativeScale3D': {'x': .07,'y': .07,'z': .07}})
        SetMetadata(Scenario['refPath'], {'ClinicDisplayConfigured': '1'})
        Save(Scenario)
    return {'patient': PatientBlueprint, 'instruments': InstrumentBlueprints, 'scenario': Scenario}


def Graph(Blueprint, Name):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.get_graph',
        json.dumps({'blueprint': Blueprint, 'graph_name': Name}))['returnValue']


def Nodes(Graph):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.find_nodes', json.dumps({'graph': Graph, 'title': ''}))['returnValue']


def NodeInfo(Node):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.get_node_infos', json.dumps({'nodes': [Node]}))['returnValue'][0]


def ResetGraph(Graph):
    for Node in Nodes(Graph):
        Class = execute_tool('editor_toolset.toolsets.object.ObjectTools.get_class', json.dumps({'instance': Node}))['returnValue']['refPath']
        if Class not in ['/Script/BlueprintGraph.K2Node_FunctionEntry', '/Script/BlueprintGraph.K2Node_ComponentBoundEvent']:
            execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.delete_node', json.dumps({'node': Node}))


def Discover(Graph, Type):
    execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.find_node_types',
        json.dumps({'graph': Graph, 'type_id_filter': Type.split('|')[-1], 'context_pins': []}))
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.get_node_type_pins',
        json.dumps({'graph': Graph, 'type_id': Type}))['returnValue']


def WriteGraph(Graph, Code):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.write_graph_dsl', json.dumps({'graph': Graph, 'code': Code}))


def CreateNode(Graph, Type, Position):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.create_node',
        json.dumps({'graph': Graph, 'type_id': Type, 'pos': {'x': Position[0], 'y': Position[1]}}))['returnValue']


def Connect(Output, Input):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.connect_pins', json.dumps({'output_pin': Output, 'input_pin': Input}))


def Pin(Info, Name, Direction):
    return next(Value for Value in Info[Direction] if Value['name'] == Name)


def BuildClinicReferenceHandler(WidgetBlueprint):
    Widgets = GetWidgets(WidgetBlueprint)
    if 'ReferenceText' in Widgets:
        Renamed = execute_tool('UMGToolSet.UMGToolSet.RenameWidget', json.dumps({
            'widgetBlueprint': WidgetBlueprint, 'widget': Widgets['ReferenceText']['widget'],
            'newDisplayName': 'ReferenceArticleText'}))['returnValue']
        if not Renamed:
            raise RuntimeError('Failed to rename the reference article text widget')
    Name = 'ShowClinicReferenceArticle'
    Handler = execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_function_graph',
        json.dumps({'blueprint': WidgetBlueprint, 'graph_name': Name}))['returnValue']
    Entry = next(NodeInfo(Node) for Node in Nodes(Handler) if Node['refPath'].rsplit('.', 1)[-1].startswith('K2Node_FunctionEntry'))
    if not any(Output['name'] == 'DiseaseIdentifier' for Output in Entry['output_pins']):
        execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_function_param',
            json.dumps({'graph': Handler, 'param_name': 'DiseaseIdentifier', 'param_type': 'name', 'input_param': True}))
    ResetGraph(Handler)
    ReferenceGetter = ClinicPropertyGetter(Handler, 'ClinicDiseaseDefinition', 'ReferenceText', 'Текстсправочника')
    for Type in ['Variables|Default|GetScenario', 'Variables|WBP_ClinicConsole|GetReferenceArticleText',
        'Clinic|GetDiseaseDefinitions', 'Class|ClinicDiseaseDefinition|GetDiseaseIdentifier',
        'Utilities|IsValid', 'Widget|SetText(Text)']:
        Discover(Handler, Type)
    WriteGraph(Handler, '''
(fn ShowClinicReferenceArticle (DiseaseIdentifier)
    (bind Owner (Variables|Default|GetScenario))
    (bind Article (Variables|WBP_ClinicConsole|GetReferenceArticleText))
    (Widget|SetText(Text) :self Article :InText "Статья недоступна.")
    (Utilities|IsValid Owner
        (:"Is Valid"
            (for Disease (Clinic|GetDiseaseDefinitions :self Owner)
                (Utilities|IsValid Disease
                    (:"Is Valid"
                        (if (== DiseaseIdentifier (Class|ClinicDiseaseDefinition|GetDiseaseIdentifier :self Disease))
                            (Widget|SetText(Text) :self Article
                                :InText (REFERENCE_GETTER :self Disease))
                        )
                    )
                )
            )
        )
    )
)
'''.replace('REFERENCE_GETTER', ReferenceGetter))


def ClinicBlueprint(Tool, **Arguments):
    return execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.' + Tool,
        json.dumps(Arguments))['returnValue']


def ClinicPropertyGetter(Graph, ClassName, *Names):
    Prefix = 'Class|' + ClassName + '|Get'
    Available = ClinicBlueprint('find_node_types', graph=Graph, type_id_filter=Prefix, context_pins=[])
    for Name in Names:
        if Prefix + Name in Available:
            return Prefix + Name
    raise RuntimeError('Could not find a Blueprint getter for ' + ClassName + '.' + Names[0])


def ClinicParameters(Graph, Inputs, Outputs=()):
    NodesByType = [NodeInfo(Node) for Node in Nodes(Graph)]
    Present = {(Pin['name'], Entry) for Node in NodesByType
        for Entry, NodeType, Direction in [(True, 'K2Node_FunctionEntry', 'output_pins'),
            (False, 'K2Node_FunctionResult', 'input_pins')]
        if Node['node']['refPath'].rsplit('.', 1)[-1].startswith(NodeType)
        for Pin in Node[Direction]}
    for Direction, Parameters in [(True, Inputs), (False, Outputs)]:
        for Name, Type in Parameters:
            if (Name, Direction) in Present:
                continue
            Arguments = {'graph':Graph, 'param_name':Name, 'input_param':Direction}
            if Type == 'VisitSnapshot':
                ClinicBlueprint('add_struct_function_param', **Arguments,
                    struct_type={'refPath':'/Script/YakutSimulator.ClinicVisitSnapshot'})
            elif Type.startswith('/Script/'):
                ClinicBlueprint('add_object_function_param', **Arguments, object_class={'refPath':Type})
            else:
                ClinicBlueprint('add_function_param', **Arguments, param_type=Type)


def ClinicFunction(Blueprint, Name, Inputs, Body, Outputs=()):
    Graph = ClinicBlueprint('add_function_graph', blueprint=Blueprint, graph_name=Name)
    ClinicParameters(Graph, Inputs, Outputs)
    Header = '(fn ' + Name + ' (' + ' '.join(Name for Name, Type in Inputs) + ')\n'
    WriteGraph(Graph, Header + Body + ')')
    return Graph


def InsertClinicDelegate(Graph, Type, Function, Target, Position, Previous, Following=None):
    Binding = NodeInfo(CreateNode(Graph, Type, Position))
    Delegate = NodeInfo(CreateNode(Graph, 'EventDispatchers|CreateEvent', [Position[0] - 220, Position[1] + 100]))
    Connect(Target, Pin(Binding, 'self', 'input_pins')['pin_id'])
    Connect(Pin(Delegate, 'OutputDelegate', 'output_pins')['pin_id'], Pin(Binding, 'Delegate', 'input_pins')['pin_id'])
    ClinicBlueprint('set_create_event_function', node=Delegate['node'], function_name=Function)
    for Output in Previous:
        if Following is not None:
            ClinicBlueprint('break_pins', output_pin=Output, input_pin=Following)
        Connect(Output, Pin(Binding, 'execute', 'input_pins')['pin_id'])
    if Following is not None:
        Connect(Pin(Binding, 'then', 'output_pins')['pin_id'], Following)


def BuildClinicConsoleRow():
    Row = ReferenceWidget('WBP_ClinicConsoleRow')
    if HasMetadata(Row['refPath'], 'ClinicRowVersion', '1'):
        return Row
    Cache = GetWidgets(Row)
    Column = Widget(Row, Cache, 'RowRoot', '/Script/UMG.VerticalBox')
    MainButton = Widget(Row, Cache, 'RowButton', '/Game/Base/UI/WBP_MainButton.WBP_MainButton_C', Column['widget'])
    ExposeWidget(Row, MainButton)
    Content = Widget(Row, Cache, 'RowContent', '/Script/UMG.VerticalBox', MainButton['widget'], True)
    Text(Row, Cache, 'RowTitle', Content['widget'], '', 'Button')
    Text(Row, Cache, 'RowDetail', Content['widget'], '', 'Caption')
    Events = Graph(Row, 'EventGraph')
    if not ClinicBlueprint('find_nodes', graph=Events, title='RowButton', entry_points_only=True):
        execute_tool('UMGToolSet.UMGToolSet.BindToEventProperty', json.dumps({
            'widgetBlueprint':Row, 'eventName':'OnClicked', 'propertyName':'RowButton',
            'propertyClass':{'refPath':'/Game/Base/UI/WBP_MainButton.WBP_MainButton_C'}}))
    Variables = ClinicBlueprint('list_variables', blueprint=Row)
    ClickParameters = [('Command','name'), ('Identifier','name'), ('Area','name')]
    for Name, Type in ClickParameters:
        if 'Row' + Name not in Variables:
            ClinicBlueprint('add_variable', blueprint=Row, name='Row' + Name, type_name=Type)
    Dispatchers = ClinicBlueprint('list_event_dispatchers', blueprint=Row)
    Dispatcher = next((Graph for Graph in Dispatchers if Graph['refPath'].endswith(':OnClinicRowClicked')), None)
    if Dispatcher is None:
        Dispatcher = ClinicBlueprint('add_event_dispatcher', blueprint=Row, name='OnClinicRowClicked')
    ClinicParameters(Dispatcher, ClickParameters)

    ClinicFunction(Row, 'ConfigureClinicRow', [('Title','text')] + ClickParameters, '''
        (Variables|Default|SetRowCommand Command)
        (Variables|Default|SetRowIdentifier Identifier)
        (Variables|Default|SetRowArea Area)
        (Widget|SetText(Text) :self (Variables|WBP_ClinicConsoleRow|GetRowTitle) :InText Title)
        (bind Detail (Variables|WBP_ClinicConsoleRow|GetRowDetail))
        (if (== Area (Utilities|Name|MakeLiteralName "None"))
            (Widget|SetVisibility :self Detail :InVisibility "Collapsed")
            (else (Widget|SetText(Text) :self Detail :InText Area)))''')
    ClinicFunction(Row, 'ShowInvestigationStatus', [('Status','text'), ('Available','bool'), ('Command','name')], '''
        (Variables|Default|SetRowCommand Command)
        (bind Detail (Variables|WBP_ClinicConsoleRow|GetRowDetail))
        (Widget|SetText(Text) :self Detail :InText Status)
        (Widget|SetVisibility :self Detail :InVisibility "Visible")
        (Widget|SetIsEnabled :self (Variables|WBP_ClinicConsoleRow|GetRowButton) :bInIsEnabled Available)''')
    ClinicFunction(Row, 'FindInvestigationStatus', [('Snapshot','VisitSnapshot'), ('Identifier','name')], '''
        (bind (Visit Phase Patient Complaint Age Sex Observations History Orders)
            (Utilities|Struct|BreakClinicVisitSnapshot Snapshot))
        (for Order Orders
            (bind (OrderIdentifier State Seconds Read) (Utilities|Struct|BreakClinicInvestigationOrder Order))
            (if (== OrderIdentifier Identifier)
                (if Read (return "Read"))
                (switch Utilities|FlowControl|Switch|SwitchonEClinicInvestigationState State
                    (:InProgress (return "Pending"))
                    (:Ready (return "Ready")))))
        (return "Unordered")''', [('Status','name')])
    ClinicFunction(Row, 'RefreshClinicInvestigation', [('Snapshot','VisitSnapshot'), ('Examining','bool')], '''
        (bind Status (CallFunction|FindInvestigationStatus :Snapshot Snapshot :Identifier (Variables|Default|GetRowIdentifier)))
        (if (== Status (Utilities|Name|MakeLiteralName "Read"))
            (CallFunction|ShowInvestigationStatus :Status "Результат в карте" :Available false :Command "None")
            (elif (== Status (Utilities|Name|MakeLiteralName "Ready"))
                (if Examining
                    (CallFunction|ShowInvestigationStatus :Status "Получить результат" :Available true :Command "ReadInvestigation")
                    (else (CallFunction|ShowInvestigationStatus :Status "Готово, не прочитано" :Available false :Command "None")))
                (elif (== Status (Utilities|Name|MakeLiteralName "Pending"))
                    (if Examining
                        (CallFunction|ShowInvestigationStatus :Status "Ожидание результата" :Available false :Command "None")
                        (else (CallFunction|ShowInvestigationStatus :Status "Результат не получен" :Available false :Command "None")))
                    (else
                        (if Examining
                            (CallFunction|ShowInvestigationStatus :Status "Назначить" :Available true :Command "OrderInvestigation")
                            (else (CallFunction|ShowInvestigationStatus :Status "Не назначено" :Available false :Command "None")))))))''')
    WriteGraph(Events, '''(event OnClicked(RowButton)
        (Default|CallOnClinicRowClicked :Command (Variables|Default|GetRowCommand)
            :Identifier (Variables|Default|GetRowIdentifier) :Area (Variables|Default|GetRowArea)))''')
    Compile(Row)
    SetMetadata(Row['refPath'], {'ClinicRowVersion':'1'})
    Save(Row)
    return Row


def ClinicConsoleGroups(Console):
    Cache = GetWidgets(Console)
    Parent = Cache['ActionsColumn']['widget']
    Groups = [('Question','Уточняющие вопросы'), ('Examination','Осмотр и приборы'),
        ('Investigation','Дополнительные исследования'), ('Diagnosis','Диагноз'), ('Reference','Справочник')]
    for Kind, Caption in Groups:
        HeaderName = 'Heading' + Kind
        Header = Cache.get(HeaderName)
        if Header is None:
            Header = Text(Console, Cache, HeaderName, Parent, Caption, 'Section')
        elif Kind == 'Diagnosis' and GetProperties(Header['widget'], ['Text'])['Text'] == 'Диагноз и справочник':
            SetProperties(Header['widget'], {'text':Caption})
        Name = Kind + 'Rows'
        if Name not in Cache:
            Panel = Widget(Console, Cache, Name, '/Script/UMG.VerticalBox', Parent)
            ExposeWidget(Console, Panel)
            Siblings = [Info['widgetName'] for Info in GetWidgets(Console).values() if Info['parent'] == Parent]
            execute_tool('UMGToolSet.UMGToolSet.MoveWidget', json.dumps({
                'widgetBlueprint':Console, 'widget':Panel['widget'], 'newParent':Parent,
                'childIndex':Siblings.index(HeaderName) + 1}))


def RetireStaticClinicRows(Console):
    Prefixes = ('Question_', 'Examination_', 'OrderInvestigation_', 'ReadInvestigation_', 'Diagnose_', 'Reference_')
    Events = Graph(Console, 'EventGraph')
    for Node in ClinicBlueprint('find_nodes', graph=Events, title='', entry_points_only=True):
        if NodeInfo(Node)['type_id'].startswith(tuple('AddEvent|OnClicked(' + Prefix for Prefix in Prefixes)):
            ClinicBlueprint('delete_node', node=Node)
    Cache = GetWidgets(Console)
    Parent = Cache['ActionsColumn']['widget']
    for Name, Info in Cache.items():
        if Info['parent'] == Parent and Name.startswith(Prefixes):
            execute_tool('UMGToolSet.UMGToolSet.RemoveWidget',
                json.dumps({'widgetBlueprint':Console, 'widget':Info['widget']}))


def BuildClinicCommandHandler(Console):
    Commands = [
        ('Question', '(Clinic|AskQuestion :self Owner :ActionIdentifier Identifier :ExpectedVisitIdentifier Visit)'),
        ('Examination', '(Clinic|RequestExamination :self Owner :ActionIdentifier Identifier :AreaIdentifier Area :ExpectedVisitIdentifier Visit)'),
        ('OrderInvestigation', '(Clinic|OrderInvestigation :self Owner :ActionIdentifier Identifier :ExpectedVisitIdentifier Visit)'),
        ('ReadInvestigation', '(Clinic|ReadInvestigation :self Owner :ActionIdentifier Identifier :ExpectedVisitIdentifier Visit)'),
        ('Diagnosis', '(Clinic|SubmitDecision :self Owner :Decision (Utilities|Struct|MakeClinicDecision :DiseaseIdentifier Identifier) :ExpectedVisitIdentifier Visit)'),
    ]
    Branches = ['(elif (== Command (Utilities|Name|MakeLiteralName "Reference"))\n'
        '  (CallFunction|ShowClinicReferenceArticle :DiseaseIdentifier Identifier))']
    for Index, (Mode, Call) in enumerate(Commands):
        Branches.insert(Index, '(' + ('if' if Index == 0 else 'elif') +
            ' (== Command (Utilities|Name|MakeLiteralName "' + Mode + '"))\n' +
            '  (bind (Status Message) (Utilities|Struct|BreakClinicCommandResult ' + Call + '))\n' +
            '  (Widget|SetText(Text) :self Feedback :InText Message)')
    ClinicFunction(Console, 'ExecuteClinicRow', [('Command','name'), ('Identifier','name'), ('Area','name')], '''
        (bind Owner (Variables|Default|GetScenario))
        (bind Visit (Variables|Default|GetVisitIdentifier))
        (bind Feedback (Variables|WBP_ClinicConsole|GetFeedback))
        (Utilities|IsValid Owner (:"Is Valid"
    ''' + '\n'.join(Branches) + ')' * (len(Commands) + 2))


def BuildClinicCatalogGraph(Console, Row):
    Panel = '/Script/UMG.VerticalBox'
    Add = ClinicFunction(Console, 'AddClinicRow', [('Panel',Panel), ('Title','text'), ('Command','name'),
        ('Identifier','name'), ('Area','name')], '''
        (bind Row (UserInterface|CreateWidget :Class "ROW_CLASS" :OwningPlayer (Widget|GetOwningPlayer :self self)))
        (Class|WBPClinicConsoleRow|ConfigureClinicRow :self Row :Title Title :Command Command :Identifier Identifier :Area Area)
        (Panel|AddChildtoVerticalBox :self Panel :Content Row)'''.replace('ROW_CLASS', Row['refPath'] + '_C'))
    Attachment = next(Info for Info in ClinicBlueprint('get_node_infos', nodes=Nodes(Add))
        if Info['type_id'].endswith('|AddChildToVerticalBox'))
    RowOutput = Pin(Attachment, 'Content', 'input_pins')['connected_pins'][0]
    AttachmentInput = Pin(Attachment, 'execute', 'input_pins')
    Position = [Attachment['position']['x'], Attachment['position']['y'] + 215]
    InsertClinicDelegate(Add, 'Default|BindEventtoOnClinicRowClicked', 'ExecuteClinicRow',
        RowOutput, Position, AttachmentInput['connected_pins'], AttachmentInput['pin_id'])

    Catalog = ClinicBlueprint('add_function_graph', blueprint=Console, graph_name='BuildClinicCatalogRows')
    DiseaseTitle = ClinicPropertyGetter(Catalog, 'ClinicDiseaseDefinition', 'DisplayName', 'Название')
    RequiresInstrument = ClinicPropertyGetter(Catalog, 'ClinicActionDefinition', 'UsesInstrument', 'bUsesInstrument', 'Требуетсяприбор')
    WriteGraph(Catalog, '''
        (fn BuildClinicCatalogRows ()
            (bind Owner (Variables|Default|GetScenario))
            (bind Questions (Variables|WBP_ClinicConsole|GetQuestionRows))
            (bind Examinations (Variables|WBP_ClinicConsole|GetExaminationRows))
            (bind Investigations (Variables|WBP_ClinicConsole|GetInvestigationRows))
            (bind Diagnoses (Variables|WBP_ClinicConsole|GetDiagnosisRows))
            (bind References (Variables|WBP_ClinicConsole|GetReferenceRows))
            (for Action (Clinic|GetActionDefinitions :self Owner)
                (Utilities|IsValid Action (:"Is Valid"
                    (bind Title (Class|ClinicActionDefinition|GetDisplayName :self Action))
                    (bind Identifier (Class|ClinicActionDefinition|GetActionIdentifier :self Action))
                    (switch Utilities|FlowControl|Switch|SwitchonEClinicActionType (Class|ClinicActionDefinition|GetActionType :self Action)
                        (:Question (CallFunction|AddClinicRow :Panel Questions :Title Title :Command "Question" :Identifier Identifier :Area "None"))
                        (:Investigation (CallFunction|AddClinicRow :Panel Investigations :Title Title :Command "OrderInvestigation" :Identifier Identifier :Area "None"))
                        (:Examination
                            (bind Areas (Clinic|GetAreas :self Action))
                            (if (Math|Boolean|ORBoolean (USE_INSTRUMENT :self Action) (== (Utilities|Array|Length Areas) 0))
                                (CallFunction|AddClinicRow :Panel Examinations :Title Title :Command "Examination" :Identifier Identifier :Area "None")
                                (else (for Area Areas
                                    (CallFunction|AddClinicRow :Panel Examinations :Title Title :Command "Examination" :Identifier Identifier :Area Area)))))))))
            (for Disease (Clinic|GetDiseaseDefinitions :self Owner)
                (Utilities|IsValid Disease (:"Is Valid"
                    (bind DiseaseIdentifier (Class|ClinicDiseaseDefinition|GetDiseaseIdentifier :self Disease))
                    (bind DiseaseTitle (DISEASE_TITLE :self Disease))
                    (CallFunction|AddClinicRow :Panel Diagnoses :Title DiseaseTitle :Command "Diagnosis" :Identifier DiseaseIdentifier :Area "None")
                    (CallFunction|AddClinicRow :Panel References :Title DiseaseTitle :Command "Reference" :Identifier DiseaseIdentifier :Area "None"))))
            (Variables|Default|SetCatalogBuilt true))
    '''.replace('DISEASE_TITLE', DiseaseTitle).replace('USE_INSTRUMENT', RequiresInstrument))


def BuildClinicConsoleRefresh(Console):
    ClinicFunction(Console, 'RefreshClinicControls', [('Snapshot','VisitSnapshot'), ('Examining','bool'), ('Diagnosing','bool'), ('Completed','bool')], '''
        (Widget|SetIsEnabled :self (Variables|WBP_ClinicConsole|GetQuestionRows) :bInIsEnabled Examining)
        (Widget|SetIsEnabled :self (Variables|WBP_ClinicConsole|GetExaminationRows) :bInIsEnabled Examining)
        (Widget|SetIsEnabled :self (Variables|WBP_ClinicConsole|GetDiagnosisRows) :bInIsEnabled Diagnosing)
        (Widget|SetIsEnabled :self (Variables|WBP_ClinicConsole|GetFinishButton) :bInIsEnabled Examining)
        (Widget|SetIsEnabled :self (Variables|WBP_ClinicConsole|GetNextButton) :bInIsEnabled Completed)
        (for Child (Widget|Panel|GetAllChildren :self (Variables|WBP_ClinicConsole|GetInvestigationRows))
            (bind Row (Utilities|Casting|CastToWBP_ClinicConsoleRow :Object Child)
                (:then (Class|WBPClinicConsoleRow|RefreshClinicInvestigation :self Row :Snapshot Snapshot :Examining Examining))))''')
    ClinicFunction(Console, 'RefreshReadout', [], '''
        (bind Owner (Variables|Default|GetScenario))
        (Utilities|IsValid Owner (:"Is Valid"
            (bind Snapshot (Clinic|GetVisitSnapshot :self Owner))
            (bind (Identifier Phase) (Utilities|Struct|BreakClinicVisitSnapshot Snapshot))
            (if (!= Identifier (Variables|Default|GetVisitIdentifier))
                (Widget|SetText(Text) :self (Variables|WBP_ClinicConsole|GetFeedback) :InText ""))
            (Variables|Default|SetVisitIdentifier Identifier)
            (Widget|SetText(Text) :self (Variables|WBP_ClinicConsole|GetReadout) :InText (Workstation|GetReadout :self Owner))
            (Widget|SetText(Text) :self (Variables|WBP_ClinicConsole|GetInstruction) :InText (Scenario|GetCurrentInstruction :self Owner))
            (switch Utilities|FlowControl|Switch|SwitchonEClinicVisitPhase Phase
                (:NotStarted (CallFunction|RefreshClinicControls :Snapshot Snapshot :Examining false :Diagnosing false :Completed false))
                (:Examination (CallFunction|RefreshClinicControls :Snapshot Snapshot :Examining true :Diagnosing false :Completed false))
                (:Diagnosis (CallFunction|RefreshClinicControls :Snapshot Snapshot :Examining false :Diagnosing true :Completed false))
                (:Completed (CallFunction|RefreshClinicControls :Snapshot Snapshot :Examining false :Diagnosing false :Completed true)))))''')


def BuildDynamicClinicConsole():
    Console = Load(Folder + '/UI/WBP_ClinicConsole')
    if HasMetadata(Console['refPath'], 'ClinicGraphVersion', '4'):
        return {'widget':Console, 'preserved':True}
    Row = BuildClinicConsoleRow()
    ClinicConsoleGroups(Console)
    if 'CatalogBuilt' not in ClinicBlueprint('list_variables', blueprint=Console):
        ClinicBlueprint('add_variable', blueprint=Console, name='CatalogBuilt', type_name='bool')
    Functions = ClinicBlueprint('list_graphs', blueprint=Console)
    AddRow = next((Graph for Graph in Functions if Graph['refPath'].endswith(':AddClinicRow')), None)
    if AddRow is not None:
        ResetGraph(AddRow)
    if not any(Graph['refPath'].endswith(':ShowClinicReferenceArticle') for Graph in Functions):
        BuildClinicReferenceHandler(Console)
    BuildClinicCommandHandler(Console)
    BuildClinicCatalogGraph(Console, Row)
    BuildClinicConsoleRefresh(Console)

    RetireStaticClinicRows(Console)
    Events = Graph(Console, 'EventGraph')
    WriteGraph(Events, '''
        (event UserInterface|EventConstruct
            (bind Owner (Variables|Default|GetScenario))
            (Utilities|IsValid Owner (:"Is Valid"
                (if (not (Variables|Default|GetCatalogBuilt)) (CallFunction|BuildClinicCatalogRows))
                (CallFunction|RefreshReadout))))
        (event UserInterface|EventDestruct
            (bind Owner (Variables|Default|GetScenario))
            (Utilities|IsValid Owner))
        (event OnClicked(FinishButton)
            (Clinic|FinishExamination :self (Variables|Default|GetScenario) :ExpectedVisitIdentifier (Variables|Default|GetVisitIdentifier)))
        (event OnClicked(RestartButton)
            (Workstation|RestartWorkstation :self (Variables|Default|GetScenario))
            (Widget|SetText(Text) :self (Variables|WBP_ClinicConsole|GetFeedback) :InText ""))
        (event OnClicked(NextButton)
            (Clinic|StartNextPatient :self (Variables|Default|GetScenario) :ExpectedVisitIdentifier (Variables|Default|GetVisitIdentifier))
            (Widget|SetText(Text) :self (Variables|WBP_ClinicConsole|GetFeedback) :InText ""))
    ''')
    NodesByType = {Info['type_id']: Info for Info in ClinicBlueprint('get_node_infos', nodes=Nodes(Events))}
    Refresh = next(Info for Type, Info in NodesByType.items() if Type.endswith('|RefreshReadout'))
    RefreshInput = Pin(Refresh, 'execute', 'input_pins')
    for Event, Type, Following in [('Construct','Scenario|BindEventtoOnScenarioChanged',RefreshInput),
        ('Destruct','Scenario|UnbindEventfromOnScenarioChanged',None)]:
        Entry = NodesByType['AddEvent|UserInterface|Event' + Event]
        Valid = NodeInfo(Pin(Entry, 'then', 'output_pins')['connected_pins'][0]['node'])
        Owner = Pin(Valid, 'InputObject', 'input_pins')['connected_pins'][0]
        if Following is None:
            InsertClinicDelegate(Events, Type, 'RefreshReadout', Owner,
                [Valid['position']['x'] + 300, Valid['position']['y']],
                [Pin(Valid, 'Is Valid', 'output_pins')['pin_id']])
        else:
            InsertClinicDelegate(Events, Type, 'RefreshReadout', Owner,
                [Refresh['position']['x'], Refresh['position']['y'] + 225],
                Following['connected_pins'], Following['pin_id'])
    Compile(Console)
    SetMetadata(Console['refPath'], {'ClinicLayoutVersion':'3','ClinicGraphVersion':'4'})
    Save(Console)
    return {'widget':Console, 'row':Row}


def ConnectClinicConsole(Scenario):
    Initialization = Graph(Scenario, 'EventGraph')
    EntryPoints = ClinicBlueprint('find_nodes', graph=Initialization, title='', entry_points_only=True)
    if any(NodeInfo(Node)['type_id'] == 'AddEvent|Workstation|EventInitializeConsole' for Node in EntryPoints):
        return
    WriteGraph(Initialization, """
        (event Workstation|EventInitializeConsole (Player)
            (bind Screen (UserInterface|CreateWidget :Class "/Game/Minigames/Clinic/UI/WBP_ClinicConsole.WBP_ClinicConsole_C" :OwningPlayer Player))
            (Class|WBPClinicConsole|SetScenario :self Screen :Scenario self)
            (UserInterface|SetWidget :self (Variables|Workstation|GetConsoleDisplay) :Widget Screen))
    """)
    Compile(Scenario)
    Save(Scenario)


def FindActors(Name='', ActorClass=None):
    Arguments = {'name': Name, 'tag': '', 'collision_channels': []}
    if ActorClass:
        Arguments['actor_type'] = {'refPath': ActorClass}
    return execute_tool('editor_toolset.toolsets.scene.SceneTools.find_actors', json.dumps(Arguments))['returnValue']


def SpawnAsset(Path, Name, Location, Rotation=None, Scale=None):
    Transform = {'location': {'x': Location[0], 'y': Location[1], 'z': Location[2]}}
    if Rotation:
        Transform['rotation'] = {'pitch': Rotation[0], 'yaw': Rotation[1], 'roll': Rotation[2]}
    if Scale:
        Transform['scale'] = Scale
    return execute_tool('editor_toolset.toolsets.scene.SceneTools.add_to_scene_from_asset',
        json.dumps({'asset_path': Path, 'name': Name, 'xform': Transform, 'snap_to_ground': False}))['returnValue']


def SpawnClass(Class, Name, Location):
    return execute_tool('editor_toolset.toolsets.scene.SceneTools.add_to_scene_from_class', json.dumps({
        'actor_type': {'refPath': Class}, 'name': Name, 'xform': {'location': {'x': Location[0], 'y': Location[1], 'z': Location[2]}}}))['returnValue']


def Cube(Name, Location, Scale):
    Actor = SpawnAsset('/Engine/BasicShapes/Cube', Name, Location)
    Mesh = next(Component for Component in Components(Actor) if 'StaticMeshComponent' in Component['refPath'])
    SetProperties(Mesh, {'RelativeScale3D': {'x': Scale[0], 'y': Scale[1], 'z': Scale[2]}})
    return Actor


def TonometerOutcomes(Outcomes):
    if any(Outcome['actionIdentifier'] == 'MeasureBloodPressure' for Outcome in Outcomes):
        return Outcomes
    Updated = copy.deepcopy(Outcomes)
    Triage = next((Outcome for Outcome in Updated if Outcome['actionIdentifier'] == 'ReadTriageCard'), None)
    if Triage is None:
        raise RuntimeError('Cannot migrate blood pressure without the triage outcome')
    Pressure = [Observation for Observation in Triage['observations']
        if Observation['observationIdentifier'] in ['SystolicPressure', 'DiastolicPressure']]
    if len(Pressure) != 2 or {Value['observationIdentifier'] for Value in Pressure} != {'SystolicPressure', 'DiastolicPressure'}:
        raise RuntimeError('Cannot migrate blood pressure without both existing readings')
    Triage['observations'] = [Observation for Observation in Triage['observations'] if Observation not in Pressure]
    for Observation in Pressure:
        Observation['sourceAction'] = 'MeasureBloodPressure'
        Observation['areaIdentifier'] = 'LeftUpperArm'
    Updated.append({'actionIdentifier':'MeasureBloodPressure', 'areaIdentifier':'LeftUpperArm',
        'response':'Измерение артериального давления завершено.', 'observations':Pressure})
    return Updated


def ConfigureTonometerArea(Patient, OwnerPath):
    Values = GetProperties(Patient, ['Areas','LeftUpperArm','LeftUpperArmHighlight'])
    Changed = False
    if not any(Area['areaIdentifier'] == 'LeftUpperArm' for Area in Values['Areas']):
        Values['Areas'].append({'areaIdentifier':'LeftUpperArm','zone':Values['LeftUpperArm'],
            'highlight':Values['LeftUpperArmHighlight']})
        SetProperties(Patient, {'Areas':Values['Areas']})
        Changed = True
    if not HasMetadata(OwnerPath, 'ClinicTonometerAreaConfigured', '1'):
        SetProperties(Values['LeftUpperArm'], {'RelativeLocation':{'x':14,'y':0,'z':0},
            'RelativeRotation':{'pitch':0,'yaw':0,'roll':0}, 'RelativeScale3D':{'x':1,'y':1,'z':1},
            'BoxExtent':{'x':10,'y':7,'z':7}})
        SetMetadata(OwnerPath, {'ClinicTonometerAreaConfigured':'1'})
        Changed = True
    _, HighlightsChanged = ConfigurePatientHighlights(Patient, Folder+'/Materials/M_ClinicAreaHighlight')
    return Changed or HighlightsChanged


def ConfigureTonometer():
    if execute_tool('EditorToolset.EditorAppToolset.IsPIERunning', '{}')['returnValue']:
        raise RuntimeError('PIE is running; tonometer authoring is prohibited during play')
    Scenario = Load(Folder+'/Blueprints/BP_ClinicScenario')
    Catalog = GetProperties(Scenario, ['ActionDefinitions','PatientCases','Visits'])
    Plans = []
    for Case in ([] if Catalog['Visits'] else Catalog['PatientCases']):
        Outcomes = GetProperties(Case, ['Outcomes'])['Outcomes']
        Plans.append((Case, Outcomes, TonometerOutcomes(Outcomes)))

    PatientBlueprint = Load(Folder+'/Blueprints/BP_ClinicPatient')
    PatientDefaults = DefaultObject(PatientBlueprint)
    Body = GetProperties(PatientDefaults, ['Body'])['Body']
    Mesh = GetProperties(Body, ['SkeletalMeshAsset'])['SkeletalMeshAsset']
    if isinstance(Mesh, dict):
        Sockets = execute_tool('editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools.get_socket_names',
            json.dumps({'mesh':Mesh}))['returnValue']
        if 'LeftUpperArm' not in Sockets:
            Bones = execute_tool('editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools.get_bone_names',
                json.dumps({'mesh':Mesh}))['returnValue']
            if 'upperarm_l' not in Bones:
                raise RuntimeError('Create a LeftUpperArm socket on the patient mesh before configuring the tonometer')
            execute_tool('editor_toolset.toolsets.skeletal_mesh.SkeletalMeshTools.add_socket',
                json.dumps({'mesh':Mesh,'socket_name':'LeftUpperArm','bone_name':'upperarm_l'}))
            Save(Mesh)
    if ConfigureTonometerArea(PatientDefaults, PatientBlueprint['refPath']):
        Compile(PatientBlueprint)
        Save(PatientBlueprint)

    Action, IsNew = DataAsset('DA_Action_MeasureBloodPressure',
        Folder+'/Blueprints/BP_ClinicActionDefinition.BP_ClinicActionDefinition_C', ActionFolders['Examination'])
    if IsNew:
        Values = {'ActionIdentifier':'MeasureBloodPressure','DisplayName':'Измерить артериальное давление',
            'ActionType':'Examination','PresentationStages':[{'poseIdentifier':'Hand','areas':['LeftUpperArm']}],
            'bUsesInstrument':True,'DurationSeconds':2.0,
            'Instruction':'Поднесите манжету к подсвеченной области левого плеча и удерживайте Trigger до завершения измерения.'}
        SetProperties(Action, Values)
        Save(Action)
    CatalogChanged = not any(Value['refPath'] == Action['refPath'] for Value in Catalog['ActionDefinitions'])
    if CatalogChanged:
        Catalog['ActionDefinitions'].append(Action)
        SetProperties(Scenario, {'ActionDefinitions':Catalog['ActionDefinitions']})
        Compile(Scenario)
        Save(Scenario)
    Migrated = []
    for Case, Previous, Updated in Plans:
        if Previous != Updated:
            # MCP requires array insertion and changes to existing elements in separate edits.
            SetProperties(Case, {'Outcomes':Previous+Updated[len(Previous):]})
            try:
                SetProperties(Case, {'Outcomes':Updated})
            except Exception:
                SetProperties(Case, {'Outcomes':Previous})
                raise
            Save(Case)
            Migrated.append(Case['refPath'])

    Instrument, _ = Blueprint('BP_ClinicTonometer', '/Script/YakutSimulator.ClinicInstrument')
    if not HasMetadata(Instrument['refPath'], 'ClinicTonometerConfigured', '1'):
        SetProperties(Instrument, {'ActionDefinition':Action,'DisplayName':'Тонометр','ItemType':'ClinicTonometer'})
        Parts = GetProperties(Instrument, ['Body','WorkingPoint'])
        SetProperties(Parts['Body'], {'StaticMesh':'/Game/QuarantineTent/Meshes/SM_BloodPressure_01a.SM_BloodPressure_01a',
            'RelativeScale3D':{'x':.5,'y':.5,'z':.5}})
        SetProperties(Parts['WorkingPoint'], {'RelativeLocation':{'x':0,'y':0,'z':5.5}})
        SetMetadata(Instrument['refPath'], {'ClinicTonometerConfigured':'1'})
        Compile(Instrument)
        Save(Instrument)

    Level = '/Game/Maps/L_Clinic'
    if Exists(Level):
        CurrentLevel = execute_tool('editor_toolset.toolsets.scene.SceneTools.get_current_level', '{}')['returnValue']
        if CurrentLevel == Level and CatalogChanged:
            execute_tool('editor_toolset.toolsets.scene.SceneTools.load_level', json.dumps({'level_path':'/Game/Maps/L_MainMenu'}))
        if CurrentLevel != Level or CatalogChanged:
            execute_tool('editor_toolset.toolsets.scene.SceneTools.load_level', json.dumps({'level_path':Level}))
        Patients = FindActors('', '/Script/YakutSimulator.ClinicPatient')
        Scenarios = FindActors('', '/Script/YakutSimulator.ClinicScenario')
        if len(Patients) != 1 or len(Scenarios) != 1:
            raise RuntimeError('Expected one patient and one clinic scenario')
        LevelChanged = ConfigureTonometerArea(Patients[0], Level)
        Existing = FindActors('', Instrument['refPath']+'_C')
        Tonometer = Existing[0] if Existing else SpawnAsset(Instrument['refPath'], 'ClinicTonometer', [35,32,88], Scale={'x':.5,'y':.5,'z':.5})
        Instruments = GetProperties(Scenarios[0], ['Instruments'])['Instruments']
        if Tonometer not in Instruments:
            Instruments.append(Tonometer)
            SetProperties(Scenarios[0], {'Instruments':Instruments})
            LevelChanged = True
        if LevelChanged:
            Save(Load(Level))

    return {'action':Action,'instrument':Instrument,'area':'LeftUpperArm','migratedCases':Migrated,'gameplayTestsRun':False}


def BuildScene():
    LevelPath = '/Game/Maps/L_Clinic'
    if Exists(LevelPath):
        if not HasMetadata(LevelPath, 'ClinicSceneOwner', 'therapist-prototype'):
            raise RuntimeError('Existing clinic level was not created by this authoring task; preserving it')
    else:
        Duplicated = execute_tool('editor_toolset.toolsets.asset.AssetTools.duplicate',
            json.dumps({'path': '/Engine/Maps/Templates/Template_Default', 'new_path': LevelPath}))['returnValue']
        if not Duplicated:
            raise RuntimeError('Failed to create clinic level from the engine template')
        SetMetadata(LevelPath, {'ClinicSceneOwner': 'therapist-prototype'})
    if not HasMetadata(LevelPath, 'ClinicSceneReady', '1'):
        Save(Load(LevelPath))
    CurrentLevel = execute_tool('editor_toolset.toolsets.scene.SceneTools.get_current_level', '{}')['returnValue']
    if CurrentLevel != LevelPath:
        execute_tool('editor_toolset.toolsets.scene.SceneTools.load_level', json.dumps({'level_path': LevelPath}))
    if HasMetadata(LevelPath, 'ClinicSceneReady', '1'):
        return {'level': LevelPath, 'preserved': True}
    ExistingPatient = FindActors('ClinicPatient')
    Patient = ExistingPatient[0] if ExistingPatient else SpawnAsset(Folder + '/Blueprints/BP_ClinicPatient', 'ClinicPatient', [60,10,0], [0,180,0])
    Instruments = []
    for Name, Location in [('ClinicThermometer',[35,-18,88]),('ClinicPulseOximeter',[35,0,88]),('ClinicStethoscope',[35,18,88]),('ClinicTonometer',[35,36,88])]:
        Existing = FindActors(Name)
        Path = Folder + '/Blueprints/BP_' + Name
        Defaults = GetProperties(Load(Path), ['Body'])
        Scale = GetProperties(Defaults['Body'], ['RelativeScale3D'])['RelativeScale3D']
        Actor = Existing[0] if Existing else SpawnAsset(Path, Name, Location, Scale=Scale)
        Instruments.append(Actor)
    ExistingScenario = FindActors('ClinicScenario')
    Scenario = ExistingScenario[0] if ExistingScenario else SpawnAsset(Folder + '/Blueprints/BP_ClinicScenario', 'ClinicScenario', [0,0,0])
    SetProperties(Scenario, {'Patient': Patient['refPath'], 'Instruments': [Actor['refPath'] for Actor in Instruments]})
    Starts = FindActors('', '/Script/Engine.PlayerStart')
    Start = Starts[0] if Starts else SpawnClass('/Script/Engine.PlayerStart', 'ClinicPlayerStart', [0,0,0])
    execute_tool('editor_toolset.toolsets.actor.ActorTools.set_actor_transform', json.dumps({
        'actor': Start, 'xform': {'location': {'x':0,'y':0,'z':0}, 'rotation': {'pitch':0,'yaw':0,'roll':0},
            'scale': {'x':1,'y':1,'z':1}}, 'worldspace': True}))
    if not FindActors('ClinicDeskTop'):
        Cube('ClinicDeskTop', [30,0,76], [.40,.85,.08])
        for Index, Position in enumerate([[15,-32,36],[45,-32,36],[15,32,36],[45,32,36]]):
            Cube('ClinicDeskLeg' + str(Index), Position, [.05,.05,.72])
    Save(Load(LevelPath))
    SetMetadata(LevelPath, {'ClinicSceneReady': '1'})
    Save(Load(LevelPath))
    return {'level': LevelPath, 'patient': Patient, 'instruments': Instruments, 'scenario': Scenario, 'playerStart': Start}


def RegisterClinicMap(LevelPath='/Game/Maps/L_Clinic.L_Clinic', DisplayName='Кабинет терапевта'):
    GameInstance = Load('/Game/Base/Blueprints/BP_MainGameInstance')
    Entries = GetProperties(GameInstance, ['ScenarioMaps'])['ScenarioMaps']
    if not any(CaseAssetPath(Entry['map']) == LevelPath for Entry in Entries):
        Entries.append({'displayName': DisplayName, 'map': LevelPath})
        SetProperties(GameInstance, {'ScenarioMaps': Entries})
        Save(GameInstance)
        Entries = GetProperties(GameInstance, ['ScenarioMaps'])['ScenarioMaps']
    return {'scenarioMaps': Entries}


DesignerQuestions = [
    ('AskPainCharacter', 'Где болит и как ощущается боль?', 'PainCharacter', 'Характер и место боли',
        ('Absent', 'RetrosternalPressure', 'SevereHeadache'), 'Такой боли сейчас нет.'),
    ('AskPainTrigger', 'Что происходило перед болью и изменилась ли она в покое?', 'PainTrigger', 'Обстоятельства боли',
        ('NotApplicable', 'PersistentAtRest', 'NoClearTrigger'), 'Боли, связанной с нагрузкой, не было.'),
    ('AskPainPeak', 'Как быстро боль стала самой сильной?', 'PainPeak', 'Скорость нарастания боли',
        ('NotApplicable', 'Gradual', 'Rapid'), 'Такой боли не было.'),
    ('AskPainAssociatedSymptoms', 'Что ещё случилось одновременно с болью?', 'PainAssociatedFeatures', 'Сопутствующие признаки',
        ('Absent', 'SweatingNausea', 'Vomiting'), 'Внезапного холодного пота, тошноты и рвоты не было.'),
    ('AskSimilarPain', 'Раньше бывала похожая боль?', 'PainHistory', 'Предыдущие эпизоды боли',
        ('NotApplicable', 'FirstEpisode', 'PriorDifferent'), 'Похожих эпизодов не припоминаю.'),
]

DesignerECG = ('OrderECG', 'ЭКГ', 'Electrocardiogram', 'Заключение ЭКГ',
    ('NoAcuteChanges', 'IschemicChanges'), 'Острых ишемических изменений на этой записи не выявлено.')


def DesignerTool(Tool, **Arguments):
    Result = execute_tool('editor_toolset.toolsets.' + Tool, json.dumps(Arguments, ensure_ascii=False))
    return Result['returnValue'] if 'returnValue' in Result else None


def DesignerDuplicate(Source, Destination):
    if not Exists(Destination):
        if not DesignerTool('asset.AssetTools.duplicate', path=Source, new_path=Destination):
            raise RuntimeError('Failed to duplicate asset: ' + Source)
    return Load(Destination)


def DesignerSave(Asset, Values):
    SetProperties(Asset, Values)
    SetMetadata(Asset['refPath'], {'ClinicDesignerVersion':'1'})
    Save(Asset)


def DesignerFact(Table, Name, Area=''):
    return {'indicator':{'dataTable':Table['refPath'], 'rowName':Name}, 'areaIdentifier':Area}


def DesignerExtraData(Table, Actions, Overrides):
    Data = {'values':[], 'answers':[], 'specialResults':[]}
    for Identifier, _, Row, _, Categories, Default in DesignerQuestions + [DesignerECG]:
        if Overrides is not None and Row not in Overrides:
            continue
        Category, Response = Overrides[Row] if Overrides is not None else (Categories[0], Default)
        Data['values'].append({'fact':DesignerFact(Table, Row), 'categoryValue':Category, 'description':Response})
        Messages = 'specialResults' if Identifier == 'OrderECG' else 'answers'
        Data[Messages].append({'action':Actions[Identifier]['refPath'], 'response':Response})
    return Data


def DesignerFilterData(Data, ActiveFacts, ActiveActions):
    Filtered = {
        'values':[Value for Value in Data['values'] if CaseValueKey(Value['fact']) in ActiveFacts],
        'answers':[Message for Message in Data['answers'] if CaseAssetPath(Message['action']) in ActiveActions],
        'specialResults':[Message for Message in Data['specialResults'] if CaseAssetPath(Message['action']) in ActiveActions],
    }
    return {Field:[{Key:Value for Key, Value in Entry.items() if Key not in ('sound','image') or CaseAssetPath(Value)}
        for Entry in Entries] for Field, Entries in Filtered.items()}


def DesignerCatalog(Table, Scenario):
    Included = {'AskOnset','AskFever','AskRespiratorySymptoms','AskBreathlessness','AskFatigue',
        'AskThirstAndUrination','AskBleedingAndDiet','AskHistory','AskMedication',
        'ReadTriageCard','MeasureTemperature','MeasurePulseAndOxygen','AuscultateChest','MeasureBloodPressure',
        'OrderBloodCount','OrderIronStudies','OrderPlasmaGlucose','OrderHbA1c','OrderChestRadiograph'}
    Definitions = {}
    ActiveFacts = set()
    for Action in GetProperties(Scenario, ['ActionDefinitions'])['ActionDefinitions']:
        Properties = GetProperties(Action, ['ActionIdentifier','ActionType','PresentationStages','RevealedIndicators'])
        Identifier = Properties['ActionIdentifier']
        if Identifier not in Included:
            continue
        Definitions[Identifier] = (Properties['ActionType'], Action)
        Areas = [Area for Stage in Properties['PresentationStages'] for Area in Stage['areas']] or ['']
        ActiveFacts.update((CaseAssetPath(Row['dataTable']), Row['rowName'], Area)
            for Row in Properties['RevealedIndicators'] for Area in Areas)
    ActionClass = Load(Folder + '/Blueprints/BP_ClinicActionDefinition')['refPath'] + '_C'
    for Identifier, Caption, Row, _, _, _ in DesignerQuestions + [DesignerECG]:
        Kind = 'Investigation' if Identifier == 'OrderECG' else 'Question'
        Asset, _ = DataAsset('DA_Action_Designer_' + Identifier, ActionClass, ActionFolders[Kind] + '/Designer')
        if not HasMetadata(Asset['refPath'], 'ClinicDesignerVersion', '1'):
            Properties = {'ActionIdentifier':Identifier, 'ActionType':Kind, 'DisplayName':Caption,
                'RevealedIndicators':[DesignerFact(Table, Row)['indicator']]}
            if Kind == 'Investigation':
                Properties['InvestigationDelaySeconds'] = 5
            DesignerSave(Asset, Properties)
        Definitions[Identifier] = (Kind, Asset)
        ActiveFacts.add((Table['refPath'], Row, ''))
    Actions = {Identifier:Asset for Identifier, (Kind, Asset) in Definitions.items()}
    Order = [Asset['refPath'] for Group in ('Question','Examination','Investigation')
        for Kind, Asset in Definitions.values() if Kind == Group]
    return Actions, Order, ActiveFacts


def DesignerDiseases(Table):
    Diseases = {Name:Load(Folder + '/Data/Diseases/DA_Disease_' + Name)
        for Name in ('ViralInfection','Pneumonia','IronDeficiency','Diabetes')}
    Specs = [
        ('SuspectedACS', 'Подозрение на острый коронарный синдром',
            'Характер и длительность боли в груди, сопутствующие признаки и ЭКГ помогают распознать подозрение на острый коронарный синдром. Требуется экстренная оценка. Ожидание ЭКГ не должно задерживать её, а нормальная запись сама по себе не исключает острый коронарный синдром.',
            'https://www.nice.org.uk/guidance/cg95/chapter/recommendations',
            [('ChestPressure','Давление за грудиной','PainCharacter','RetrosternalPressure'),
                ('PersistentPain','Боль сохраняется в покое','PainTrigger','PersistentAtRest'),
                ('SweatingNausea','Холодный пот и тошнота одновременно с болью','PainAssociatedFeatures','SweatingNausea'),
                ('IschemicECG','Изменения на ЭКГ, подозрительные на ишемию','Electrocardiogram','IschemicChanges')]),
        ('SuspectedSAH', 'Подозрение на субарахноидальное кровоизлияние',
            'Внезапная сильная головная боль, быстро достигшая максимума, требует экстренной оценки. Такой признак встречается при разных состояниях. Причину уточняют в стационаре.',
            'https://www.nice.org.uk/guidance/ng228/chapter/Recommendations',
            [('HeadacheSeverity','Сильная непривычная головная боль','PainCharacter','SevereHeadache'),
                ('ThunderclapOnset','Боль почти сразу достигла максимума','PainPeak','Rapid'),
                ('VomitingWithHeadache','Рвота после начала головной боли','PainAssociatedFeatures','Vomiting')]),
    ]
    DiseaseClass = Load(Folder + '/Blueprints/BP_ClinicDiseaseDefinition')['refPath'] + '_C'
    for Name, Title, Article, Source, Symptoms in Specs:
        Asset, _ = DataAsset('DA_Disease_Designer_' + Name, DiseaseClass, 'Diseases/Designer')
        if not HasMetadata(Asset['refPath'], 'ClinicDesignerVersion', '1'):
            DesignerSave(Asset, {'DiseaseIdentifier':'Designer_' + Name, 'DisplayName':Title, 'ReferenceText':Article,
                'Sources':[Source], 'Symptoms':[{'symptomIdentifier':Identifier, 'description':Caption,
                    'indicator':DesignerFact(Table, Row)['indicator'], 'categoryValues':[Category]}
                    for Identifier, Caption, Row, Category in Symptoms]})
        Diseases[Name] = Asset
    return Diseases


DesignerVisits = [
    ('ChestPain','Alexey','Алексей Воронов',57,'Мужской','SuspectedACS',
        'Давит в груди и никак не отпускает. Мне стало нехорошо.',
        'Давящая боль за грудиной сохранялась в покое и сопровождалась холодным потом и тошнотой. Этих сведений достаточно для подозрения на острый коронарный синдром и экстренной оценки. Изменения на ЭКГ усиливают подозрение. Ожидание ЭКГ не должно задерживать помощь.',
        [('PainCharacter','PainTrigger','PainAssociatedFeatures'), ('PainCharacter','PainTrigger','Electrocardiogram')]),
    ('CoughAndFever','Mikhail','Михаил Орлов',43,'Мужской','Pneumonia',
        'Кашляю уже несколько дней, поднимается температура. Кажется, сегодня стало хуже.',
        'Кашель и лихорадка сочетаются с локальными изменениями дыхательных шумов справа сзади. Если сделан снимок, он также показывает инфильтрацию справа. Сатурация и частота дыхания помогают оценить тяжесть состояния.',
        [('RespiratorySymptoms','Temperature/Mouth','LungSounds/RightBack'),
            ('FeverHistory','RespiratorySymptoms','ChestRadiograph','RespiratoryRate')]),
    ('OlderFatigue','Maria','Елена Соколова',72,'Женский','IronDeficiency',
        'В последние месяцы стало тяжело подниматься по лестнице. Сил всё меньше.',
        'Общий анализ крови показывает анемию, а низкий ферритин и другие показатели обмена железа подтверждают его дефицит. Изменение питания, хронические болезни и лекарства важны для поиска причины. Отсутствие видимой крови не исключает скрытую кровопотерю.',
        [('Fatigue','Hemoglobin','MeanCellVolume','Ferritin')]),
    ('NonspecificComplaints','Sergey','Сергей Миронов',46,'Мужской','Diabetes',
        'Быстро устаю, иногда кружится голова, стал хуже спать. Не понимаю, с чем это связано.',
        'Усталость и головокружение неспецифичны. Уточняющий вопрос выявил жажду и учащённое мочеиспускание. Повышенная глюкоза плазмы натощак и повышенный HbA1c вместе дают основания для вывода о диабете в этом случае. Тип диабета на этом приёме не определён.',
        [('ThirstUrination','Glucose','GlycatedHemoglobin')]),
    ('AcuteHeadache','Olga','Анна Волкова',48,'Женский','SuspectedSAH',
        'Сегодня сильно заболела голова. Раньше такой боли у меня не было.',
        'Сильная непривычная боль достигла максимума почти сразу. Это тревожный признак, при котором требуется экстренная оценка, в том числе для исключения субарахноидального кровоизлияния. Высокое давление само по себе не объясняет такую боль.',
        [('PainCharacter','PainPeak')]),
]


def DesignerCaseData(Name, Table, SharedTable, Actions, ActiveFacts, ActiveActions):
    Templates = {'CoughAndFever':'Pneumonia', 'OlderFatigue':'IronDeficiency', 'NonspecificComplaints':'Diabetes'}
    Data = {'values':[], 'answers':[], 'specialResults':[]}
    if Name in Templates:
        Source = Load(Folder + '/Data/Cases/DA_Case_' + Templates[Name])
        Data = DesignerFilterData(GetProperties(Source, ['Overrides'])['Overrides'], ActiveFacts, ActiveActions)
    Values = {CaseValueKey(Value['fact']):Value for Value in Data['values']}
    Answers = {CaseAssetPath(Message['action']):Message for Message in Data['answers']}

    def Value(Row, Number=0, Category='', Area=''):
        Fact = DesignerFact(SharedTable, Row, Area)
        Values[CaseValueKey(Fact)] = {'fact':Fact,'numericValue':Number,'categoryValue':Category}

    def Reply(Action, Response):
        Path = Actions[Action]['refPath']
        Answers[Path] = {'action':Path,'response':Response}

    Pain = {}
    if Name == 'ChestPain':
        Value('Duration', 0)
        Value('Pulse', 96, Area='LeftFinger')
        Value('Oxygen', 97, Area='LeftFinger')
        Value('RespiratoryRate', 20)
        Value('SystolicPressure', 144, Area='LeftUpperArm')
        Value('DiastolicPressure', 88, Area='LeftUpperArm')
        Value('Fatigue', Category='Absent')
        Reply('AskOnset','Началось около сорока минут назад и до сих пор не прошло.')
        Reply('AskFatigue','До сегодняшнего эпизода привычная нагрузка давалась как обычно.')
        Pain = {
            'PainCharacter':('RetrosternalPressure','Давит за грудиной, отдаёт в левую руку.'),
            'PainTrigger':('PersistentAtRest','Началось по дороге вверх по лестнице. Остановился и сел, но боль сохраняется.'),
            'PainPeak':('Gradual','Боль усилилась в течение нескольких минут.'),
            'PainAssociatedFeatures':('SweatingNausea','Когда заболела грудь, выступил холодный пот и затошнило.'),
            'PainHistory':('FirstEpisode','Такой длительной и сильной боли раньше не было.'),
            'Electrocardiogram':('IschemicChanges','Депрессия сегмента ST в латеральных отведениях. Изменения подозрительны на ишемию.'),
        }
    elif Name == 'OlderFatigue':
        Value('BloodLoss', Category='Absent')
        Value('Medication', Category='Present')
        Reply('AskBleedingAndDiet','Видимой крови не замечала. Последние месяцы ем меньше и нерегулярно.')
        Reply('AskHistory','Давно лечу повышенное давление. Колени беспокоят несколько лет. Слабость появилась недавно.')
        Reply('AskMedication','Регулярно пью лекарство от давления, иногда принимаю ибупрофен от боли в коленях.')
    elif Name == 'AcuteHeadache':
        Value('Duration', 0)
        Value('Pulse', 92, Area='LeftFinger')
        Value('SystolicPressure', 146, Area='LeftUpperArm')
        Value('DiastolicPressure', 86, Area='LeftUpperArm')
        Value('Fatigue', Category='Absent')
        Reply('AskOnset','Началось около получаса назад. Голова всё ещё сильно болит.')
        Reply('AskFatigue','До головной боли слабости не замечала.')
        Reply('AskMedication','После начала боли лекарств не принимала.')
        Pain = {
            'PainCharacter':('SevereHeadache','Резкая сильная боль сначала была в затылке, потом разошлась по всей голове.'),
            'PainTrigger':('NoClearTrigger','Я сидела дома. Травмы и нагрузки перед этим не было.'),
            'PainPeak':('Rapid','Почти сразу боль стала очень сильной.'),
            'PainAssociatedFeatures':('Vomiting','Вскоре после начала боли меня вырвало.'),
            'PainHistory':('PriorDifferent','Бывало, что голова побаливала к вечеру. Эта боль намного сильнее и началась совсем иначе.'),
        }
    Extra = DesignerExtraData(Table, Actions, Pain)
    return {'values':list(Values.values()) + Extra['values'], 'answers':list(Answers.values()) + Extra['answers'],
        'specialResults':Data['specialResults'] + Extra['specialResults']}


def DesignerVisit(Spec, Table, SharedTable, Actions, ActiveFacts, ActiveActions, Baseline, Diseases):
    Name, Model, PatientName, Age, Sex, Disease, Complaint, Debrief, Evidence = Spec
    PatientBlueprints = Folder + '/Blueprints/Patients'
    PatientPath = PatientBlueprints + '/Designer/BP_ClinicPatient_Designer_' + Name
    Patient = DesignerDuplicate(PatientBlueprints + '/BP_ClinicPatient_' + Model, PatientPath)
    ProfileClass = Load(Folder + '/Blueprints/BP_ClinicPatientProfile')['refPath'] + '_C'
    Profile, _ = DataAsset('DA_Patient_Designer_' + Name, ProfileClass, 'Patients/Designer')
    if not HasMetadata(Profile['refPath'], 'ClinicDesignerVersion', '1'):
        DesignerSave(Profile, {'PatientName':PatientName,'Age':Age,'Sex':Sex})
    if not HasMetadata(Patient['refPath'], 'ClinicDesignerVersion', '1'):
        DesignerSave(Patient, {'PatientProfile':Profile['refPath']})
    CaseClass = Load(Folder + '/Blueprints/BP_ClinicPatientCaseDefinition')['refPath'] + '_C'
    Case, _ = DataAsset('DA_Case_Designer_' + Name, CaseClass, 'Cases/Designer')
    if not HasMetadata(Case['refPath'], 'ClinicDesignerVersion', '1'):
        NewIndicators = {Spec[2] for Spec in DesignerQuestions + [DesignerECG]}
        Groups = []
        for Group in Evidence:
            Facts = []
            for Key in Group:
                Row, _, Area = Key.partition('/')
                Facts.append(DesignerFact(Table if Row in NewIndicators else SharedTable, Row, Area))
            Groups.append({'facts':Facts})
        Unjustified = {
            'ChestPain': ('OrderIronStudies','OrderHbA1c'),
            'CoughAndFever': ('OrderIronStudies','OrderHbA1c','OrderECG'),
            'OlderFatigue': ('OrderChestRadiograph',),
            'NonspecificComplaints': ('OrderChestRadiograph','OrderECG'),
            'AcuteHeadache': ('OrderIronStudies','OrderHbA1c','OrderChestRadiograph'),
        }
        DesignerSave(Case, {'Complaint':Complaint,'ExpectedDisease':Diseases[Disease]['refPath'],
            'Baseline':Baseline['refPath'], 'Overrides':DesignerCaseData(Name, Table, SharedTable, Actions, ActiveFacts, ActiveActions),
            'EvidenceSets':Groups, 'Debrief':Debrief,
            'UnjustifiedActions':[{'action':Actions[Identifier]['refPath'],
                'rationale':'Для решения в этом приёме исследование не было обосновано собранными данными.'}
                for Identifier in Unjustified[Name]]})
    return {'case':Case['refPath'], 'patientClass':Patient['refPath'] + '_C'}


def DesignerApplyCatalogDefaults(Level, Scenario):
    if DesignerTool('scene.SceneTools.get_current_level') != Level['refPath'].split('.', 1)[0]:
        return
    Placed, = FindActors(ActorClass='/Script/YakutSimulator.ClinicScenario')
    Names = ['Visits','ActionDefinitions','DiseaseDefinitions']
    Defaults = GetProperties(Scenario, Names)
    Applied = GetProperties(Placed, Names)
    if Applied != Defaults:
        DesignerPlaceScenario(Placed, Scenario)
        Save(Level)


def DesignerPlaceScenario(Old, Scenario, Active=None):
    Previous = GetProperties(Old, ['Patient','Instruments','ConsoleDisplay','CommandControl'])
    if Active is None:
        Transform = DesignerTool('actor.ActorTools.get_actor_transform', actor=Old)
        Active = DesignerTool('scene.SceneTools.add_to_scene_from_asset',
            asset_path=Scenario['refPath'], name='ClinicScenarioDesigner', xform=Transform)
    if not Active:
        raise RuntimeError('Could not place the designer clinic scenario')
    Settings = {'Patient':CaseAssetPath(Previous['Patient']),
        'Instruments':[CaseAssetPath(Instrument) for Instrument in Previous['Instruments']],
        'InitialCaseIndex':0}
    Command = CaseAssetPath(Previous['CommandControl'])
    if Command:
        Settings['CommandControl'] = Command
    SetProperties(Active, Settings)
    Display = GetProperties(Active, ['ConsoleDisplay'])['ConsoleDisplay']
    SetProperties(Display, GetProperties(Previous['ConsoleDisplay'],
        ['RelativeLocation','RelativeRotation','RelativeScale3D','DrawSize','Pivot','Space']))
    Book, = FindActors(ActorClass='/Script/YakutSimulator.ClinicReferenceBook')
    SetProperties(Book, {'Scenario':Active['refPath']})
    DesignerTool('scene.SceneTools.remove_from_scene', actor=Old)


def DesignerClinicMap(Scenario, CatalogChanged):
    Source = '/Game/Maps/L_Clinic'
    Path = '/Game/Maps/L_Clinic_Designer'
    Level = DesignerDuplicate(Source, Path)
    if HasMetadata(Level['refPath'], 'ClinicDesignerVersion', '1'):
        if CatalogChanged:
            DesignerApplyCatalogDefaults(Level, Scenario)
        return Level
    if DesignerTool('scene.SceneTools.get_current_level') != Path:
        if DesignerTool('asset.AssetTools.is_dirty', asset_path=Path):
            Save(Level)
        DesignerTool('scene.SceneTools.load_level', level_path=Path)
    if DesignerTool('scene.SceneTools.get_current_level') != Path:
        raise RuntimeError('Designer clinic level did not open')
    Placed = FindActors(ActorClass='/Script/YakutSimulator.ClinicScenario')
    Existing = next((Actor for Actor in Placed if 'BP_ClinicScenario_Designer_C_' in Actor['refPath']), None)
    Old = next((Actor for Actor in Placed if Actor != Existing), None)
    if Old is not None:
        DesignerPlaceScenario(Old, Scenario, Existing)
    elif Existing is not None:
        Book, = FindActors(ActorClass='/Script/YakutSimulator.ClinicReferenceBook')
        SetProperties(Book, {'Scenario':Existing['refPath']})
    else:
        raise RuntimeError('Designer clinic level has no scenario placement')
    SetMetadata(Level['refPath'], {'ClinicSceneOwner':'clinic-designer','ClinicDesignerVersion':'1'})
    Save(Level)
    return Level


def BuildClinicDesignerSlice(FirstVisitOnly=False):
    if execute_tool('EditorToolset.EditorAppToolset.IsPIERunning', '{}')['returnValue']:
        raise RuntimeError('Designer clinic authoring requires the editor outside PIE')
    TablePath = Folder + '/Data/Indicators/Designer/DT_ClinicDesignerIndicators'
    Table = Load(TablePath) if Exists(TablePath) else DesignerTool('data_table.DataTableTools.create',
        folder_path=Folder + '/Data/Indicators/Designer', asset_name='DT_ClinicDesignerIndicators',
        schema={'refPath':'/Script/YakutSimulator.ClinicIndicatorRow'})
    if not HasMetadata(Table['refPath'], 'ClinicDesignerVersion', '1'):
        Rows = {Row:{'displayName':Caption,'valueType':'Choice','categories':list(Categories)}
            for _, _, Row, Caption, Categories, _ in DesignerQuestions + [DesignerECG]}
        DesignerTool('data_table.DataTableTools.add_rows', data_table=Table, row_names=list(Rows))
        DesignerTool('data_table.DataTableTools.set_rows', data_table=Table, values=json.dumps(Rows, ensure_ascii=False))
        SetMetadata(Table['refPath'], {'ClinicDesignerVersion':'1'})
        Save(Table)
    SharedTable = Load(Folder + '/Data/Indicators/DT_ClinicIndicators')
    SourceScenario = Load(Folder + '/Blueprints/BP_ClinicScenario')
    Actions, Order, ActiveFacts = DesignerCatalog(Table, SourceScenario)
    ActiveActions = set(Order)
    Diseases = DesignerDiseases(Table)
    BaselineClass = Load(Folder + '/Blueprints/BP_ClinicCaseBaseline')['refPath'] + '_C'
    Baseline, _ = DataAsset('DA_ClinicCaseBaseline_Designer', BaselineClass, 'Cases/Designer')
    if not HasMetadata(Baseline['refPath'], 'ClinicDesignerVersion', '1'):
        Source = Load(Folder + '/Data/Cases/DA_ClinicCaseBaseline')
        Data = DesignerFilterData(GetProperties(Source, ['Data'])['Data'], ActiveFacts, ActiveActions)
        Extra = DesignerExtraData(Table, Actions, None)
        DesignerSave(Baseline, {'Data':{Field:Values + Extra[Field] for Field, Values in Data.items()}})
    Scenario = DesignerDuplicate(SourceScenario['refPath'], Folder + '/Blueprints/BP_ClinicScenario_Designer')
    Stage = '1' if FirstVisitOnly else '5'
    CatalogChanged = not HasMetadata(Scenario['refPath'], 'ClinicDesignerVisits', '5') and not HasMetadata(
        Scenario['refPath'], 'ClinicDesignerVisits', Stage)
    if CatalogChanged:
        Specifications = DesignerVisits[:1] if FirstVisitOnly else DesignerVisits
        Visits = [DesignerVisit(Spec, Table, SharedTable, Actions, ActiveFacts, ActiveActions, Baseline, Diseases)
            for Spec in Specifications]
        SetProperties(Scenario, {'Visits':[],'PatientCases':[],'ActionDefinitions':[],'DiseaseDefinitions':[]})
        SetProperties(Scenario, {'Visits':Visits,'ActionDefinitions':Order,'InitialCaseIndex':0,
            'DiseaseDefinitions':[Disease['refPath'] for Disease in Diseases.values()]})
        SetMetadata(Scenario['refPath'], {'ClinicDesignerVisits':Stage})
        Save(Scenario)
    Level = DesignerClinicMap(Scenario, CatalogChanged)
    if not FirstVisitOnly:
        RegisterClinicMap(Level['refPath'], 'Кабинет терапевта — приёмы дизайнера')
    return {'level':Level,'scenario':Scenario,'visits':int(Metadata(Scenario['refPath'])['ClinicDesignerVisits'])}


def ReferenceSymptoms(Table, Disease):
    def Choice(Caption, Indicator, Values, Areas=None):
        return {'Description':Caption,
            'Indicator':{'DataTable':Table['refPath'], 'RowName':Indicator},
            'Areas':Areas or [], 'CategoryValues':Values}

    def Number(Caption, Indicator, Minimum=None, Maximum=None, Areas=None, IncludeMinimum=True, IncludeMaximum=True):
        Value = Choice(Caption, Indicator, [], Areas)
        Value.update({'bHasMinimum':Minimum is not None, 'bHasMaximum':Maximum is not None,
            'bIncludeMinimum':IncludeMinimum, 'bIncludeMaximum':IncludeMaximum})
        if Minimum is not None:
            Value['Minimum'] = Minimum
        if Maximum is not None:
            Value['Maximum'] = Maximum
        return Value

    Respiratory = Choice('Насморк, боль в горле или кашель', 'RespiratorySymptoms', ['Present'])
    Fatigue = Choice('Слабость и повышенная утомляемость', 'Fatigue', ['Present'])
    Injury = Choice('Сведения о травме левой руки', 'InjuryMechanism', ['Present'])
    LocalInjury = Choice('Признаки повреждения при осмотре предплечья', 'InjuryInspection', ['InjurySigns'], ['LeftForearm'])
    Definitions = {
        'ViralInfection': [Respiratory, Fatigue,
            Number('Температура от 37 до 38 °C, не включая 38', 'Temperature', 37, 38, ['Mouth'], IncludeMaximum=False)],
        'Pneumonia': [Respiratory,
            Choice('Лихорадка со слов пациента', 'FeverHistory', ['Present']),
            Number('Измеренная температура 38 °C и выше', 'Temperature', Minimum=38, Areas=['Mouth']),
            Choice('Локальные дополнительные дыхательные звуки', 'LungSounds', ['Crackles'], ['LeftChest','RightChest','LeftBack','RightBack']),
            Choice('Инфильтрация на рентгенограмме грудной клетки', 'ChestRadiograph', ['Infiltrate'])],
        'IronDeficiency': [Fatigue,
            Choice('Одышка при физической нагрузке', 'Breathlessness', ['Exertional']),
            Choice('Сведения о кровопотерях', 'BloodLoss', ['Present']),
            Number('Гемоглобин менее 120 г/л', 'Hemoglobin', Maximum=120, IncludeMaximum=False),
            Number('Ферритин менее 15 мкг/л', 'Ferritin', Maximum=15, IncludeMaximum=False)],
        'Diabetes': [Fatigue,
            Choice('Жажда и учащённое мочеиспускание', 'ThirstUrination', ['Present']),
            Number('Снижение веса за время болезни', 'WeightLoss', Minimum=0, IncludeMinimum=False),
            Number('Глюкоза венозной плазмы натощак ≥ 7,0 ммоль/л', 'Glucose', Minimum=7),
            Number('HbA1c ≥ 6,5 %', 'GlycatedHemoglobin', Minimum=6.5)],
        'Contusion': [Injury, LocalInjury,
            Choice('На снимке костных повреждений не выявлено', 'BoneRadiograph', ['Normal'])],
        'RadiusFracture': [Injury, LocalInjury,
            Choice('Перелом на рентгенограмме предплечья и запястья', 'BoneRadiograph', ['Fracture'])],
    }
    return copy.deepcopy(Definitions[Disease])


def ConfigureReferenceBookData():
    Table = Load(Folder + '/Data/Indicators/DT_ClinicIndicators')
    Added = []
    for Identifier, Caption, Reference, SourceKey in DiseaseSpecs:
        Disease = Load(Folder + '/Data/Diseases/DA_Disease_' + Identifier)
        if not GetProperties(Disease, ['Symptoms'])['Symptoms']:
            SetProperties(Disease, {'Symptoms':ReferenceSymptoms(Table, Identifier)})
            Save(Disease)
            Added.append(Identifier)
    return Added


def ConfigureReferenceBookGeometry():
    Book, _ = Blueprint('BP_ClinicReferenceBook', '/Script/YakutSimulator.ClinicReferenceBook')
    if HasMetadata(Book['refPath'], 'ClinicBookGeometryConfigured', '1'):
        return Book
    Names = ['Cover','LeftPage','RightPage','PagePivot','TurningPageFront','TurningPageBack','LeftPageEdge','RightPageEdge']
    Parts = GetProperties(Book, Names)
    SetProperties(Parts['Cover'], {'StaticMesh':'/Engine/BasicShapes/Cube.Cube',
        'RelativeScale3D':{'x':.22,'y':.32,'z':.006}})
    SetProperties(Parts['PagePivot'], {'RelativeLocation':{'x':0,'y':0,'z':.6}})
    for Name in ['LeftPage','RightPage','TurningPageFront','TurningPageBack']:
        Side = 7.5
        Height = .6
        Rotation = {'pitch':90,'yaw':180,'roll':0}
        if Name == 'LeftPage':
            Side = -7.5
        if Name.startswith('Turning'):
            Height = .03
        if Name == 'TurningPageBack':
            Height = -.03
            Rotation = {'pitch':-90,'yaw':0,'roll':0}
        SetProperties(Parts[Name], {'RelativeLocation':{'x':0,'y':Side,'z':Height},
            'RelativeRotation':Rotation, 'RelativeScale3D':{'x':1,'y':15/900,'z':20/1100},
            'DrawSize':{'x':900,'y':1100}})
    for Name, Side in [('LeftPageEdge',-15),('RightPageEdge',15)]:
        SetProperties(Parts[Name], {'RelativeLocation':{'x':0,'y':Side,'z':.6},
            'BoxExtent':{'x':10,'y':1.5,'z':9}})
    Compile(Book)
    SetMetadata(Book['refPath'], {'ClinicBookGeometryConfigured':'1'})
    Save(Book)
    return Book


def ReferenceWidget(Name):
    Path = Folder + '/UI/' + Name
    if Exists(Path):
        return Load(Path)
    Asset = execute_tool('UMGToolSet.UMGToolSet.CreateWidgetBlueprint', json.dumps({
        'folderPath':Folder+'/UI', 'assetName':Name, 'parentClass':{'refPath':'/Script/UMG.UserWidget'}}))['returnValue']
    CreatedAssets.append(Asset['refPath'])
    return Asset


def ReferenceVariables(Asset, Page):
    Existing = execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.list_variables', json.dumps({'blueprint':Asset}))['returnValue']
    if 'Book' not in Existing:
        execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_object_variable', json.dumps({
            'blueprint':Asset, 'name':'Book', 'object_class':{'refPath':'/Script/YakutSimulator.ClinicReferenceBook'}}))
    Names = [('SurfaceIndex','int')]
    if not Page:
        Names = [('DiseaseIdentifier','name'),('VisitIdentifier','int')]
        if 'Symptom' not in Existing:
            execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_struct_variable', json.dumps({
                'blueprint':Asset, 'name':'Symptom', 'struct_type':{'refPath':'/Script/YakutSimulator.ClinicReferenceSymptom'}}))
    for Name, Type in Names:
        if Name not in Existing:
            execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_variable', json.dumps({
                'blueprint':Asset, 'name':Name, 'type_name':Type}))


def ConfigureReferenceBookLayout():
    Row = ReferenceWidget('WBP_ClinicReferenceSymptom')
    if not HasMetadata(Row['refPath'], 'ClinicBookLayoutConfigured', '1'):
        Cache = GetWidgets(Row)
        Border = Widget(Row, Cache, 'Highlight', '/Script/UMG.Border')
        ExposeWidget(Row, Border)
        SetProperties(Border['widget'], {'brushColor':{'r':.30,'g':.30,'b':.26,'a':1},
            'padding':{'left':5,'top':5,'right':5,'bottom':5}})
        ButtonInfo = Widget(Row, Cache, 'SymptomButton', '/Game/Base/UI/WBP_MainButton.WBP_MainButton_C', Border['widget'])
        ExposeWidget(Row, ButtonInfo)
        Column = Widget(Row, Cache, 'ButtonContent', '/Script/UMG.VerticalBox', ButtonInfo['widget'], True)
        Text(Row, Cache, 'Caption', Column['widget'], 'Симптом', 'Button')
        Text(Row, Cache, 'Feedback', Column['widget'], 'Нажмите, чтобы проверить', 'Body')
        execute_tool('UMGToolSet.UMGToolSet.BindToEventProperty', json.dumps({'widgetBlueprint':Row,
            'eventName':'OnClicked','propertyName':'SymptomButton',
            'propertyClass':{'refPath':'/Game/Base/UI/WBP_MainButton.WBP_MainButton_C'}}))
        ReferenceVariables(Row, False)
        Compile(Row)
        SetMetadata(Row['refPath'], {'ClinicBookLayoutConfigured':'1'})
        Save(Row)
    Page = ReferenceWidget('WBP_ClinicReferencePage')
    if not HasMetadata(Page['refPath'], 'ClinicBookLayoutConfigured', '1'):
        Cache = GetWidgets(Page)
        Paper = Widget(Page, Cache, 'Paper', '/Script/UMG.Border')
        SetProperties(Paper['widget'], {'brushColor':{'r':.88,'g':.83,'b':.69,'a':1},
            'padding':{'left':32,'top':28,'right':32,'bottom':24}})
        Column = Widget(Page, Cache, 'PageContent', '/Script/UMG.VerticalBox', Paper['widget'])
        Text(Page, Cache, 'Title', Column['widget'], 'Справочник болезней', 'Title')
        Text(Page, Cache, 'Reference', Column['widget'], 'Кабинет терапевта', 'Body')
        Symptoms = Widget(Page, Cache, 'Symptoms', '/Script/UMG.VerticalBox', Column['widget'])
        ExposeWidget(Page, Symptoms)
        Text(Page, Cache, 'Help', Column['widget'], 'Листайте рукой: Grip у наружного края страницы.', 'Body')
        ReferenceVariables(Page, True)
        Compile(Page)
        SetMetadata(Page['refPath'], {'ClinicBookLayoutConfigured':'1'})
        Save(Page)
    return {'page':Page,'row':Row}


def BuildReferenceSymptomGraph():
    Asset = Load(Folder+'/UI/WBP_ClinicReferenceSymptom')
    if HasMetadata(Asset['refPath'], 'ClinicBookGraphConfigured', '1'):
        return Asset
    Update = execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_function_graph',
        json.dumps({'blueprint':Asset,'graph_name':'UpdateEvidence'}))['returnValue']
    ResetGraph(Update)
    for Type in ['Variables|Default|GetBook','Variables|Default|GetSymptom','Variables|Default|GetDiseaseIdentifier',
        'Variables|Default|SetVisitIdentifier','Variables|WBP_ClinicReferenceSymptom|GetCaption',
        'Variables|WBP_ClinicReferenceSymptom|GetFeedback','Variables|WBP_ClinicReferenceSymptom|GetHighlight',
        'Clinic|Book|GetVisitIdentifier','Clinic|Book|GetCheckedSymptomStatus','Utilities|Struct|BreakClinicReferenceSymptom',
        'Utilities|FlowControl|Switch|SwitchonEClinicReferenceSymptomStatus','Appearance|SetBrushColor','Widget|SetText(Text)']:
        Discover(Update, Type)
    WriteGraph(Update, '''(fn UpdateEvidence ()
        (bind Owner (Variables|Default|GetBook))
        (Variables|Default|SetVisitIdentifier (Clinic|Book|GetVisitIdentifier :self Owner))
        (bind (Identifier Description Indicator Areas Categories HasMinimum Minimum IncludeMinimum HasMaximum Maximum IncludeMaximum)
            (Utilities|Struct|BreakClinicReferenceSymptom (Variables|Default|GetSymptom)))
        (Widget|SetText(Text) :self (Variables|WBP_ClinicReferenceSymptom|GetCaption) :InText Description)
        (Widget|SetText(Text) :self (Variables|WBP_ClinicReferenceSymptom|GetFeedback) :InText "Нажмите, чтобы проверить")
        (Appearance|SetBrushColor :self (Variables|WBP_ClinicReferenceSymptom|GetHighlight) :InBrushColor "(R=0.30,G=0.30,B=0.26,A=1.0)")
        (bind (Status Checked) (Clinic|Book|GetCheckedSymptomStatus :self Owner
            :DiseaseIdentifier (Variables|Default|GetDiseaseIdentifier) :SymptomIdentifier Identifier))
        (if Checked
            (switch Utilities|FlowControl|Switch|SwitchonEClinicReferenceSymptomStatus Status
                (:Unknown (Widget|SetText(Text) :self (Variables|WBP_ClinicReferenceSymptom|GetFeedback) :InText "Недостаточно данных"))
                (:Confirmed
                    (Widget|SetText(Text) :self (Variables|WBP_ClinicReferenceSymptom|GetFeedback) :InText "✓ Подтверждено")
                    (Appearance|SetBrushColor :self (Variables|WBP_ClinicReferenceSymptom|GetHighlight) :InBrushColor "(R=0.02,G=0.55,B=0.12,A=1.0)"))
                (:NotConfirmed (Widget|SetText(Text) :self (Variables|WBP_ClinicReferenceSymptom|GetFeedback) :InText "Не подтверждается")))))''')
    Events = Graph(Asset,'EventGraph')
    ResetGraph(Events)
    for Type in ['Variables|Default|GetBook','Variables|Default|GetSymptom','Variables|Default|GetDiseaseIdentifier',
        'Variables|Default|GetVisitIdentifier','Clinic|Book|CheckSymptom','Utilities|Struct|BreakClinicReferenceSymptom',
        'Utilities|IsValid','CallFunction|UpdateEvidence','AddEvent|UserInterface|EventConstruct',
        'Clinic|Book|BindEventtoOnEvidenceChanged','EventDispatchers|CreateEvent']:
        Discover(Events,Type)
    WriteGraph(Events, '''(event UserInterface|EventConstruct
        (bind Owner (Variables|Default|GetBook))
        (Utilities|IsValid Owner
            (:"Is Valid" (CallFunction|UpdateEvidence))
            (:"Is Not Valid")))
        (event OnClicked(SymptomButton)
            (bind Identifier (Utilities|Struct|BreakClinicReferenceSymptom (Variables|Default|GetSymptom)))
            (Clinic|Book|CheckSymptom :self (Variables|Default|GetBook)
                :DiseaseIdentifier (Variables|Default|GetDiseaseIdentifier) :SymptomIdentifier Identifier
                :ExpectedVisitIdentifier (Variables|Default|GetVisitIdentifier)))''')
    Infos = [NodeInfo(Node) for Node in Nodes(Events)]
    UpdateCall = next(Info for Info in Infos if Info['type_id'].split('|')[-1] == 'UpdateEvidence')
    Valid = next(Info for Info in Infos if Info['type_id'] == 'Utilities|IsValid')
    Target = Pin(UpdateCall,'execute','input_pins')
    Previous = Target['connected_pins'][0]
    Owner = Pin(Valid,'InputObject','input_pins')['connected_pins'][0]
    Binding = CreateNode(Events,'Clinic|Book|BindEventtoOnEvidenceChanged',[200,-250])
    Delegate = CreateNode(Events,'EventDispatchers|CreateEvent',[0,-400])
    BindingInfo = NodeInfo(Binding)
    DelegateInfo = NodeInfo(Delegate)
    Connect(Owner,Pin(BindingInfo,'self','input_pins')['pin_id'])
    Connect(Pin(DelegateInfo,'OutputDelegate','output_pins')['pin_id'],Pin(BindingInfo,'Delegate','input_pins')['pin_id'])
    execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.set_create_event_function',
        json.dumps({'node':Delegate,'function_name':'UpdateEvidence'}))
    execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.break_pins',json.dumps({'output_pin':Previous,'input_pin':Target['pin_id']}))
    Connect(Previous,Pin(BindingInfo,'execute','input_pins')['pin_id'])
    Connect(Pin(BindingInfo,'then','output_pins')['pin_id'],Target['pin_id'])
    Compile(Asset)
    SetMetadata(Asset['refPath'], {'ClinicBookGraphConfigured':'1'})
    Save(Asset)
    return Asset


def BuildReferencePageGraph():
    Asset = Load(Folder+'/UI/WBP_ClinicReferencePage')
    if HasMetadata(Asset['refPath'], 'ClinicBookGraphConfigured', '1'):
        return Asset
    Render = execute_tool('editor_toolset.toolsets.blueprint.BlueprintTools.add_function_graph',
        json.dumps({'blueprint':Asset,'graph_name':'RenderPage'}))['returnValue']
    ResetGraph(Render)
    DiseaseTitle = ClinicPropertyGetter(Render, 'ClinicDiseaseDefinition', 'DisplayName', 'Название')
    DiseaseArticle = ClinicPropertyGetter(Render, 'ClinicDiseaseDefinition', 'ReferenceText', 'Текстсправочника')
    DiseaseSymptoms = ClinicPropertyGetter(Render, 'ClinicDiseaseDefinition', 'Symptoms', 'Симптомы')
    for Type in ['Variables|Default|GetBook','Variables|Default|GetSurfaceIndex',
        'Variables|WBP_ClinicReferencePage|GetTitle','Variables|WBP_ClinicReferencePage|GetReference',
        'Variables|WBP_ClinicReferencePage|GetSymptoms','Variables|WBP_ClinicReferencePage|GetHelp',
        'Clinic|Book|GetPageDisease','Clinic|Book|IsSymptomsPage',DiseaseSymptoms,
        DiseaseTitle,DiseaseArticle,
        'Class|ClinicDiseaseDefinition|GetDiseaseIdentifier','Widget|Panel|ClearChildren','Panel|AddChildtoVerticalBox',
        'Class|Text|SetColorandOpacity','Widget|SetVisibility','Widget|GetOwningPlayer','UserInterface|CreateWidget',
        'Class|WBPClinicReferenceSymptom|SetSymptom','Class|WBPClinicReferenceSymptom|SetBook',
        'Class|WBPClinicReferenceSymptom|SetDiseaseIdentifier','Utilities|IsValid']:
        Discover(Render,Type)
    WriteGraph(Render, '''(fn RenderPage ()
        (bind Owner (Variables|Default|GetBook))
        (bind Surface (Variables|Default|GetSurfaceIndex))
        (bind Container (Variables|WBP_ClinicReferencePage|GetSymptoms))
        (bind Title (Variables|WBP_ClinicReferencePage|GetTitle))
        (bind Reference (Variables|WBP_ClinicReferencePage|GetReference))
        (bind Help (Variables|WBP_ClinicReferencePage|GetHelp))
        (Widget|Panel|ClearChildren :self Container)
        (Class|Text|SetColorandOpacity :self Title :ColorAndOpacity "(SpecifiedColor=(R=0.04,G=0.035,B=0.025,A=1.0),ColorUseRule=UseColor_Specified)")
        (Class|Text|SetColorandOpacity :self Reference :ColorAndOpacity "(SpecifiedColor=(R=0.04,G=0.035,B=0.025,A=1.0),ColorUseRule=UseColor_Specified)")
        (Class|Text|SetColorandOpacity :self Help :ColorAndOpacity "(SpecifiedColor=(R=0.12,G=0.10,B=0.07,A=1.0),ColorUseRule=UseColor_Specified)")
        (Utilities|IsValid Owner
            (:"Is Valid"
                (bind Disease (Clinic|Book|GetPageDisease :self Owner :SurfaceIndex Surface))
                (Utilities|IsValid Disease
                    (:"Is Valid"
                        (Widget|SetText(Text) :self Title :InText (DISEASE_TITLE :self Disease))
                        (if (Clinic|Book|IsSymptomsPage :self Owner :SurfaceIndex Surface)
                            (Widget|SetVisibility :self Reference :InVisibility "Collapsed")
                            (Widget|SetText(Text) :self Help :InText "Нажмите на признак, чтобы сопоставить его с собранными данными. Совпадения сами по себе не устанавливают диагноз.")
                            (for Symptom (DISEASE_SYMPTOMS :self Disease)
                                (bind Row (UserInterface|CreateWidget :Class "/Game/Minigames/Clinic/UI/WBP_ClinicReferenceSymptom.WBP_ClinicReferenceSymptom_C"
                                    :OwningPlayer (Widget|GetOwningPlayer :self self)))
                                (Class|WBPClinicReferenceSymptom|SetBook :self Row :Book Owner)
                                (Class|WBPClinicReferenceSymptom|SetSymptom :self Row :Symptom Symptom)
                                (Class|WBPClinicReferenceSymptom|SetDiseaseIdentifier :self Row :DiseaseIdentifier (Class|ClinicDiseaseDefinition|GetDiseaseIdentifier :self Disease))
                                (Panel|AddChildtoVerticalBox :self Container :Content Row))
                            (else
                                (Widget|SetVisibility :self Reference :InVisibility "Visible")
                                (Widget|SetText(Text) :self Reference :InText (DISEASE_ARTICLE :self Disease))
                                (Widget|SetText(Text) :self Help :InText "Листайте по одному листу: Grip у наружного края, затем движение руки через корешок."))))
                    (:"Is Not Valid")))
            (:"Is Not Valid")))'''.replace('DISEASE_TITLE', DiseaseTitle)
            .replace('DISEASE_SYMPTOMS', DiseaseSymptoms).replace('DISEASE_ARTICLE', DiseaseArticle))
    Compile(Asset)
    SetMetadata(Asset['refPath'], {'ClinicBookGraphConfigured':'1'})
    Save(Asset)
    return Asset


def BuildReferenceBookGraph():
    Asset = Load(Folder+'/Blueprints/BP_ClinicReferenceBook')
    if HasMetadata(Asset['refPath'], 'ClinicBookGraphConfigured', '1'):
        return Asset
    Events = Graph(Asset,'EventGraph')
    ResetGraph(Events)
    Parts = ['LeftPage','RightPage','TurningPageFront','TurningPageBack']
    for Type in ['AddEvent|Clinic|Book|EventInitializePages','AddEvent|Clinic|Book|EventRefreshPages',
        'Class|WBPClinicReferencePage|SetBook','Class|WBPClinicReferencePage|SetSurfaceIndex',
        'Class|WBPClinicReferencePage|RenderPage','Utilities|Casting|CastToWBP_ClinicReferencePage',
        'UserInterface|CreateWidget','UserInterface|SetWidget','UserInterface|GetUserWidgetObject']:
        Discover(Events,Type)
    for Part in Parts:
        Discover(Events,'Variables|Clinic|Book|Get'+Part)
    Initialize = ['(event Clinic|Book|EventInitializePages (Player)']
    for Index, Part in enumerate(Parts):
        Initialize.append('''(bind Screen{0} (UserInterface|CreateWidget :Class "/Game/Minigames/Clinic/UI/WBP_ClinicReferencePage.WBP_ClinicReferencePage_C" :OwningPlayer Player))
            (Class|WBPClinicReferencePage|SetBook :self Screen{0} :Book self)
            (Class|WBPClinicReferencePage|SetSurfaceIndex :self Screen{0} :SurfaceIndex {0})
            (UserInterface|SetWidget :self (Variables|Clinic|Book|Get{1}) :Widget Screen{0})'''.format(Index,Part))
    Refresh = ''
    for Index in reversed(range(len(Parts))):
        Refresh = '''(bind Page{0} (UserInterface|GetUserWidgetObject :self (Variables|Clinic|Book|Get{1})))
            (bind Screen{0} (Utilities|Casting|CastToWBP_ClinicReferencePage :Object Page{0})
                (:then (Class|WBPClinicReferencePage|RenderPage :self Screen{0}) {2})
                (:CastFailed))'''.format(Index,Parts[Index],Refresh)
    WriteGraph(Events,'\n'.join(Initialize+[')','(event Clinic|Book|EventRefreshPages '+Refresh+')']))
    Compile(Asset)
    SetMetadata(Asset['refPath'], {'ClinicBookGraphConfigured':'1'})
    Save(Asset)
    return Asset


def ConfigureReferenceHandInput():
    Hud = Load('/Game/Base/Blueprints/BP_MainHUD')
    if HasMetadata(Hud['refPath'], 'LocalHandGripPointerConfigured', '1'):
        return Hud
    for Hand in ['Left','Right']:
        Target = Graph(Hud,'Process'+Hand+'Pointer')
        for Type in ['HUD|GetOwningPlayerController','Pawn|GetControlledPawn','Utilities|Casting|CastToVirtualRealityPawn',
            'Class|VirtualRealityPawn|Get'+Hand+'Pointer','Interaction|IsHandBusy',
            'Variables|Default|Get'+Hand+'TriggerPressed','Variables|Default|Set'+Hand+'TriggerPressed',
            'Interaction|IsOverHitTestVisibleWidget','Interaction|PressPointerKey','Interaction|ReleasePointerKey']:
            Discover(Target,Type)
        ResetGraph(Target)
        WriteGraph(Target, '''(fn Process{Hand}Pointer (Value)
            (bind Pawn (Utilities|Casting|CastToVirtualRealityPawn :Object (Pawn|GetControlledPawn :self (HUD|GetOwningPlayerController)))
                (:then
                    (bind Pointer (Class|VirtualRealityPawn|Get{Hand}Pointer :self Pawn))
                    (bind Busy (Interaction|IsHandBusy :self Pawn :bLeftHand {Left}))
                    (if Busy (Interaction|ReleasePointerKey :self Pointer :Key "LeftMouseButton"))
                    (if (and (>= Value 0.65) (not (Variables|Default|Get{Hand}TriggerPressed)))
                        (Variables|Default|Set{Hand}TriggerPressed true)
                        (if (not Busy)
                            (if (Interaction|IsOverHitTestVisibleWidget :self Pointer)
                                (Interaction|PressPointerKey :self Pointer :Key "LeftMouseButton")))
                        (elif (and (<= Value 0.35) (Variables|Default|Get{Hand}TriggerPressed))
                            (Variables|Default|Set{Hand}TriggerPressed false)
                            (Interaction|ReleasePointerKey :self Pointer :Key "LeftMouseButton"))))
                (:CastFailed)))'''.format(Hand=Hand,Left=str(Hand=='Left').lower()))
    Compile(Hud)
    SetMetadata(Hud['refPath'], {'LocalHandGripPointerConfigured':'1'})
    Save(Hud)
    return Hud


def ConfigureReferenceBookSurfaces(Book):
    if HasMetadata(Book['refPath'], 'ClinicBookSurfacesConfigured', '1'):
        return
    Parts = GetProperties(Book,['Cover','LeftPage','RightPage','TurningPageFront','TurningPageBack'])
    Material = '/Game/Minigames/Fishing/Workstations/WinchConsole/Materials/M_WinchRubber.M_WinchRubber'
    if Exists(Material):
        SetProperties(Parts['Cover'],{'OverrideMaterials':[Material]})
    for Name in ['LeftPage','RightPage','TurningPageFront','TurningPageBack']:
        SetProperties(Parts[Name],{'WidgetClass':Folder+'/UI/WBP_ClinicReferencePage.WBP_ClinicReferencePage_C'})
        if Name.startswith('Turning'):
            SetProperties(Parts[Name],{'bVisible':False})
    Page = Load(Folder+'/UI/WBP_ClinicReferencePage')
    Cache = GetWidgets(Page)
    for Name in ['Title','Reference','Help']:
        InspectWidget(Cache[Name])
        SetProperties(Cache[Name]['widget'],{'ColorAndOpacity':{'specifiedColor':{'r':.04,'g':.035,'b':.025,'a':1},'colorUseRule':'UseColor_Specified'}})
    Compile(Page)
    Save(Page)
    Compile(Book)
    SetMetadata(Book['refPath'],{'ClinicBookSurfacesConfigured':'1'})
    Save(Book)


def ConfigureReferenceBookHolding():
    if execute_tool('EditorToolset.EditorAppToolset.IsPIERunning', '{}')['returnValue']:
        raise RuntimeError('Reference book holding authoring requires PIE to be stopped')
    Book = Load(Folder+'/Blueprints/BP_ClinicReferenceBook')
    if HasMetadata(Book['refPath'], 'ClinicBookHoldingConfigured', '1'):
        return Book
    Parts = GetProperties(Book, ['BookGripZone', 'LeftHandGrip'])
    SetProperties(Parts['BookGripZone'], {'RelativeLocation':{'x':-8,'y':-6,'z':0},
        'BoxExtent':{'x':3,'y':6,'z':4}})
    SetProperties(Parts['LeftHandGrip'], {'RelativeLocation':{'x':-8,'y':-6,'z':-1},
        'RelativeRotation':{'pitch':-35,'yaw':0,'roll':0}})
    Compile(Book)
    SetMetadata(Book['refPath'], {'ClinicBookHoldingConfigured':'1'})
    Save(Book)
    return Book


def ConfigureReferenceBook():
    if execute_tool('EditorToolset.EditorAppToolset.IsPIERunning','{}')['returnValue']:
        raise RuntimeError('Reference book authoring requires PIE to be stopped')
    Added = ConfigureReferenceBookData()
    Widgets = ConfigureReferenceBookLayout()
    BuildReferenceSymptomGraph()
    BuildReferencePageGraph()
    Book = ConfigureReferenceBookGeometry()
    BuildReferenceBookGraph()
    ConfigureReferenceBookSurfaces(Book)
    ConfigureReferenceBookHolding()
    Hud = ConfigureReferenceHandInput()
    Level = '/Game/Maps/L_Clinic'
    Current = execute_tool('editor_toolset.toolsets.scene.SceneTools.get_current_level','{}')['returnValue']
    if Current == Level and execute_tool('editor_toolset.toolsets.asset.AssetTools.is_dirty',json.dumps({'asset_path':Level}))['returnValue']:
        Save(Load(Level))
    execute_tool('editor_toolset.toolsets.scene.SceneTools.load_level',json.dumps({'level_path':Level}))
    Scenarios = FindActors(ActorClass='/Script/YakutSimulator.ClinicScenario')
    if len(Scenarios) != 1:
        raise RuntimeError('Reference book requires one explicit clinic scenario')
    Existing = FindActors(ActorClass='/Script/YakutSimulator.ClinicReferenceBook')
    if len(Existing) > 1:
        raise RuntimeError('Multiple reference books require explicit authoring')
    if Existing:
        Actor = Existing[0]
    else:
        Actor = SpawnAsset(Folder+'/Blueprints/BP_ClinicReferenceBook','ClinicReferenceBook',[20,0,80.3],[0,0,0])
        SetProperties(Actor,{'Scenario':Scenarios[0]['refPath']})
        Save(Load(Level))
    return {'book':Book,'actor':Actor,'widgets':Widgets,'hud':Hud,'diseasesAdded':Added,'gameplayTestsRun':False}
