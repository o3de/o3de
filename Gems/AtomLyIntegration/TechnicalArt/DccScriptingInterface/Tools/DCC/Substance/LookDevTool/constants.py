import pysbs

# Optional/Debugging autofill locations --------------->>
DCC_DEFAULT_FILE = 'E:/TEST_ASSETS/maya_arnold_materials/arnold_material_testing.mb'
DCC_DEFAULT_DIRECTORY = 'E:/TEST_ASSETS/maya_arnold_materials'
SBS_DEFAULT_FILE = 'E:/PROJECTS/MATERIALS/Alex_Materials/Leather_DimpledCracked_01/Leather_DimpledCracked_01.sbs'
SBS_DEFAULT_DIRECTORY = 'E:/PROJECTS/MATERIALS/Alex_Materials'
O3DE_LOCATION = 'E:/Depot/o3de-engine'
OUTPUT_LOCATION = 'C:/Users/benblac/Desktop/TESTING/LABC'

default_settings = {
    'dcc_input_file': {'path': DCC_DEFAULT_FILE, 'id': 0},
    'dcc_input_directory': {'path': DCC_DEFAULT_DIRECTORY, 'id': 1},
    'existing_input_file': {'path': SBS_DEFAULT_FILE, 'id': 2},
    'existing_input_directory': {'path': SBS_DEFAULT_DIRECTORY, 'id': 3},
    'o3de_directory_field': {'path': O3DE_LOCATION, 'id': 4},
    'output_directory': {'path': OUTPUT_LOCATION, 'id': 5}
}

# App/library locations ---------------------->>
SBS_COOKER_LOCATION = '"C:/Program Files/Adobe/Adobe Substance 3D Designer/sbscooker.exe"'

# Substance Channel Mapping ---------------------->>
OUTPUT_MAPPING = {
    'Metallic': {
        pysbs.sbsenum.UsageEnum.METALLIC: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'BaseColor': {
        pysbs.sbsenum.UsageEnum.BASECOLOR: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'Normal': {
        pysbs.sbsenum.UsageEnum.NORMAL: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'Roughness': {
        pysbs.sbsenum.UsageEnum.ROUGHNESS: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'Emissive': {
        pysbs.sbsenum.UsageEnum.EMISSIVE: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'AmbientOcclusion': {
        pysbs.sbsenum.UsageEnum.AMBIENT_OCCLUSION: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'Height': {
        pysbs.sbsenum.UsageEnum.HEIGHT: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    },
    'Opacity': {
        pysbs.sbsenum.UsageEnum.OPACITY: {
            pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
        }
    }
    # 'SpecularF0': {
    #     pysbs.sbsenum.UsageEnum.OPACITY: {
    #         pysbs.sbsenum.UsageDataEnum.COMPONENTS: pysbs.sbsenum.ComponentsEnum.RGBA
    #     }
    # }
}

PBR_TEXTURE_KEYS = {
    'Metallic': ['metalness', 'metallic', 'metal', 'm'],
    'BaseColor': ['color', 'albedo', 'diffuse', 'base_color', 'basecolor'],
    'Normal': ['normal', 'ddna', 'nrm', 'normalmap'],
    'Roughness': ['roughness', 'rough'],
    'Emissive': ['emissive', 'emission', 'emit'],
    'AmbientOcclusion': ['ao', 'occlusion', 'ambientocclusion', 'diffuseintensity'],
    'Height': ['height', 'parallax'],
    'Opacity': ['transparency', 'opacity', 'alpha', 'mask', 'transparentl']
}
