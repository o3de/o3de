# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import bpy
import collections
import json


def get_shader_information():
    """
    Queries all materials and corresponding material attributes and file textures in the Blender scene.

    :return:
    """
    # TODO - link file texture location to PBR material plugs- finding it difficult to track down how this is achieved
    #  in the Blender Python API documentation and/or in forums

    materials_count = 1
    shader_types = get_blender_shader_types()
    materials_dictionary = {}
    for target_mesh in [o for o in bpy.data.objects if type(o.data) is bpy.types.Mesh]:
        material_information = collections.OrderedDict(DccApplication='Blender', AppliedMesh=target_mesh,
                                                       SceneName=bpy.data.filepath, MaterialAttributes={},
                                                       FileConnections={})
        for target_material in target_mesh.data.materials:
            material_information['MaterialName'] = target_material.name
            shader_attributes = {}
            shader_file_connections = {}

            for node in target_material.node_tree.nodes:
                socket = node.inputs[0]
                print('NODE: {}'.format(node))
                print('Socket: {}'.format(socket))
                
                for material_input in node.inputs:
                    attribute_name = material_input.name
                    try:
                        attribute_value = material_input.default_value
                        print('Name: [{}] [{}]  ValueType ::::::> {}'.format(attribute_name, attribute_value,
                                                                             type(attribute_value)))
                        material_information['MaterialAttributes'].update({attribute_name: str(attribute_value)})
                    except Exception as e:
                        pass
                print('\n')
                if node.type == 'TEX_IMAGE':
                    material_information['FileConnections'].update({str(node): str(node.image.filepath)})
                if node.name in shader_types.keys():
                    material_information['MaterialType'] = shader_types[node.name]

            
#            material_information['MaterialAttributes'] = shader_attributes              
            materials_dictionary['Material_{}'.format(materials_count)] = material_information
            materials_count += 1
            print('_________________________________________________________________\n')
            
    return materials_dictionary


def get_blender_shader_types():
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
        
    
materials_dictionary = get_shader_information()
#print('Materials Dictionary:')
#print(materials_dictionary)
#parsed = json.loads(str(materials_dictionary))
#print(json.dumps(parsed, indent=4, sort_keys=True))


