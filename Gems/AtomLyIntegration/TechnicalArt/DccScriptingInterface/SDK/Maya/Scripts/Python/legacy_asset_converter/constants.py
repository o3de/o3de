# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# File Description:
# This is an area to make customized changes in how the script looks for and processes files
# -------------------------------------------------------------------------


IMAGE_TYPES = ['.tif', '.tiff', '.png', '.jpg', '.jpeg', '.tga']

DIRECTORY_EXCLUSION_LIST = ['.mayaSwatches']

LUMBERYARD_DATA_FILES = ['.mtl', '.assetinfo', '.material']

EXPORT_MATERIAL_TYPES = ['Illum', 'Vegetation', 'Glass']

PREFERRED_IMAGE_FORMAT = '.tif'

TRANSFER_EXTENSIONS = ['.fbx', '.assetinfo']

FILE_MASKS = {
    'ddn': ['ddn'],
    'ddna': ['ddna'],
    'diffuse': ['diffuse', 'diff', 'dif', 'd'],
    'emissive': ['emis', 'emissive', 'e', 'emiss'],
    'specular': ['spec', 'specular'],
    'scattering': ['scattering', 'sss'],
    'normal': ['normal'],
    'basecolor': ['basecolor', 'albedo']
}

LOAD_WEIGHTS = {
    'maya': 4,
    'fbx': 2,
    'material': 1
}

SUPPORTED_MATERIAL_PROPERTIES = ['general', 'ambientOcclusion', 'baseColor', 'emissive',
                        'metallic', 'roughness', 'specularF0', 'normal', 'opacity', 'uv']

MATERIAL_SUFFIX_LIST = ['_mat', '_m']

DIELECTRIC_METALLIC_COLOR = (.04, .04, .04)

# For more information see OpenImageIO docs
# https://openimageio.readthedocs.io/en/latest/pythonbindings.html
# Recommended weight values for:
# ImageBufAlgo.channel_sum
WEIGHTED_RGB_VALUES = (.2126, .7152, .0722)

# MATERIAL DB
FBX_DIRECTORY_PATH = 'directorypath'
FBX_DIRECTORY_NAME = 'directoryname'
FBX_FILES = 'fbxfiles'
FBX_MATERIALS = 'materials'
FBX_TEXTURES = 'textures'
FBX_MAYA_FILE = 'mayafile'
FBX_DESIGNER_FILE = 'designerfile'
FBX_TEXTURE_MODIFICATIONS = 'modifications'
FBX_NUMERICAL_SETTINGS = 'numericalsettings'
FBX_MATERIAL_FILE = 'materialfile'
FBX_ASSIGNED_GEO = 'assigned'

# Threshold values for baked vertex color
# id mask images when no UVs present
EMPTY_IMAGE_LOW = 260000
EMPTY_IMAGE_LOW = 270000
