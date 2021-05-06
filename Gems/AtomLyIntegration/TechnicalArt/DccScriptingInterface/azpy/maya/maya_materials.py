# coding:utf-8
#!/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# -- This line is 75 characters -------------------------------------------
# -- Standard Python modules --
import os
import socket
import time
import logging as _logging
import json

# -- External Python modules --
import maya.cmds as mc

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.maya.maya_materials'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


# TODO - Revamp original processing- remove create maya files, maybe this should be imported into separate Maya tool?
# TODO - You need to provide for creating new files while Maya is already open
# TODO - You could add this tool to the shelf and incorporate that as well
# TODO - Create a modal dialog that asks if you want to convert current file, current directory, or all directories


def process_single_file(action, target_file, data):
    """
    Provides mechanism for compound material actions to be performed in a scene
    :param action: Description of the task that needs to be performed
    :param target_file: Target file to perform actions to
    :param data: Information needed to perform the specified action
    :return:
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


def process_multiple_files(action, target_files, data):
    """
        Provides mechanism for compound material actions to be performed for a list of Maya scenes
        :param action: Description of the task that needs to be performed to the target_files list
        :param target_files: List of files to perform actions to
        :param data: Information needed to perform the specified action
        :return:
        """
    pass


def get_materials_in_scene():
    """
    Audits scene to gather all materials present in the Hypershade, and returns them in a list
    :return:
    """
    material_list = []
    for shading_engine in mc.ls(type='shadingEngine'):
        if mc.sets(shading_engine, q=True):
            for material in mc.ls(mc.listConnections(shading_engine), materials=True):
                material_list.append(material)
    return material_list


def get_material_assignments():
    """
    Gathers all materials in a scene and which mesh they are assigned to
    :return: Returns dictionary of material keys and assigned mesh values
    """
    assignments = {}
    scene_geo = mc.ls(v=True, geometry=True)
    for mesh in scene_geo:
        mesh_material = get_mesh_materials(mesh)[0]
        if mesh_material not in assignments.keys():
            assignments[mesh_material] = [mesh]
        else:
            assignments[mesh_material].append(mesh)
    return assignments


def get_scene_material_information():
    """
    Gathers several points of information about materials in current Maya scene
    :return:
    """
    material_information = {}
    scene_materials = get_materials_in_scene()
    material_assignments = get_material_assignments()
    for material_name in scene_materials:
        material_listing = {
            'shading_group': get_shading_group(material_name),
            'material_type': mc.nodeType(material_name),
            'material_textures': get_material_textures(material_name),
            'assigned_geo': material_assignments[material_name]
        }
        material_information[material_name] = material_listing
    return material_information


def get_shading_group(material_name):
    """
    Convenience function for obtaining the shader that the specified material (as an argument)
    is attached to.

    :param material_name: Takes the material name as an argument to get associated shader object
    :return: Shading group of assigned material_name
    """
    return mc.listConnections(material_name, type='shadingEngine')[0]


def get_mesh_materials(target_mesh):
    """
    Gathers a list of all materials attached to each mesh's shader

    :param target_mesh: The target mesh to pull attached material information from.
    :return: List of unique material values attached to the mesh passed as an argument.
    """
    shading_group = mc.listConnections(target_mesh, type='shadingEngine')
    materials = mc.ls(mc.listConnections(shading_group), materials=1)
    return list(set(materials))


def get_material_textures(material_name):
    """
    Gets the texture file list for specified material_name in Maya scene
    :param material_name: The target material from which to gather assigned texture files
    :return:
    """
    material_textures = {}
    file_nodes = mc.listConnections(material_name, type='file', c=False)
    for node in file_nodes:
        try:
            plug = mc.listConnections(node, plugs=1, source=1)[0]
            texture_name = mc.getAttr("{}.fileTextureName".format(node))
            material_textures[plug] = texture_name
        except Exception:
            pass
    return material_textures


def set_material(material_name, material_type, shading_group, assignment_list):
    """
    Assigns specified material to specified mesh
    :param material_name: Material name
    :param material_type: Material name (i.e. Stingray, Blinn, Lambert)
    :param shading_group: Shading Group
    :param assignment_list: List of geometry to assign shader to
    :return:
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


def set_textures(material_name, texture_list):
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
    for texture_type, texture_path in texture_list.items():
        try:
            _LOGGER.info('Texture type: {}   Texture path: {}'.format(texture_type, texture_path))
            file_count = len(mc.ls(type='file')) + 1
            texture_file = 'file{}'.format(file_count)
            mc.shadingNode('file', asTexture=True, name=texture_file)
            mc.setAttr('{}.fileTextureName'.format(texture_file), texture_path, type="string")
            _LOGGER.info('TexturePath: {}'.format(texture_path))
            _LOGGER.info('Attributes: {}'.format(mc.listAttr(material_name)))
            mc.setAttr('{}.use_{}_map'.format(material_name, shader_translation_keys[texture_type]), 1)
            mc.connectAttr('{}.outColor'.format(texture_file),
                           '{}.TEX_{}_map'.format(material_name, shader_translation_keys[texture_type]), force=True)
        except Exception as e:
            _LOGGER.info('Conversion failed: {}'.format(e))


def assign_stingray_materials(action):
    _LOGGER.info('AssignStingrayMaterials fired in maya_materials: {}'.format(action))
