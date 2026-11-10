import unreal


def CreateWorkstationMaterials(Rebuild=False):
    Path = '/Game/Minigames/Fishing/Workstations/Materials/M_ConveyorBelt'
    Material = unreal.load_asset(Path)
    if Material and not Rebuild:
        return Material
    if Material:
        from BuildStationaryWorkstations import Backup
        Backup(Material)
    if not Material:
        Material = unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_ConveyorBelt', '/Game/Minigames/Fishing/Workstations/Materials', unreal.Material, unreal.MaterialFactoryNew())
    Library = unreal.MaterialEditingLibrary
    Library.delete_all_material_expressions(Material)
    Coordinate = Library.create_material_expression(Material, unreal.MaterialExpressionTextureCoordinate)
    Speed = Library.create_material_expression(Material, unreal.MaterialExpressionScalarParameter)
    Speed.set_editor_property('parameter_name', 'BeltSpeed')
    Zero = Library.create_material_expression(Material, unreal.MaterialExpressionConstant)
    VectorSpeed = Library.create_material_expression(Material, unreal.MaterialExpressionAppendVector)
    Library.connect_material_expressions(Zero, '', VectorSpeed, 'A')
    Library.connect_material_expressions(Speed, '', VectorSpeed, 'B')
    Panner = Library.create_material_expression(Material, unreal.MaterialExpressionPanner)
    Library.connect_material_expressions(Coordinate, '', Panner, 'Coordinate')
    Library.connect_material_expressions(VectorSpeed, '', Panner, 'Speed')
    Channel = Library.create_material_expression(Material, unreal.MaterialExpressionComponentMask)
    Channel.set_editor_property('r', False)
    Channel.set_editor_property('g', True)
    Library.connect_material_expressions(Panner, '', Channel, '')
    Frequency = Library.create_material_expression(Material, unreal.MaterialExpressionMultiply)
    Frequency.set_editor_property('const_b', 18)
    Library.connect_material_expressions(Channel, '', Frequency, 'A')
    Sine = Library.create_material_expression(Material, unreal.MaterialExpressionSine)
    Library.connect_material_expressions(Frequency, '', Sine, '')
    Absolute = Library.create_material_expression(Material, unreal.MaterialExpressionAbs)
    Library.connect_material_expressions(Sine, '', Absolute, '')
    Seam = Library.create_material_expression(Material, unreal.MaterialExpressionPower)
    Seam.set_editor_property('const_exponent', 20)
    Library.connect_material_expressions(Absolute, '', Seam, 'Base')
    Base = Library.create_material_expression(Material, unreal.MaterialExpressionConstant3Vector)
    Base.set_editor_property('constant', unreal.LinearColor(.018, .022, .024, 1))
    Edge = Library.create_material_expression(Material, unreal.MaterialExpressionConstant3Vector)
    Edge.set_editor_property('constant', unreal.LinearColor(.065, .073, .075, 1))
    Color = Library.create_material_expression(Material, unreal.MaterialExpressionLinearInterpolate)
    Library.connect_material_expressions(Base, '', Color, 'A')
    Library.connect_material_expressions(Edge, '', Color, 'B')
    Library.connect_material_expressions(Seam, '', Color, 'Alpha')
    Library.connect_material_property(Color, '', unreal.MaterialProperty.MP_BASE_COLOR)
    Roughness = Library.create_material_expression(Material, unreal.MaterialExpressionConstant)
    Roughness.set_editor_property('r', .85)
    Library.connect_material_property(Roughness, '', unreal.MaterialProperty.MP_ROUGHNESS)
    Library.recompile_material(Material)
    unreal.EditorAssetLibrary.save_loaded_asset(Material, False)
    return Material


if __name__ == '__main__':
    CreateWorkstationMaterials()
