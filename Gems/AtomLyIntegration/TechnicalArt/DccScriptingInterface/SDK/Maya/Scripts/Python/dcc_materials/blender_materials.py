# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import bpy
import sys
import json


class BlenderMaterials(object):
    def __init__(self, files_list, materials_count):
        super(BlenderMaterials, self).__init__()

        self.files_list = files_list
        self.current_scene = None
        self.materials_dictionary = {}
        self.materials_count = materials_count
        self.get_material_information()

    def get_material_information(self):
        """
        Main entry point for the material information extraction. Because this class is run
        in Standalone mode as a subprocess, the list is passed as a string- some parsing/measures
        need to be taken in order to separate values that originated as a list before passed.

        :return: A dictionary of all of the materials gathered. Sent back to main UI through stdout
        """
        for file in file_list.split(','):
            self.current_scene = file.replace('\'', '')
            print('File in BlenderMaterials to be processed: {}'.format(self.current_scene))

        #     pm.openFile(self.current_scene, force=True)
        #     self.set_material_descriptions()
        # json.dump(self.materials_dictionary, sys.stdout)

    @staticmethod
    def get_materials(target_mesh):
        """
        Gathers a list of all materials attached to each mesh's shader

        :param target_mesh: The target mesh to pull attached material information from.
        :return: List of unique material values attached to the mesh passed as an argument.
        """
        pass

    @staticmethod
    def get_shader(material_name):
        """
        Convenience function for obtaining the shader that the specified material (as an argument)
        is attached to.

        :param material_name: Takes the material name as an argument to get associated shader object
        :return:
        """
        pass

    @staticmethod
    def get_shader_information(shader):
        """
        Helper function for extracting shader/material attributes used to form the DCC specific dictionary
        of found material values for conversion.

        :param shader: The target shader object to analyze
        :return: Complete set (in the form of two dictionaries) of file connections and material attribute values
        """
        pass

    def set_material_dictionary(self, material_name, material_type, material_mesh):
        """
        When a unique material has been found, this creates a dictionary entry with all relevant material values. This
        includes material attributes as well as attached file textures. Later in the process this information is
        leveraged when creating the Lumberyard material definition.

        :param material_name: The name attached to the material
        :param material_type: Specific type of material (Arnold, Stingray, etc.)
        :param material_mesh: Mesh that the material is applied to
        :return:
        """
        self.materials_count += 1
        pass
        # shader = self.get_shader(material_name)
        # shader_file_connections, shader_attributes = self.get_shader_information(shader)
        # material_dictionary = {'MaterialName': material_name, 'MaterialType': material_type, 'DccApplication': 'Maya',
        #                        'AppliedMesh': material_mesh, 'FileConnections': shader_file_connections,
        #                        'SceneName': str(self.current_scene), 'MaterialAttributes': shader_attributes}
        # material_name = 'Material_{}'.format(self.materials_count)
        # self.materials_dictionary[material_name] = material_dictionary

    def set_material_descriptions(self):
        """
        This function serves as the clearinghouse for all analyzed materials passing through the system.
        It will determine whether or not the found material has already been processed, or if it needs to
        be added to the final material dictionary. In the event that an encountered material has already
        been processed, this function creates a register of all meshes it is applied to in the 'AppliedMesh'
        attribute.
        :return:
        """
        pass
        # scene_geo = pm.ls(v=True, geometry=True)
        # for target_mesh in scene_geo:
        #     material_list = self.get_materials(target_mesh)
        #     for material_name in material_list:
        #         material_type = pm.nodeType(material_name, api=True)
        #         material_listed = [x for x in self.materials_dictionary
        #                            if self.materials_dictionary[x]['MaterialName'] == material_name]
        #
        #         if not material_listed:
        #             self.set_material_dictionary(str(material_name), str(material_type), str(target_mesh))
        #         else:
        #             mesh_list = self.materials_dictionary[material_name].get('AppliedMesh')
        #             if not isinstance(mesh_list, list):
        #                 self.materials_dictionary[material_name]['AppliedMesh'] = [mesh_list, target_mesh]
        #             else:
        #                 mesh_list.append(target_mesh)

# def get_material_information():
#     material_attributes = {}
#     for target_object in bpy.data.objects:
#         if target_object.type == 'MESH':
#             print('Object: {}    Material: {}'.format(target_object, target_object.active_material))
#             assigned_material = target_object.active_material
#             material_nodes = assigned_material.node_tree.nodes
#
#             for node in material_nodes:
#                 for material_input in node.inputs:
#                     try:
#                         attribute_name = material_input.name
#                         attribute_value = node.inputs[attribute_name].default_value
#                         material_attributes[attribute_name] = str(attribute_value)
#                     except Exception as e:
#                         print ('[{}] Exception encountered: {}'.format(material_input.name, e))
#
#     print(json.dumps(material_attributes, sort_keys=True, indent=4))




# get_material_information()


# ++++++++++++++++++++++++++++++++++++++++++++++++#
# Maya Specific Shader Mapping #
# ++++++++++++++++++++++++++++++++++++++++++++++++#

if __name__ == '__main__':
    print(len(sys.argv))
    print(sys.argv[5])
    # arg_values = sys.argv[5].split(',')
    # scene_information = []
    # for value in arg_values:
    #     print(value)
    #     scene_information.append(value)
    # initialize_scene(scene_information)
# instance = BlenderMaterials(file_list, total_material_count)
