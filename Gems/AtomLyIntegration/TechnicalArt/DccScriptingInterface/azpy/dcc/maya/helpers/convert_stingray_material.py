# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief This module assists with material operations involving Maya StingrayPBS materials. """

##
# @file convert_stingray_material.py
#
# @brief This module contains several common materials utilities/operations for extracting/converting material
# information contained in Maya StingrayPBS. This includes gathering file texture information, and input/output
# connections, and other material property settings relevant for materials conversion to O3DE
#
# @section Maya Materials Notes
# - Comments are Doxygen compatible

import logging as _logging
import maya.cmds as mc
from pathlib import Path

_LOGGER = _logging.getLogger('azpy.dcc.maya.helpers.convert_stingray_material')

stingray_file_connections = [
    'TEX_ao_map',
    'TEX_color_map',
    'TEX_metallic_map',
    'TEX_normal_map',
    'TEX_roughness_map',
    'TEX_emissive'
]


def get_material_info(target_material: str):
    material_textures = get_material_textures(target_material)
    return get_material_settings(material_textures)


def get_material_textures(target_material: str):
    material_textures_dict = {}
    nodes = mc.listConnections(target_material, source=True, destination=False, skipConversionNodes=True) or {}
    for node in nodes:
        texture_path = Path(mc.getAttr('{}.fileTextureName'.format(node.split('.')[0])))
        if texture_path.suffix != 'dds':
            texture_key = get_texture_key(node, target_material)
            if texture_key:
                material_textures_dict[texture_key] = {'node': node, 'path': texture_path}

    return material_textures_dict


def get_material_settings(material_textures):
    return material_textures


def get_file_node(source_node):
    file_node = mc.listConnections(source_node, type='file', c=True)[1]
    return file_node


def get_texture_key(source_node, target_material):
    active_texture_slots = get_active_texture_slots(get_shader(target_material))
    key = mc.listConnections(source_node, plugs=1, destination=1)[-1]
    texture_slot_name = str(key).split('.')[-1]
    if texture_slot_name in active_texture_slots:
        texture_base_type = texture_slot_name.split('_')[1]
        texture_key = 'baseColor' if texture_base_type == 'color' else texture_base_type
        return texture_key
    return None


def get_shader(material_name):
    """
    Convenience function for obtaining the shader that the specified material (as an argument)
    is attached to.

    :param material_name: Takes the material name as an argument to get associated shader object
    :return:
    """
    connections = mc.listConnections(material_name, type='shadingEngine')[0]
    shader_name = '{}.surfaceShader'.format(connections)
    shader = mc.listConnections(shader_name)[0]
    return shader


def get_active_texture_slots(shader):
    """
    Helper function for checking which texture slots are enabled via 'use_<textureSlot>_map' attributes

    :param shader: The target shader object to analyze
    :return: Complete set (in the form of two dictionaries) of file connections and material attribute values
    """
    slot_list = [x[3:] for x in stingray_file_connections]
    active_slots = []
    for shader_attribute in mc.listAttr(shader, s=True, iu=True):
        try:
            for index, slot in enumerate(slot_list):
                target_attribute = mc.getAttr('{}.{}'.format(shader, shader_attribute))
                target_property = f'use{slot}'
                if target_property == shader_attribute and target_attribute == 1.0:
                    active_slots.append(stingray_file_connections[index])
        except Exception as e:
            _LOGGER.error('MayaAttributeError: {}'.format(e))

    return active_slots

