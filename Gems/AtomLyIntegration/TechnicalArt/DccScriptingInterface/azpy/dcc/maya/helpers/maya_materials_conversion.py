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
# @file maya_materials_conversion.py
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
from DccScriptingInterface.azpy.dcc.maya.helpers import convert_aiStandard_material as aiStandard
from DccScriptingInterface.azpy.dcc.maya.helpers import convert_aiStandardSurface_material as aiStandardSurface
from DccScriptingInterface.azpy.dcc.maya.helpers import convert_stingray_material as stingray

# this breaks if you aren't running pycharm / pydev
# need to wrap this safely with an envar or another mechanism
# from azpy.dcc.maya.utils import pycharm_debugger

# maya, pyside imports, etc
import maya.cmds as mc
import maya.mel

_LOGGER = _logging.getLogger('DCCsi.azpy.dcc.maya.helpers.maya_materials_conversion')


# TODO - You need to provide for creating new files while Maya is already open
# TODO - You could add this tool to the shelf and incorporate that as well

supported_material_types = [
    'StingrayPBS',
    'aiStandardSurface',
    'aiStandard'
]

# Activate FBX plugin
if 'fbxmaya' not in mc.pluginInfo(query=True, listPlugins=True):
    mc.loadPlugin("fbxmaya")


def prepare_material_export(target_mesh, export_location):
    mesh_materials = get_mesh_materials(target_mesh)
    export_path = get_mesh_export_path(target_mesh, export_location) if export_location else ''
    return export_path, mesh_materials


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
    _LOGGER.info(f'\n\n:::::::::::::::::::\nRun operation fired\n:::::::::::::::::::\n\nOperation Type: {operation}\n')
    if operation == 'convert':
        _LOGGER.info('* Set up conversion here *')


def get_object_hierarchy(target_object):
    _LOGGER.info(f'================================')
    object_hierarchy = [[get_object_type(target_object), target_object]]
    while True:
        parent = mc.listRelatives(target_object, allParents=True)
        if parent:
            parent_type = get_object_type(parent[0])
            _LOGGER.info(f'[{target_object}] -- Parent type: {parent_type}')
            if parent_type in ['mesh', 'group']:
                object_hierarchy.append([parent_type, parent[0]])
                target_object = parent[0]
            else:
                return object_hierarchy
        else:
            return object_hierarchy


def get_object_type(target_object):
    mc.select(target_object)
    if is_group(mc.ls(sl=1)):
        return 'group'
    else:
        current_shape = mc.listRelatives(mc.ls(sl=True), shapes=True)
        return mc.objectType(current_shape)


def is_group(node):
    if mc.objectType(node, isType = 'joint'):
        return False
    children = mc.listRelatives(node, c=1)
    if children:
        for child in children:
            if not mc.objectType(child, isType = 'transform'):
                return False
    return True


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


def get_material_attributes(target_material: str, target_keys: list):
    attribute_dictionary = {}
    attributes = mc.listAttr(target_material)
    for target_key in attributes:
        if target_key in target_keys:
            try:
                target_value = mc.getAttr(target_material + '.' + target_key)
                attribute_dictionary[target_key] = target_value
            except RuntimeError as e:
                _LOGGER.info(f'GetMaterialAttributes error: {e}')
    return attribute_dictionary


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
            'material_textures': get_material_textures(mc.nodeType(material_name), {'shading_group':
                                                                                    get_shading_group(material_name),
                                                                                    'name': material_name})
        }
        material_information[material_name] = material_listing
    return material_information


def get_shading_group(material_name):
    """! Convenience function for obtaining the shader that the specified material (as an argument) is attached to.

    @param material_name Takes the material name as an argument to get associated shader object
    @return Maya Shading group
    """
    return mc.listConnections(material_name, type='shadingEngine')[0]


def get_mesh_export_path(target_mesh, export_location):
    if os.path.isdir(export_location):
        return Path(export_location) / target_mesh / f'{target_mesh}.fbx'
    return ''


def get_mesh_materials(target_mesh):
    """! Gathers single materials attached to mesh passed as argument. This will not return additional materials
    attached to parented objects

    @param target_mesh The target mesh to pull attached material information from.
    @return list - Attached material name.
    """
    material_list = []
    mc.select(target_mesh, r=True)
    children = mc.listRelatives(ad=1)
    for child in children:
        shader_groups = mc.listConnections(mc.listHistory(child))
        if shader_groups is not None:
            for material in mc.ls(mc.listConnections(shader_groups), materials=1):
                material_list.append(material)
    return list(set(material_list))


def get_all_mesh_materials(target_mesh):
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
        mc.delete(material_name)

    try:
        sha = mc.shadingNode('StingrayPBS', asShader=True, name=material_name)
        sg = mc.sets(renderable=True, noSurfaceShader=True, empty=True)
        mc.connectAttr(sha + '.outColor', sg + '.surfaceShader', force=True)
        enable_stingray_textures(material_name)
    except Exception as e:
        _LOGGER.info('Shader creation failed: {}'.format(e))
    return [sha, sg]


