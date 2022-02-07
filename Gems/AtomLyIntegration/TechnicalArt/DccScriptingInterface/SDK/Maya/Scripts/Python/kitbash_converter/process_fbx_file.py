# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# File Description:
# This file is designed to run in Maya Standalone- it imports existing FBX files and reformats materials as Stingray
# PBS materials, and attempts to attach PBR Metal/Rough texture sets generated for each asset. There is currently a
# bug in Maya that prevents this from being possible, although it has been officially logged with Autodesk so hopefully
# a fix will exist in an updated version
# -------------------------------------------------------------------------


#  maya imports
from PySide2 import QtCore
import maya.cmds as mc
import maya.standalone
maya.standalone.initialize(name='python')
mc.loadPlugin("fbxmaya")
import maya.mel as mel
mel.eval('loadPlugin fbxmaya')

#  built-ins
import logging as _logging
import collections
import json
import sys
import os

module_name = 'kitbash_converter.process_fbx_file'
_LOGGER = _logging.getLogger(module_name)


SUPPORTED_MATERIAL_PROPERTIES = ['baseColor', 'emissive', 'metallic', 'roughness', 'normal', 'opacity']


class ProcessFbxFile(QtCore.QObject):
    def __init__(self, fbx_file, base_directory, relative_destination_path, parent=None):
        super(ProcessFbxFile, self).__init__(parent)
        self.fbx_file = fbx_file
        self.base_directory = base_directory
        self.textures_directory = self.get_textures_directory()
        self.relative_destination_path = relative_destination_path
        self.default_material_definition = 'standardPBR.template.material'
        self.transfer_data = {}
        self.process_file()

    def process_file(self):
        """
        This makes a copy of the Standard PBR material for use as a template when processing a Kitbash3d
        asset. Then it imports the FBX file into Maya and begins the separation and material conversion
        process.
        :return:
        """
        _LOGGER.info('MAYA STANDALONE PROCESS STARTED ---------------------------------------------')
        _LOGGER.info('FBX File Passed: {}'.format(self.fbx_file))
        _LOGGER.info('Base Directory: {}'.format(self.base_directory))
        _LOGGER.info('Destination Directory: {}'.format(self.relative_destination_path))
        _LOGGER.info('-----------------------------------------------------------------------------\n')

        if self.textures_directory and os.path.exists(self.fbx_file):
            # Get Material Definition Template
            with open(self.default_material_definition) as json_file:
                self.default_material_definition = json.load(json_file)

            mc.file(self.fbx_file, i=True, type="FBX")
            return_dictionary = self.process_groups()
            _LOGGER.info('ReturnDictionary: {}'.format(return_dictionary))
            json.dump(return_dictionary, sys.stdout)
            _LOGGER.info('Process Complete.')

    def process_groups(self):
        """
        This is the main function of the module and orchestrates the complete set of steps for extracting
        each subobject from the asset FBX file and generating the .material files for use in Lumberyard.
        :return:
        """
        asset_dictionary = {}
        ignore_list = ['persp', 'top', 'front', 'side']
        group_list = []
        for item in mc.ls(assemblies=True):
            if item not in ignore_list:
                group_list.append(item)

        for grp in group_list:
            _LOGGER.info('\n_\n////////////////////////////////\n--> {}\n////////////////////////////////'.format(grp))
            asset_material_dictionary = self.get_asset_materials(grp)
            material_dictionary = []
            for material_name, texture_set in asset_material_dictionary.items():
                _LOGGER.info('+++++ MaterialName: {}   TextureSet: {}'.format(material_name, texture_set))
                fbx_parts_list = grp.split('_')[1:-1]
                new_fbx_name = ('_').join(fbx_parts_list)
                _LOGGER.info('NewFBXName: {}'.format(new_fbx_name))
                material_definition_name = '{}_{}.material'.format(new_fbx_name, material_name)
                _LOGGER.info('Material File: {}'.format(material_definition_name))

                # Export FBX
                output_directory = os.path.join(base_directory, self.relative_destination_path, new_fbx_name)
                _LOGGER.info('FBX Output Directory: {}'.format(output_directory))
                if not os.path.exists(output_directory):
                    os.makedirs(output_directory)
                self.export_fbx(grp, os.path.join(output_directory, '{}.fbx'.format(new_fbx_name)))
                material_definition = self.create_object_material(output_directory, material_definition_name,
                                                                  texture_set, material_name)
                asset_dictionary[new_fbx_name] = {'asset_location': output_directory}
        return asset_dictionary

    def find_textures(self, material_name):
        """
        This is run if there were no files attached to materials. There appears to be material sets for all materials,
        and in the case of glass textures they just are not assigned.
        :param material_name: The material name inside of the FBX- if there are textures they will begin with this.
        """
        _LOGGER.info('Searching for textures for the following material: {}'.format(material_name))
        texture_set = {}
        for file in os.listdir(self.textures_directory):
            if file.startswith(material_name):
                try:
                    texture_type = self.get_texture_type(file)
                    texture_set[texture_type] = os.path.join(self.textures_directory, file)
                except Exception as e:
                    _LOGGER.info('Could not create texture record for file [{}]... Exception: {}'.format(file, e))
        return texture_set

    def create_object_material(self, output_directory, material_definition_name, texture_dictionary, material_name):
        """
        Using the file textures present, this function takes materials and associated texture sets
        and forms the .material file based on those values for each material/fbx found
        :param output_directory: The destination directory for the exported assets
        :param material_definition_name: The name for the exported .material file
        :param texture_dictionary: All the file textures associated with a material
        :return:
        """
        output_path = os.path.join(output_directory, material_definition_name)
        _LOGGER.info('Creating Material: {}   OutputPath: {}'.format(material_definition_name, output_path))

        if not os.path.exists(output_path):
            material_definition = collections.OrderedDict()
            material_template = self.default_material_definition.copy()
            try:
                for key, values in material_template.items():
                    # These values remain constant
                    if key != 'properties':
                        material_definition[key] = values
                    else:
                        modified_items = {}
                        for material_property in SUPPORTED_MATERIAL_PROPERTIES:
                            for k, v in values.items():
                                if k.lower() in texture_dictionary.keys():
                                    temp_dict = self.get_texture_attributes(k.lower(), texture_dictionary, material_name)
                                    modified_items[k] = temp_dict

                        if modified_items:
                            material_definition[key] = modified_items

                if len(material_definition) > 4:
                    _LOGGER.info('\n++++++++++++++\n+++++++++++++++\nFinal Material: {}\n++++++++++++++\n'
                                 '+++++++++++++++\n_\n'.format(json.dumps(material_definition, indent=4, sort_keys=False)))
                    self.export_lumberyard_material(output_path, material_definition)
            except Exception as e:
                _LOGGER.info('Problem creating Material Definition: {}'.format(e))

    def remediate_empty_material(self, material_name):
        _LOGGER.info('Resolving empty material::::::: {}'.format(material_name))

    ##############################
    # Getters/Setters ############
    ##############################

    def get_asset_materials(self, target_group):
        """
        Pulls information about assigned materials from materials in order to coordinate file textures present
        for each material being set up for Lumberyard.
        :param target_group: Kitbash 3d assets contain several subobjects grouped under locators. "Target group"
        signifies a group from the total set of groups present.
        :return:
        """
        _LOGGER.info('Getting Asset Materials... Target Group: {}'.format(target_group))
        materials_dictionary = {}
        mc.select(target_group, hierarchy=True)

        target_nodes = mc.ls(sl=True, dag=True, s=True)
        sg = mc.listConnections(target_nodes, type= 'shadingEngine')
        materials = list(set(mc.ls(mc.listConnections(sg), materials=True)))

        for material_name in materials:
            found_textures = []
            texture_files = [x for x in mc.listConnections(material_name, plugs=1, source=1) if x.endswith('outColor')]
            for file_name in texture_files:
                try:
                    found_textures.append(mc.getAttr('{}.fileTextureName'.format(file_name.split('.')[0])))
                except Exception as e:
                    _LOGGER.info('Error occured: {}'.format(e))

            texture_set = self.get_texture_set(found_textures) if found_textures else self.find_textures(material_name)
            if texture_set:
                materials_dictionary[material_name] = texture_set
            else:
                attribute_list = self.remediate_empty_material(material_name)
                if attribute_list:
                    materials_dictionary[material_name] = attribute_list
        return materials_dictionary

    def get_textures_directory(self):
        for (root, dirs, files) in os.walk(self.base_directory, topdown=True):
            for dir in dirs:
                if dir.lower() == 'textures':
                    return os.path.join(root, dir)
        return None

    def get_texture_set(self, texture_files):
        """
        Gets the entire set of textures associated with a material using a single texture.
        :param texture_files:
        :return:
        """
        texture_set = {}
        for texture_file in texture_files:
            base_texture_name = self.get_base_texture_name(texture_file)
            for file in os.listdir(self.textures_directory):
                if file.startswith(base_texture_name):
                    try:
                        texture_type = self.get_texture_type(file)
                        texture_set[texture_type] = os.path.join(self.textures_directory, file)
                        _LOGGER.info('{} texture added. Path::: {}'.format(texture_type, os.path.join(self.textures_directory, file)))
                    except Exception:
                        pass
        return texture_set

    def get_texture_type(self, file):
        """
        Tries to extract the texture type from a texture filename (ie "basecolor", "roughness")

        :param file: The file name to get the texture type from
        """
        file_stem = file.split('.')[0]
        file_parts = file_stem.split('_')
        if len(file_parts) > 1:
            return file_parts[-1]

    def get_texture_attributes(self, texture_type, texture_dictionary, material_name):
        target_path = os.path.normpath(texture_dictionary[texture_type])
        if texture_type == 'opacity':
            temp_dict = self.get_opacity_settings(material_name, target_path)
        elif texture_type == 'subsurfacescattering':
            temp_dict = self.get_sss_settings(material_name, target_path)
        elif texture_type == 'emissive':
            temp_dict = self.get_emissive_settings(material_name, target_path)
        elif texture_type == 'basecolor':
            temp_dict = self.get_basecolor_settings(material_name, target_path)
        else:
            temp_dict = {'textureMap': self.get_relative_path(target_path)}

        return temp_dict

    def get_opacity_settings(self, material_name, target_path):
        """
        Reads Maya material to construct material opacity attributes

        :param material_name: The Maya Material name being translated for Lumberyard
        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting opacity settings...')
        return {'alphaSource': 'Split', 'mode': 'Cutout', 'textureMap': self.get_relative_path(target_path)}

    def get_sss_settings(self, material_name, target_path):
        """
        Reads Maya material to construct material subsurface scattering attributes

        :param material_name: The Maya material name being translated for Lumberyard
        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting Subsurface Scattering settings...')
        return {'enableSubsurfaceScattering': True, 'influenceMap': self.get_relative_path(target_path)}

    def get_emissive_settings(self, material_name, target_path):
        """
        Reads Maya material to construct material emissive attributes

        :param material_name: The Maya material name being translated for Lumberyard
        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting Emissive settings...')
        return {'enable': True, 'intensity': 4, 'textureMap': self.get_relative_path(target_path)}

    def get_basecolor_settings(self, material_name, target_path):
        """
        Reads Maya material to construct material BaseColor settings

        :param material_name: The Maya material name being translated for Lumberyard
        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting BaseColor settings...')
        # Get shader color values
        return {'textureMap': self.get_relative_path(target_path)}

    def get_base_texture_name(self, texture_path):
        """
        Tries to extract texture name without the added texture type (ie "basecolor", "roughness", etc)

        :param texture_path: The path to the texture from which to extract the base texture name
        """
        path_list = texture_path.split('\\')
        file_name = path_list[-1]
        texture_list = file_name.split('_')
        base_texture_name = ('_').join(texture_list[:-1])
        return base_texture_name

    def get_relative_path(self, full_path):
        """
        Material definitions use relative paths for file textures- this function takes the full paths of assets and
        converts to the abbreviated relative path format needed for Lumberyard to source the files

        :param full_path: Full path to the asset
        :return:
        """
        truncated_path = self.textures_directory.split('AtomContent\\')[-1]
        start_directory = truncated_path.split('\\')[0]
        path_list = full_path.split('\\')
        return_path = ('/').join(path_list[path_list.index(start_directory):])
        return return_path

    ##############################
    # Export Files ###############
    ##############################

    def export_fbx(self, group_name, output_path):
        """
        Exports sub-objects from Kitbash3d assets, groups of assets are contained in a single FBX for delivery.

        :param group_name: Group name inside of FBX- subobjects are grouped under locators
        :param output_path: The destination path for the exported FBX object
        :return:
        """
        if not os.path.exists(output_path):
            mc.move(0, 0, 0, group_name, absolute=True)
            mc.select(group_name, hierarchy=True)
            mc.FBXExport('-file', output_path, '-s')
            mc.select(clear=True)

    def export_lumberyard_material(self, output_path, material_description):
        """
        Takes one final dictionary with information gathered in the process and saves with JSON formatting into a
        .material file

        :param output:
        :param material_description:
        :return:
        """
        _LOGGER.info('Output .material++++++>> {}'.format(output_path))
        with open(output_path, 'w') as material_file:
            json.dump(dict(material_description), material_file, ensure_ascii=False, indent=4)


fbx_file = sys.argv[1]
_LOGGER.info('FBX file: {}'.format(fbx_file))
base_directory = sys.argv[-2]
_LOGGER.info('Base Directory: {}'.format(base_directory))
relative_destination_path = sys.argv[-1].replace('/', '\\')
_LOGGER.info('Relative Destination Path: {}'.format(relative_destination_path))
ProcessFbxFile(fbx_file, base_directory, relative_destination_path)
