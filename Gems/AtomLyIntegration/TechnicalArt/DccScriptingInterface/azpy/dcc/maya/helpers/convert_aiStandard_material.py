#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief This module assists with material operations involving Maya aiStandard (Arnold) materials. """

##
# @file convert_aiStandard_material.py
#
# @brief This module contains several common materials utilities/operations for extracting/converting material
# information contained in Maya aiStandardSurface. This includes gathering file texture information, and input/output
# connections, and other material property settings relevant for materials conversion to O3DE
#
# @section Maya Materials Notes
# - Comments are Doxygen compatible

from DccScriptingInterface.azpy.dcc.maya.helpers import maya_materials_conversion
from DccScriptingInterface.azpy import general_utils as helpers
from pathlib import Path
import logging as _logging
import maya.cmds as mc


_LOGGER = _logging.getLogger('DCCsi.azpy.dcc.maya.helpers.convert_aiStandard_material')


def get_material_info(target_material: str):
    material_textures = get_material_textures(target_material)
    target_keys = get_transferable_properties()
    material_settings = maya_materials_conversion.get_material_attributes(target_material, target_keys)
    material_info = {
        'material_type': 'aiStandard',
        'dcc_application': 'Maya',
        'settings': material_settings,
        'textures': material_textures
    }
    return material_info


def get_material_textures(target_material: str):
    arnold_file_connections = {
        'metalness':            None,
        'color':                None,
        'specularRoughness':    None,
        'emissionColor':        None,
        'normal':               None,
        'aiMatteColor':         None,
        'KsssColor':            None
    }

    nodes = mc.listConnections(target_material, source=True, destination=False, skipConversionNodes=True) or {}
    for node in nodes:
        parent_node = mc.listConnections(node, p=True)[0]
        texture_slot = parent_node.split('.')[-1]

        if texture_slot == 'normal':
            slot_name = mc.listConnections(parent_node)[0]
            texture_path = mc.getAttr(f'{slot_name}.fileTextureName')
        else:
            texture_path = mc.getAttr(f'{node}.fileTextureName')
        arnold_file_connections[texture_slot] = {'node': node, 'path': helpers.get_clean_path(texture_path)}

    arnold_file_connections = get_unassigned_textures(arnold_file_connections)
    return arnold_file_connections


def get_unassigned_textures(textures_dict: dict):
    """! This runs a second pass of the location of registered textures in the event that additional related texture
    files may exist, but are either unassigned or the script has difficulty sourcing connections

    @param textures_dict This dictionary is passed after a material has been parsed, just as a cleanup pass, so that
    when migrating assets, no textures are left behind that might otherwise get lost in the transfer to a new location.
    """
    search_values = {}
    found_entries = {}
    # Get search info //------------------------------------------------>>
    for texture_type, texture_values in textures_dict.items():
        if texture_values:
            if texture_values['path']:
                found_entries[texture_type] = texture_values['path']
                if not search_values:
                    base_directory = Path(texture_values['path']).parent
                    search_values['base_directory'] = base_directory
                    base_texture_name = Path(texture_values['path']).stem
                    texture_parts = base_texture_name.split('_')
                    texture_prefix = '_'.join(texture_parts[:-1])
                    search_values['prefix'] = texture_prefix

    # Filter //---------------------------------------------------------->>
    p = Path(search_values['base_directory']).glob('**/*')
    target_files = [x for x in p if x.is_file()]
    for index, file_path in enumerate(target_files):
        if file_path.suffix in ['.jpg', '.png', '.tif', 'tga']:
            file_name = file_path.stem
            file_path = helpers.get_clean_path(str(file_path))
            if file_name.startswith(search_values['prefix']):
                texture_type = file_name.split('_')[-1]
                if not [k for k, v in found_entries.items() if Path(v) == Path(file_path)]:
                    textures_dict[get_texture_type(texture_type)] = {'path': file_path}
    return textures_dict


def get_file_node(source_node):
    file_node = mc.listConnections(source_node, type='file', c=True)[1]
    return file_node


def get_texture_type(texture_type):
    """! Eventually we are going to want to add suffix matching for texture types in a centralized constants file
    location. For the purposes of converting the Intel Sponza scene, this will provide proper texture assignments

    @param texture_type Found in file texture name after the final underscore (but before extension)
    Example: file_<texture_type>.png
    """
    texture_types = get_transferable_properties()
    for type in texture_types.keys():
        if type.lower() == texture_type.lower():
            return type
    return texture_type


def get_transferable_properties():
    """! This is a listing of properties on an aiStandardSurface material that can be directly transferred to an
    O3DE material. When parsing the material attributes in Maya these values will be extracted and added to scene
    material information. "*" Indicates a texture slot- if textures don't exist this likely can be controlled with
    RGB values. This should probably be moved to a constants file once we work out how we are set everything up
    """
    target_properties = {
        'color': 'baseColor.textureMap',                # BASE - * Color
        'metalness': 'metallic.textureMap',             # BASE - Metalness
        'specularRoughness': 'roughness.textureMap',    # SPECULAR - * Roughness
        'emissionColor': 'emissive.textureMap',         # EMISSION - * Color
        'normal': 'normal.textureMap',                  # GEOMETRY - * Bump Mapping (Normal Map input)
        'aiMatteColor': 'opacity.textureMap',           # MATTE - Matte Color
    }
    return target_properties


