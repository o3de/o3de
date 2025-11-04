#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief This module contains several common material utilities/operations. """

##
# @file maya_materials.py
#
# @brief This module contains several common materials utilities/operations for use with Maya.
#
# @section Maya Utilities Description
# Python module containing most of the commonly needed helper functions for working with Maya scenes in the DCCsi.
# DCCsi (Digital Content Creation Scripting Interface)
#
# @section Maya Materials Notes
# - Comments are Doxygen compatible

# standard imports
import os
import logging as _logging
from pathlib import Path

# o3de, dccsi imports
from DccScriptingInterface.azpy.dcc.maya.helpers import convert_arnold_material as arnold
from DccScriptingInterface.azpy.dcc.maya.helpers import convert_stingray_material as stingray

# this breaks if you aren't running pycharm / pydev
# need to wrap this safely with an envar or another mechanism
# from azpy.dcc.maya.utils import pycharm_debugger

# maya, pyside imports, etc
import maya.cmds as mc

# module global scope
_LOGGER = _logging.getLogger('DCCsi.azpy.dcc.maya.utils.maya_materials')


# TODO - You need to provide for creating new files while Maya is already open
# TODO - You could add this tool to the shelf and incorporate that as well

supported_material_types = [
    'StingrayPBS',
    'aiStandardSurface'
]


def create_preview_files(target_files, database_values):
    """! This function is run from the 'material_preview_builder' tool to build Maya preview files using fbx files,
    recorded materials and attached texture files from legacy .mtl file information.

    @param target_files List of fbx files to process
    @param database_values Values extracted from legacy .mtl files that assist with material assignments
    """
    _LOGGER.info('Database values passed: {}'.format(database_values))
    for file in target_files:
        _LOGGER.info('Creating Maya preview file:::> {}'.format(file))
        maya_file_path = get_maya_file_path(file)
        mc.file(new=True, force=True)
        mc.file(file, i=True, type="FBX")
        set_scene_attributes()
        set_stingray_materials(database_values)
        mc.file(rename=maya_file_path)
        mc.file(save=True, type='mayaAscii', force=True)


def process_single_file(action: str, target_file: Path, data: list):
    """! Provides mechanism for compound material actions to be performed in a scene.

    @param action: Description of the task that needs to be performed
    @param target_file: Target file to perform actions to
    @param data: Information needed to perform the specified action
    """
    if action == 'stingray_texture_assignment':
        mc.file(target_file, o=True, force=True)
        material_info = get_scene_material_information()

        for material in data['materials']:
            try:
                if material in material_info.keys():
                    pass
            except Exception as e:
                _LOGGER.info('Material conversion failed [{}]-- Error: {}'.format(material, e))


def refresh_stingray_attributes():
    """! This function is called to initialize/import the Stingray material support that does not initialize
    by default, but is needed to programmatically assign stingray materials.

    @return Boolean Success of refresh attempt
    """
    scene_geo = mc.ls(v=True, geometry=True)

    for target_mesh in scene_geo:
        shading_groups = list(set(mc.listConnections(target_mesh, type='shadingEngine')))
        for sg in shading_groups:
            materials = mc.ls(mc.listConnections(sg), materials=True)
            for material in materials:
                material_name = str(material)
                material_attributes = {}

                # Get shader attributes
                for material_attribute in mc.listAttr(material_name, s=True, iu=True):
                    try:
                        target_value = mc.getAttr('{}.{}'.format(material_name, material_attribute))
                        material_attributes[material_attribute] = target_value
                    except Exception:
                        pass

                for attr_name, attr_value in material_attributes.items():
                    if attr_name == 'use_color_map':
                        return True
    return False


def open_target_scene(scene_path):
    if scene_path != get_current_scene():
        _LOGGER.info('Scenes dont match- need to open new scene')
    else:
        _LOGGER.info('Same scene. Skipping...')
        return scene_path


def run_operation(process_dictionary, operation, output):
    _LOGGER.info(f'Run operation fired::::::::\nOperation: {operation}\nOutput: {output} ')


def get_materials_in_scene():
    """! Audits scene to gather all materials present in the Hypershade.

    @return list
    """
    material_list = []
    for shading_engine in mc.ls(type='shadingEngine'):
        if mc.sets(shading_engine, q=True):
            for material in mc.ls(mc.listConnections(shading_engine), materials=True):
                material_list.append(material)
    return material_list


