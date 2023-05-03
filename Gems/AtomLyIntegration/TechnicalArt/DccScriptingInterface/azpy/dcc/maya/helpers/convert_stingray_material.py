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

from DccScriptingInterface.azpy.dcc.maya.helpers import maya_materials_conversion
from DccScriptingInterface.azpy import general_utils as helpers
import logging as _logging
import maya.cmds as mc
from pathlib import Path


_LOGGER = _logging.getLogger('DCCsi.azpy.dcc.maya.helpers.convert_stingray_material')


stingray_file_connections = [
    'TEX_ao_map',
    'TEX_color_map',
    'TEX_metallic_map',
    'TEX_normal_map',
    'TEX_roughness_map',
    'TEX_emissive'
]


def enable_stingray_textures(material_name: str):
    mc.setAttr('{}.use_color_map'.format(material_name), 1)
    mc.setAttr('{}.use_normal_map'.format(material_name), 1)
    mc.setAttr('{}.use_metallic_map'.format(material_name), 1)
    mc.setAttr('{}.use_roughness_map'.format(material_name), 1)
    mc.setAttr('{}.use_emissive_map'.format(material_name), 1)


def get_material_info(target_material: str):
    material_textures = get_material_textures(target_material)
    target_keys = get_transferable_properties()
    material_settings = maya_materials_conversion.get_material_attributes(target_material, target_keys)
    material_info = {
        'material_type': 'StingrayPBS',
        'dcc_application': 'Maya',
        'settings': material_settings,
        'textures': material_textures
    }
    return material_info


def get_material_textures(target_material: str):
    material_textures_dict = {}
    nodes = mc.listConnections(target_material, source=True, destination=False, skipConversionNodes=True) or {}
    for node in nodes:
        texture_path = Path(mc.getAttr('{}.fileTextureName'.format(node.split('.')[0])))
        if texture_path.suffix != 'dds':
            texture_key = get_texture_key(node, target_material)
            if texture_key:
                material_textures_dict[texture_key] = {
                    'node': node,
                    'path': helpers.get_clean_path(texture_path.__str__())
                }
    return material_textures_dict


def get_file_node(source_node):
    file_node = mc.listConnections(source_node, type='file', c=True)[1]
    return file_node


def get_texture_key(source_node, target_material):
    active_texture_slots = get_active_texture_slots(get_shader(target_material))
    key = mc.listConnections(source_node, plugs=1, destination=1)[0]
    texture_slot_name = str(key).split('.')[-1]
    if texture_slot_name in active_texture_slots:
        texture_base_type = texture_slot_name.split('_')[1]
        texture_key = 'baseColor' if texture_base_type == 'color' else texture_base_type
        return texture_key
    return None


def get_shader(material_name):
    """! Convenience function for obtaining the shader that the specified material (as an argument)
    is attached to.

    @param material_name Takes the material name as an argument to get associated shader object
    """
    connections = mc.listConnections(material_name, type='shadingEngine')[0]
    shader_name = '{}.surfaceShader'.format(connections)
    shader = mc.listConnections(shader_name)[0]
    return shader


def get_active_texture_slots(shader):
    """
    Helper function for checking which texture slots are enabled via 'use_<textureSlot>_map' attributes

    @param shader: The target shader object to analyze
    @return Complete set (in the form of two dictionaries) of file connections and material attribute values
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


def get_texture_type(plug_name):
    texture_types = {
        'TEX_ao_map': 'AmbientOcclusion',
        'TEX_color_map': 'BaseColor',
        'TEX_metallic_map': 'Metallic',
        'TEX_normal_map': 'Normal',
        'TEX_roughness_map': 'Roughness',
        'TEX_emissive': 'Emissive'
    }
    plug = plug_name.split('.')[-1]
    if plug in texture_types.keys():
        return texture_types[plug]
    return None


def get_transferable_properties():
    """! This is a listing of properties on an StingrayPBS material that can be directly transferred to an
    O3DE material. When parsing the material attributes in Maya these values will be extracted and added to scene
    material information. "*" Indicates a texture slot- if textures don't exist this likely can be controlled with
    RGB values. This should probably be moved to a constants file once we work out how we are set everything up
    """
    target_properties = {

        'TEX_color_map': 'baseColor.textureMap',        # Color Map
        'use_color_map': 'baseColor.useTexture',        # Use Color Map
        'base_color': 'baseColor.color',                # Base Color
        'TEX_normal_map': 'normal.textureMap',          # Normal Map
        'use_normal_map': 'normal.useTexture',          # Use Normal Map
        'TEX_metallic_map': 'metallic.textureMap',      # Metallic Map
        'use_metallic_map': 'metallic.useTexture',      # Use Metallic Map
        'metallic': 'metallic.factor',                  # Metallic
        'TEX_roughness_map': 'roughness.textureMap',    # Roughness Map
        'use_roughness_map': 'roughness.useTexture',    # Use Roughness Map
        'roughness': 'roughness.factor',                # Roughness
        'TEX_emissive_map': 'emissive.textureMap',      # Emissive Map
        'use_emissive_map': 'emissive.useTexture',      # Use Emissive Map
        'emissive': 'emissive.color',                   # Emissive
        'emissive_intensity': 'emissive.intensity',     # Emissive Intensity
        'TEX_ao_map': 'occlusion.diffuseTextureMap',    # Ao Map
        'use_ao_map': 'occlusion.diffuseUseTexture',    # Use Ao Map
        'uv_offsetX': 'uv.offsetU',                     # UV Offset (first listing)
        'uv_offsetY': 'uv.offsetV',                     # UV Offset (second listing)
        'uv_scaleX': 'uv.scale'                         # Uv Scale (I'm just using scaleX value to control... O3DE
                                                        # Has a single control, where Stingray has two)
    }
    return target_properties


def set_stingray_materials(material_dict: dict):
    for material_name, material_values in material_dict.items():
        if 'textures' in material_values.keys():

            sha, sg = get_material(material_name)
            material_textures = material_values['textures']
            material_assignments = material_values['assigned']
            material_modifications = material_values['modifications']

            _LOGGER.info('+++++++++++++++++++++++++++++')
            _LOGGER.info('Converting material: {}'.format(material_name))
            _LOGGER.info('Material Textures: {}'.format(material_textures))
            _LOGGER.info('Material Assignments: {}'.format(material_assignments))

            try:
                set_material(material_name, sg, material_assignments)
                set_textures(material_name, material_textures)
                if material_modifications:
                    _LOGGER.info('\n++++++++++++++\nMaterialMods: {}\n++++++++++++++'.format(material_modifications))
            except Exception as e:
                _LOGGER.info('Material Assignment failed: {}'.format(e))




