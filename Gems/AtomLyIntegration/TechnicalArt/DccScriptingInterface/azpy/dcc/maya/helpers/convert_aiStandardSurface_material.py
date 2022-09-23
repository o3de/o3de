#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""! @brief This module assists with material operations involving Maya aiStandardSurface (Arnold) materials. """

##
# @file convert_aiStandardSurface_material.py
#
# @brief This module contains several common materials utilities/operations for extracting/converting material
# information contained in Maya aiStandardSurface. This includes gathering file texture information, and input/output
# connections, and other material property settings relevant for materials conversion to O3DE
#
# @section Maya Materials Notes
# - Comments are Doxygen compatible

from DccScriptingInterface.azpy.dcc.maya.helpers import maya_materials_conversion
from DccScriptingInterface.azpy import general_utils as helpers
import logging as _logging
import maya.cmds as mc

_LOGGER = _logging.getLogger('DCCsi.azpy.dcc.maya.helpers.convert_aiStandardSurface_material')


def get_material_info(target_material: str):
    material_textures = get_material_textures(target_material)
    target_keys = get_transferable_properties()
    material_settings = maya_materials_conversion.get_material_attributes(target_material, target_keys)
    material_info = {
        'material_type': 'aiStandardSurface',
        'dcc_application': 'Maya',
        'settings': material_settings,
        'textures': material_textures
    }
    return material_info


def get_material_textures(target_material: str):
    arnold_file_connections = {
        'metalness':         None,
        'baseColor':         None,
        'specularRoughness': None,
        'emissionColor':     None,
        'normal':           None,
        'alpha':             None,
        'subsurfaceColor':   None
    }

    nodes = mc.listConnections(target_material, source=True, destination=False, skipConversionNodes=True) or {}
    for node in nodes:
        if 'file' not in node.lower():
            node = get_file_node(node)

        node_connections = mc.listConnections(node, p=True)
        if node_connections[0] == 'defaultColorMgtGlobals.cmEnabled':
            texture_slot = get_texture_slot(node_connections)
        else:
            parent_node = mc.listConnections(node, p=True)[0]
            texture_slot = parent_node.split('.')[-1]
        texture_path = mc.getAttr(f'{node}.fileTextureName')
        arnold_file_connections[texture_slot] = {'node': node, 'path': helpers.get_clean_path(texture_path)}

    return arnold_file_connections


def get_texture_slot(node_connections):
    supported_connections = [
        'subsurfaceColor',
        'bumpValue'
    ]
    for connection in node_connections:
        connection = connection.split('.')[-1]
        if connection in supported_connections:
            connection = 'normal' if connection == 'bumpValue' else connection
            return connection


def get_file_node(source_node):
    file_node = mc.listConnections(source_node, type='file', c=True)[1]
    return file_node


def get_transferable_properties():
    """! This is a listing of properties on an aiStandardSurface material that can be directly transferred to an
    O3DE material. When parsing the material attributes in Maya these values will be extracted and added to scene
    material information. "*" Indicates a texture slot- if textures don't exist this likely can be controlled with
    RGB values. This should probably be moved to a constants file once we work out how we are set everything up
    """

    # Please note: I'm unsure about the differences in mapping between "roughness" and "specular roughness with the
    # Arnold shader. I'm pretty sure standard "roughness" mapping is found under "Specular" in the material properties
    # but this is then contends with specularF0 roughness... need to get an answer on this
    target_properties = {
        'base': 'baseColor.factor',                     # BASE - Weight
        'baseColor': 'baseColor.textureMap',            # BASE - * Color
        'diffuseRoughness': '',                         # BASE - Roughness
        'metalness': 'metallic.textureMap',             # BASE - Metalness

        'specular': 'roughness.factor',                 # SPECULAR - Weight
        'specularColor': '',                            # SPECULAR - Color
        'specularRoughness': 'roughness.textureMap',    # SPECULAR - * Roughness
        'specularIOR': '',                              # SPECULAR - IOR
        'specularAnisotropy': '',                       # SPECULAR - Anisotropy
        'specularRotation': '',                         # SPECULAR - Rotation

        'subsurface': '',                               # SUBSURFACE - Weight
        'subsurfaceColor': '',                          # SUBSURFACE - * Subsurface Color
        'subsurfaceRadius': '',                         # SUBSURFACE - Radius
        'subsurfaceScale': '',                          # SUBSURFACE - Scale

        'coat': 'clearCoat.factor',                     # COAT - Weight
        'coatColor': 'clearCoat.influenceMap',          # COAT - Color
        'coatRoughness': 'clearCoat.roughness',         # COAT - Roughness
        'coatIOR': '',                                  # COAT - IOR
        'coatAnisotropy': '',                           # COAT - Anisotropy
        'coatRotation': '',                             # COAT - Rotation

        'sheen': '',                                    # SHEEN - Weight
        'sheenColor': '',                               # SHEEN - Color
        'sheenRoughness': '',                           # SHEEN - Roughness

        'emission': 'emissive.intensity',               # EMISSION - Weight
        'emissionColor': 'emissive.textureMap',         # EMISSION - * Color

        'opacity': 'opacity.textureMap',                # GEOMETRY - Opacity
        'normalCamera': 'normal.textureMap',            # GEOMETRY - * Bump Mapping (Normal Map input)

        'aiEnableMatte': '',                            # MATTE - * Enable Matte (Alpha)
        'aiMatteColor': 'opacity.textureMap',           # MATTE - Matte Color
        'aiMatteColorA': 'opacity.factor',              # MATTE - Matte Opacity
    }
    return target_properties