def get_material_assignments():
    """! Gathers all materials in a scene and which mesh they are assigned to.

    @return dict - material keys and assigned mesh values
    """
    assignments = {}
    scene_geo = mc.ls(v=True, geometry=True)
    for mesh in scene_geo:
        mesh_materials = get_mesh_materials(mesh)
        for material in mesh_materials:
            if material not in assignments.keys():
                assignments[material] = [mesh]
            else:
                assignments[material].append(mesh)
    return assignments


def get_materials(selected_object):
    shader = mc.listConnections(selected_object, type='shadingEngine')
    materials = mc.ls(mc.listConnections(shader), materials=True)
    return materials


def get_material_type(target_material):
    return mc.nodeType(target_material)


def get_current_scene():
    current_scene = mc.file(q=True, sceneName=True)
    return Path(current_scene)


def get_scene_material_information():
    """! Gathers several points of information about materials in current Maya scene.

    @return dict - Material Info: Material Name, Shading Group, Material Type, Textures, Assignments
    """
    material_information = {}
    scene_materials = get_materials_in_scene()
    material_assignments = get_material_assignments()
    for material_name in scene_materials:
        material_listing = {
            'shading_group': get_shading_group(material_name),
            'material_type': mc.nodeType(material_name),
            'assigned_geo': material_assignments[material_name],
            # 'material_textures': get_material_textures(mc.nodeType(material_name), {'shading_group':
            #                                                                         get_shading_group(material_name),
            #                                                                         'name': material_name})
        }
        material_information[material_name] = material_listing
    return material_information


def get_shading_group(material_name):
    """! Convenience function for obtaining the shader that the specified material (as an argument) is attached to.

    @param material_name Takes the material name as an argument to get associated shader object
    @return Maya Shading group
    """
    return mc.listConnections(material_name, type='shadingEngine')[0]


def get_mesh_materials(target_mesh):
    """! Gathers a list of all materials attached to each mesh's shader.

    @param target_mesh The target mesh to pull attached material information from.
    @return list - Unique material values attached to the mesh passed as an argument.
    """
    mc.select(target_mesh, r=True)
    target_nodes = mc.ls(sl=True, dag=True, s=True)
    shading_group = mc.listConnections(target_nodes, type='shadingEngine')
    materials = mc.ls(mc.listConnections(shading_group), materials=1)
    return list(set(materials))


def get_maya_file_path(fbx_file_path):
    """! Convenience function for creating a maya file path based on an existing FBX path.
    @param fbx_file_path Path to the existing FBX file
    @return str - File path
    """
    return os.path.splitext(fbx_file_path)[0] + '.ma'


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


def enable_stingray_textures(material_name: str):
    mc.setAttr('{}.use_color_map'.format(material_name), 1)
    mc.setAttr('{}.use_normal_map'.format(material_name), 1)
    mc.setAttr('{}.use_metallic_map'.format(material_name), 1)
    mc.setAttr('{}.use_roughness_map'.format(material_name), 1)
    mc.setAttr('{}.use_emissive_map'.format(material_name), 1)


def get_material(material_name: str) -> list:
    """! Gets material assignments and swaps legacy formatted materials with Stingray PBS materials. This will delete
    non-Stingray materials and replace with Stingray. Currently Stingray material attributes can only be queried
    and/or altered if user manually initiates them in an open Maya system first. Bug has been filed with Autodesk.

    @param material_name Target material name
    @return list - Material, Shading Group
    """
    _LOGGER.info('Shaders in scene: {}'.format(get_materials_in_scene()))
    sha = None
    sg = None
    if material_name in get_materials_in_scene():
        _LOGGER.info('Deleting material: {}'.format(material_name))
        mc.delete(material_name)

    _LOGGER.info('Creating Stingray material: {}'.format(material_name))
    try:
        sha = mc.shadingNode('StingrayPBS', asShader=True, name=material_name)
        sg = mc.sets(renderable=True, noSurfaceShader=True, empty=True)
        mc.connectAttr(sha + '.outColor', sg + '.surfaceShader', force=True)
        enable_stingray_textures(material_name)
    except Exception as e:
        _LOGGER.info('Shader creation failed: {}'.format(e))
    return [sha, sg]


