#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! @brief This module assists with material operations involving Blender Principled BSDF materials. """

##
# @file convert_bsdf_material.py
#
# @brief This module contains several common materials utilities/operations for extracting/converting material
# information contained in Blender Principled BSDF shaders. This includes gathering file texture information,
# and input/output connections, and other material property settings relevant for materials conversion to O3DE
#
# @section BSDF Material Notes
# - Comments are Doxygen compatible


import bpy


bsdf_file_connections = [
    'Base Color',
    'Metallic',
    'Roughness',
    'Emission',
    'Normal',
    'Subsurface',
    'Alpha',
    'Displacement'
]


# Add command to avoid default Blender relative paths
bpy.ops.file.make_paths_absolute()


def get_material_info(target_material: str) -> dict:
    material_textures = get_material_textures(target_material)
    return get_material_settings(material_textures)


def get_material_textures(target_material: str) -> dict:
    material_textures_dict = {}
    for node in target_material.node_tree.nodes:
        for material_input in node.inputs:
            if material_input.name in bsdf_file_connections:
                try:
                    link = material_input.links[0]
                    file_location = link.from_node.image.filepath
                    material_textures_dict[material_input.name] = file_location
                except IndexError:
                    pass
    return material_textures_dict


def get_material_settings(material_textures):
    # TODO- add logic for additional settings values like tiling, rotation, etc.
    return material_textures
