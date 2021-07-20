# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------

import logging

logging.basicConfig(level=logging.DEBUG)

def get_maya_material_mapping(name, material_type, file_connections):
    """
    Helps map found material DCC attribute values/file connections with Lumberyard materials.

    :param name: Material name from within Maya
    :param material_type: Maya Material type to match values to (i.e. Stingray PBS, aiStandardSurface(Arnold)
    :param file_connections: List of all connected texture files from Maya
    :return: Key value pairs for attributes/file textures assigned as Lumberyard material values
    """
    material_properties = {}
    if material_type == 'StingrayPBS':
        logging.debug('Mapping StingrayPBS')
        maps = 'color, metallic, roughness, normal, emissive, ao, opacity'.split(', ')
        naming_exceptions = {'color': 'baseColor', 'ao': 'ambientOcclusion'}
        for m in maps:
            texture_attribute = 'TEX_{}_map'.format(m)
            for tex in file_connections.keys():
                if tex.find(texture_attribute) != -1:
                    key = m if m not in naming_exceptions else naming_exceptions.get(m)
                    logging.debug('Key, Value: {}   {}.{}'.format(key, name, texture_attribute))
                    material_properties[key] = {'useTexture': 'true',
                                                'textureMap': file_connections.get(
                                                    '{}.{}'.format(name, texture_attribute))}
    elif material_type == 'aiStandardSurface':
        logging.debug('Mapping AiStandardSurface')
        # TODO- Occlusion is based on a more difficult setup- there is no standard channel. Set this up as time permits
        maps = 'baseColor, metalness, specularRoughness, normal, emissionColor, opacity'.split(', ')
        naming_exceptions = {'metalness': 'metallic', 'specularRoughness': 'roughness', 'emissionColor': 'emissive'}
        for m in maps:
            key = m if m not in naming_exceptions.keys() else naming_exceptions.get(m)
            texture_attribute = m
            for tex in file_connections.keys():
                if tex.find(texture_attribute) != -1:
                    logging.debug('Key, Value: {}    {}.{}'.format(key, name, texture_attribute))
                    material_properties[key] = {'useTexture': 'true',
                                                'textureMap': file_connections.get(
                                                    '{}.{}'.format(name, texture_attribute))}
    else:
        pass

    return material_properties


def get_blender_material_mapping(name, material_type, file_connections):
    pass


def get_max_material_mapping(name, material_type, file_connections):
    pass