def get_material_info(target_material: str):
    mc.select(target_material, r=True)
    material_type = get_material_type(mc.ls(sl=True, long=True) or [])
    if material_type == 'aiStandardSurface':
        return arnold.get_material_info(target_material)
    elif material_type == 'StingrayPBS':
        return stingray.get_material_info(target_material)


def get_arnold_material_info(material_name: str):
    _LOGGER.info(f'Getting arnold material information')


def get_scene_objects():
    mesh_objects = mc.ls(type='mesh')
    return mesh_objects


def get_selected_objects():
    """! Filters selection to just meshes in the event that other items exist in the current selection."""
    return mc.filterExpand(sm=12)
    # try:
    #     for obj in selected_objects:
    #         _LOGGER.info(f'Selected: {obj}  Type: {mc.objectType(obj)}')
    #     return selected_objects
    # except TypeError:
    #     return []


def set_selected_objects(objects_list):
    for element in objects_list:
        mc.select(element, add=True)


def set_scene_attributes():
    """! Preps Maya scene for Stingray Materials preview. """
    set_camera_attributes()
    mc.currentUnit(linear='meter')
    mc.grid(reset=True)


def set_camera_attributes():
    """! Corrects Near/Far Clip planes so visual display of objects aren't clipped if user scene units were set to
    Maya default of centimeters. """
    camera_list = mc.ls(type='camera')
    attributes = {'nearClipPlane': 0.01, 'farClipPlane': 1000000}
    for c in camera_list:
        for attrName in attributes.keys():
            mc.setAttr('{}.{}'.format(c, attrName), attributes[attrName])


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


def set_material(material_name: str, shading_group: str, assignment_list: list):
    """! Assigns specified material to specified mesh

    @param material_name Material name
    @param shading_group Shading Group
    @param assignment_list List of geometry to assign shader to
    """
    assignment_list = list(set([x.replace('.', '|') for x in assignment_list]))
    _LOGGER.info('\n_\nSET MATERIAL: {} {{{{{{{{{{{{{{{{{{{{{{'.format(material_name))
    _LOGGER.info('Assignment list--> {}'.format(assignment_list))
    for item in assignment_list:
        try:
            mc.sets(item, e=True, forceElement=shading_group)
        except Exception as e:
            _LOGGER.info('Material assignment failed: {}'.format(e))
            pass


def set_textures(material_name: str, texture_dict: dict):
    """
    Plugs texture files into texture slots in the shader for specified material name. Currently due to the
    aforementioned bug this functionality does not work as intended, but it remains for when Autodesk supplies a fix
    :param material_name:
    :param texture_list:
    :return:
    """
    shader_translation_keys = {
        'BaseColor': 'color',
        'Roughness': 'roughness',
        'Metallic': 'metallic',
        'Emissive': 'emissive',
        'Normal': 'normal'
    }

    _LOGGER.info('\n_\nSET_TEXTURE_MAPS {{{{{{{{{{{{{{{{{{{{{{')
    _LOGGER.info('Material--> {}'.format(material_name))
    for texture_type, texture_path in texture_dict.items():
        try:
            file_count = len(mc.ls(type='file')) + 1
            texture_file = 'file{}'.format(file_count)
            mc.shadingNode('file', asTexture=True, name=texture_file)
            mc.setAttr('{}.fileTextureName'.format(texture_file), texture_path, type="string")
            mc.setAttr('{}.use_{}_map'.format(material_name, shader_translation_keys[texture_type]), 1)
            mc.connectAttr('{}.outColor'.format(texture_file),
                           '{}.TEX_{}_map'.format(material_name, shader_translation_keys[texture_type]), force=True)

            _LOGGER.info('Texture type: {}   Texture path: {}'.format(texture_type, texture_path))
            _LOGGER.info('TexturePath: {}'.format(texture_path))
            _LOGGER.info('Attributes: {}'.format(mc.listAttr(material_name)))

        except Exception as e:
            _LOGGER.info('Conversion failed: {}'.format(e))


def cleanup(target_objects):
    mc.select(target_objects, replace=True)