def get_new_material(material_type: str, material_name: str, assignment_list: list):
    _LOGGER.info(f'Getting new material[{material_name}] of type: {material_type}  ApplyTo: {assignment_list}')


def get_material_info(target_material: str):
    mc.select(target_material, r=True)
    material_type = get_material_type(mc.ls(sl=True, long=True) or [])
    if material_type == 'aiStandardSurface':
        return aiStandardSurface.get_material_info(target_material)
    elif material_type == 'aiStandard':
        return aiStandard.get_material_info(target_material)
    elif material_type == 'StingrayPBS':
        return stingray.get_material_info(target_material)


def get_default_material_settings(material_type):
    """! This is important for helping to detect user changes to the default material attributes. It compares a
    new instance of the target material type and generates a dictionary of default values to compare with the
    target material being read/parsed in the system.
    """
    # TODO - combine this with get_material above
    default_settings = None
    if material_type in mc.listNodeTypes('shader'):
        try:
            mc.sphere(n='templateSphere')
            sg = set_new_material(material_type, 'templateShader', ['templateSphere'])
            default_settings = get_material_info('templateShader')
            mc.delete('templateSphere', 'templateShader', sg)
        except KeyError:
            _LOGGER.info(f'KeyError... Unable to retrieve default material settings.')
    return default_settings


def get_scene_objects():
    mesh_objects = mc.ls(type='mesh')
    return mesh_objects


def get_selected_objects():
    """! Filters selection to just meshes in the event that other items exist in the current selection."""
    return mc.filterExpand(sm=12)


def get_object_by_name(target_object):
    try:
        mc.select(target_object)
        object_exists = True
    except ValueError:
        object_exists = False
    return object_exists


def get_object_position(target_object):
    object_position = []
    for item in ['translateX', 'translateY', 'translateZ']:
        object_position.append(mc.getAttr(f'{target_object}.{item}'))
    return object_position


def get_transferable_properties(material_type):
    if material_type == 'aiStandardSurface':
        return aiStandardSurface.get_transferable_properties()
    elif material_type == 'aiStandard':
        return aiStandard.get_transferable_properties()
    elif material_type == 'StingrayPBS':
        return stingray.get_transferable_properties()


def set_new_material(material_type: str, material_name: str, mesh_assignment: list):
    # Create material if it doesn't already exist
    sg = None
    try:
        if not mc.objExists(material_name):
            sha = mc.shadingNode(material_type, asShader=True, name=material_name)
            sg = mc.sets(renderable=True, noSurfaceShader=True, empty=True)
            mc.connectAttr(sha + '.outColor', sg + '.surfaceShader', force=True)

        for mesh_target in mesh_assignment:
            try:
                mc.select(mesh_target, replace=True)
                mc.hyperShade(assign=material_name)
            except Exception as e:
                _LOGGER.info(f'SetNewMaterialError [type(e)]: {e}')
    except Exception as e:
        _LOGGER.info(f'Set Material failed [{type(e)}]: {e}')
    return sg


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


def export_mesh(target_object, mesh_export_location, export_options):
    _LOGGER.info(f'Export Mesh ::::: [{target_object}] --> {mesh_export_location}')
    target_directory_path = Path(mesh_export_location).parent
    if not os.path.exists(target_directory_path):
        os.makedirs(target_directory_path)

    maya.mel.eval("FBXExportSmoothingGroups -v true;")
    maya.mel.eval("FBXExportSmoothMesh -v true;")

    object_position = get_object_position(target_object)
    try:
        ##########
        if 'Preserve Transform Values' not in export_options:
            mc.move(0, 0, 0, target_object, absolute=True)

        mc.select(target_object, replace=True)

        ##########
        if 'Triangulate Exported Objects' in export_options:
            maya.mel.eval("FBXExportTriangulate -v true;")
        else:
            maya.mel.eval("FBXExportTriangulate -v false;")
        mc.file(str(mesh_export_location), force=True, type='FBX export', exportSelected=True)

        ##########
        if 'Force Z-up Export' in export_options:
            maya.mel.eval("FBXExportUpAxis z;")
        else:
            maya.mel.eval("FBXExportUpAxis y;")

        mc.move(object_position[0], object_position[1], object_position[2], target_object, absolute=True)
        return True
    except Exception as e:
        _LOGGER.info(f'ExportMesh failed [type(e)]: {e}')


def export_grouped_meshes(mesh_list, mesh_export_location, export_options):
    for mesh in mesh_list:
        mc.select(mesh, add=True)

    maya.mel.eval("FBXExportSmoothingGroups -v true;")
    maya.mel.eval("FBXExportSmoothMesh -v true;")

    ##########
    if 'Force Z-up Export' in export_options:
        maya.mel.eval("FBXExportUpAxis z;")
    else:
        maya.mel.eval("FBXExportUpAxis y;")
    
    ##########
    if 'Triangulate Exported Objects' in export_options:
        maya.mel.eval("FBXExportTriangulate -v true;")
    else:
        maya.mel.eval("FBXExportTriangulate -v false;")
    mc.file(str(mesh_export_location), force=True, type='FBX export', exportSelected=True)


def cleanup(target_objects):
    mc.select(target_objects, replace=True)
