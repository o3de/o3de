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
"""! @brief This module assists with material operations involving Maya aiStandardSurface (Arnold) materials. """

##
# @file convert_lambert_material.py
#
# @brief This module contains several common materials utilities/operations for extracting/converting material
# information contained in Maya aiStandardSurface. This includes gathering file texture information, and input/output
# connections, and other material property settings relevant for materials conversion to O3DE
#
# @section Maya Materials Notes
# - Comments are Doxygen compatible


import logging as _logging
import maya.cmds as mc

_LOGGER = _logging.getLogger('azpy.dcc.maya.helpers.convert_lambert_material')


def get_material_info(target_material: str):
    material_textures = get_material_textures(target_material)
    return get_material_settings(material_textures)


def get_material_textures(target_material: str):
    lambert_file_connections = {
        'color':    None,
        'normal':   None
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
        lambert_file_connections[texture_slot] = {'node': node, 'path': texture_path}

    return lambert_file_connections


def get_material_settings(material_settings):
    # Find all material settings and insert here (can be found by getting file node connections)
    return material_settings


def get_texture_slot(node_connections):
    supported_connections = [
        'color',
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


