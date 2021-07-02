# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

from PySide2 import QtCore
import maya.standalone
maya.standalone.initialize(name='python')
import pymel.core as pm
import json
import sys
import logging


logging.basicConfig(level=logging.WARNING)


class MayaMaterials(QtCore.QObject):
    def __init__(self, files_list, materials_count, parent=None):
        super(MayaMaterials, self).__init__(parent)

        self.files_list = files_list
        self.current_scene = None
        self.materials_dictionary = {}
        self.materials_count = int(materials_count)
        self.get_material_information()

    def get_material_information(self):
        """
        Main entry point for the material information extraction. Because this class is run
        in Standalone mode as a subprocess, the list is passed as a string- some parsing/measures
        need to be taken in order to separate values that originated as a list before passed.

        :return: A dictionary of all of the materials gathered. Sent back to main UI through stdout
        """
        for file in file_list:
            self.current_scene = file.replace('\'', '')
            pm.openFile(self.current_scene, force=True)
            self.set_material_descriptions()
        json.dump(self.materials_dictionary, sys.stdout)

    @staticmethod
    def get_materials(target_mesh):
        """
        Gathers a list of all materials attached to each mesh's shader

        :param target_mesh: The target mesh to pull attached material information from.
        :return: List of unique material values attached to the mesh passed as an argument.
        """
        shading_group = pm.listConnections(pm.PyNode(target_mesh), type='shadingEngine')
        materials = pm.ls(pm.listConnections(shading_group), materials=1)
        return list(set(materials))

    @staticmethod
    def get_shader(material_name):
        """
        Convenience function for obtaining the shader that the specified material (as an argument)
        is attached to.

        :param material_name: Takes the material name as an argument to get associated shader object
        :return:
        """
        connections = pm.listConnections(material_name, type='shadingEngine')[0]
        shader_name = '{}.surfaceShader'.format(connections)
        shader = pm.listConnections(shader_name)[0]
        return shader

    @staticmethod
    def get_shader_information(shader, material_mesh):
        """
        Helper function for extracting shader/material attributes used to form the DCC specific dictionary
        of found material values for conversion.

        :param shader: The target shader object to analyze
        :param material_mesh: The material mesh needs to be passed to search for textures attached to it.
        :return: Complete set (in the form of two dictionaries) of file connections and material attribute values
        """
        shader_file_connections = {}
        shading_group = pm.listConnections(material_mesh, type='shadingEngine')
        material = pm.ls(pm.listConnections(shading_group), materials=1)[0]
        used_texture_nodes = set(pm.listHistory(material, type='file'))
        for node in used_texture_nodes:
            for attr in pm.listConnections(node, plugs=1):
                try:
                    if attr.split('.')[0] == material:
                        shader_file_connections[str(attr)] = pm.getAttr(node.fileTextureName)
                        break
                except pm.MayaAttributeError:
                    pass

        shader_attributes = {}
        for shader_attribute in pm.listAttr(shader, s=True, iu=True):
            try:
                shader_attributes[str(shader_attribute)] = str(pm.getAttr('{}.{}'.format(shader, shader_attribute)))
            except pm.MayaAttributeError as e:
                logging.error('MayaAttributeError: {}'.format(e))

        return shader_file_connections, shader_attributes

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
        shader = self.get_shader(material_name)
        shader_file_connections, shader_attributes = self.get_shader_information(shader, material_mesh)
        material_dictionary = {'MaterialName': material_name, 'MaterialType': material_type, 'DccApplication': 'Maya',
                               'AppliedMesh': material_mesh, 'FileConnections': shader_file_connections,
                               'SceneName': str(self.current_scene), 'MaterialAttributes': shader_attributes}
        material_name = 'Material_{}'.format(self.materials_count)
        self.materials_dictionary[material_name] = material_dictionary

    def set_material_descriptions(self):
        """
        This function serves as the clearinghouse for all analyzed materials passing through the system.
        It will determine whether or not the found material has already been processed, or if it needs to
        be added to the final material dictionary. In the event that an encountered material has already
        been processed, this function creates a register of all meshes it is applied to in the 'AppliedMesh'
        attribute.
        :return:
        """
        scene_geo = pm.ls(v=True, geometry=True)
        for target_mesh in scene_geo:
            material_list = self.get_materials(target_mesh)
            for material_name in material_list:
                material_type = pm.nodeType(material_name, api=True)
                material_listed = [x for x in self.materials_dictionary
                                   if self.materials_dictionary[x]['MaterialName'] == material_name]

                if not material_listed:
                    self.set_material_dictionary(str(material_name), str(material_type), str(target_mesh))
                else:
                    mesh_list = self.materials_dictionary[material_name].get('AppliedMesh')
                    if not isinstance(mesh_list, list):
                        self.materials_dictionary[material_name]['AppliedMesh'] = [mesh_list, target_mesh]
                    else:
                        mesh_list.append(target_mesh)


# ++++++++++++++++++++++++++++++++++++++++++++++++#
# Maya Specific Shader Mapping #
# ++++++++++++++++++++++++++++++++++++++++++++++++#

file_list = sys.argv[1:-1]
count = sys.argv[-1]
instance = MayaMaterials(file_list, count)

