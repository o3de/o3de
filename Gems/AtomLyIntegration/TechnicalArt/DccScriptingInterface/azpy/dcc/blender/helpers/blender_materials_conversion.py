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
# standard imports
from pathlib import Path
import logging as _logging
import bpy
from DccScriptingInterface.azpy.dcc.blender.helpers import convert_bsdf_material as bsdf

_LOGGER = _logging.getLogger('Dccsi.azpy.dcc.blender.helpers.blender_materials')


supported_material_types = [
    'Principled BSDF'
]


def prepare_material_export(target_mesh, export_location):
    mesh_materials = get_mesh_materials(target_mesh)
    export_path = get_mesh_export_path(target_mesh, export_location) if export_location else ''
    return export_path, mesh_materials


def get_current_scene():
    return bpy.data.filepath


def get_selected_objects():
    return [x.name for x in bpy.context.selected_objects]


def get_mesh_materials(target_object):
    return [bpy.data.objects[target_object].active_material]


def get_material_info(target_material):
    material_type = get_material_type(target_material)
    if material_type == 'Principled BSDF':
        return bsdf.get_material_info(target_material)


def get_material_type(target_material):
    shader_types = get_shader_types()
    for node in target_material.node_tree.nodes:
        if node.name in shader_types:
            return node.name


def get_mesh_export_path(target_mesh, export_location):
    return Path(export_location) / f'{target_mesh}.fbx'


def get_shader_types():
    """
    This returns all the material types present in the Blender scene
    :return:
    """
    shader_types = {}
    ddir = lambda data, filter_str: [i for i in dir(data) if i.startswith(filter_str)]
    get_nodes = lambda cat: [i for i in getattr(bpy.types, cat).category.items(None)]
    cycles_categories = ddir(bpy.types, "NODE_MT_category_SH_NEW")
    for cat in cycles_categories:
        if cat == 'NODE_MT_category_SH_NEW_SHADER':
            for node in get_nodes(cat):
                shader_types[node.label] = node.nodetype
            return shader_types
