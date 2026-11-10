import math
import pathlib
import random
from PIL import Image, ImageDraw, ImageFont, ImageFilter

ProjectDirectory = pathlib.Path('M:/Prototypes/Yakut-Simulator').resolve()
Directory = ProjectDirectory / 'SourceAssets/Fishing/WinchConsole'
Directory.mkdir(parents=True, exist_ok=True)
Generator = random.Random(42)
Width = 2560
Height = 1600
FontPath = 'C:/Windows/Fonts/arialbd.ttf'


def Font(Size):
    return ImageFont.truetype(FontPath, Size)


def PanelPoint(Horizontal, Depth):
    return ((Horizontal / 1.28 + .5) * Width, (Depth / .8 + .5) * Height)


Panel = Image.new('RGB', (Width, Height), (31, 82, 112))
Noise = Image.effect_noise((Width, Height), 12).convert('RGB')
Panel = Image.blend(Panel, Noise, .075)
Draw = ImageDraw.Draw(Panel)
for Count in range(10000):
    Horizontal = Generator.randrange(Width)
    Vertical = Generator.randrange(Height)
    if Generator.random() < .83:
        Horizontal = Generator.choice([Generator.randrange(45), Width - Generator.randrange(45)])
    Length = Generator.randrange(1, 18)
    Color = Generator.choice([(93, 104, 107), (25, 49, 59), (50, 62, 65), (132, 120, 96)])
    Draw.line((Horizontal, Vertical, Horizontal + Length, Vertical + Generator.randrange(-2, 3)), fill=Color, width=Generator.randrange(1, 4))


def PanelText(Text, Horizontal, Depth, Size=38, Color=(216, 202, 117)):
    Draw.text(PanelPoint(Horizontal, Depth), Text, font=Font(Size), fill=Color, anchor='mm', align='center')


PanelText('УПРАВЛЕНИЕ ТРАЛОВЫМИ ЛЕБЁДКАМИ', 0, -.365, 43)
for Horizontal, Title in [(-.27, 'ЛЕВЫЙ ВАЕР'), (.27, 'ПРАВЫЙ ВАЕР')]:
    PanelText(Title, Horizontal, -.135, 36)
    PanelText('ВЫПУСК', Horizontal, -.095, 23)
    PanelText('ВЫБОРКА', Horizontal, .145, 23)
for Horizontal in [-.515, .515]:
    PanelText('НАТЯЖЕНИЕ', Horizontal, .085, 27)
    PanelText('−          +', Horizontal, .24, 32)
    Center = PanelPoint(Horizontal, .165)
    Radius = 90
    for Index in range(9):
        Angle = math.radians(130 + Index * 35)
        Draw.line((Center[0] + math.cos(Angle) * Radius, Center[1] + math.sin(Angle) * Radius, Center[0] + math.cos(Angle) * (Radius + 13), Center[1] + math.sin(Angle) * (Radius + 13)), fill=(216, 202, 117), width=4)
PanelText('АВАРИЙНЫЙ СТОП', 0, -.16, 25)
PanelText('ПОДТВЕРЖДЕНИЕ', 0, .075, 23)
PanelText('ПУЛЬТ ЛЕБЁДОК', -.46, .345, 22, (184, 191, 187))
Panel.save(Directory / 'T_WinchPanel.png')

Dial = Image.new('RGB', (1024, 1024), (207, 198, 156))
Dial = Image.blend(Dial, Image.effect_noise((1024, 1024), 10).convert('RGB'), .04)
Draw = ImageDraw.Draw(Dial)
Draw.ellipse((15, 15, 1009, 1009), fill=(43, 46, 42))
Draw.ellipse((34, 34, 990, 990), fill=(217, 208, 172))
for Index in range(51):
    Angle = math.radians(140 + Index * 5.2)
    Start = 372
    if Index % 5 == 0:
        Start = 340
    Draw.line((512 + math.cos(Angle) * Start, 512 + math.sin(Angle) * Start, 512 + math.cos(Angle) * 405, 512 + math.sin(Angle) * 405), fill=(29, 35, 33), width=6)
    if Index % 5 == 0:
        Draw.text((512 + math.cos(Angle) * 287, 512 + math.sin(Angle) * 287), str(Index * 2), fill=(31, 36, 32), font=Font(52), anchor='mm')
Draw.arc((89, 89, 935, 935), start=361, end=400, fill=(145, 38, 25), width=24)
Draw.text((512, 420), 'НАТЯЖЕНИЕ', font=Font(42), fill=(37, 43, 38), anchor='mm')
Draw.text((512, 500), 'кН', font=Font(65), fill=(37, 43, 38), anchor='mm')
Draw.text((512, 680), '0—100', font=Font(34), fill=(67, 69, 57), anchor='mm')
Dial.save(Directory / 'T_WinchDial.png')
Readout = Image.new('RGB', (1400, 240), (5, 12, 11))
Draw = ImageDraw.Draw(Readout)
for Horizontal, Title in [(24, 'ЛЕВЫЙ'), (715, 'ПРАВЫЙ')]:
    Draw.text((Horizontal, 12), Title, font=Font(24), fill=(179, 197, 182))
    Draw.text((Horizontal, 48), '0.0 м      0.0 кН      0.0 м/с', font=Font(34), fill=(122, 204, 163))
Draw.line((24, 112, 1376, 112), fill=(51, 76, 62), width=2)
Draw.text((24, 134), 'Проверьте нейтраль обоих рычагов и подтвердите готовность.', font=Font(25), fill=(196, 207, 184))
Draw.text((24, 183), 'Ожидание команды', font=Font(22), fill=(145, 165, 148))
Readout.save(Directory / 'T_WinchReadout.png')
print('Winch panel and instrument textures saved')
