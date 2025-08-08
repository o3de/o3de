# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# -------------------------------------------------------------------------
import logging as _logging
import collections
import json
import sys
import os

from PySide2 import QtCore
import maya.cmds as mc
import maya.standalone
import maya.mel as mel
maya.standalone.initialize(name='python')
mc.loadPlugin("fbxmaya")
mel.eval('loadPlugin fbxmaya')


_LOGGER = _logging.getLogger('kitbash_converter.process_fbx_file')


SUPPORTED_MATERIAL_PROPERTIES = [
    'baseColor',
    'emissive',
    'metallic',
    'normal',
    'opacity',
    'parallax',
    'roughness'
]


class ProcessFbxFile(QtCore.QObject):
    def __init__(self, target_file, import_directory_base, export_directory_base, all_properties_definition):
        super(ProcessFbxFile, self).__init__()
        self.target_file = target_file
        self.import_directory_base = import_directory_base
        self.export_directory_base = export_directory_base
        self.textures_directory = self.get_textures_directory()
        self.default_material_definition = self.get_material_template(all_properties_definition)
        self.supported_material_properties = self.get_supported_properties()
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
        _LOGGER.info('Target File Passed: {}'.format(self.target_file))
        _LOGGER.info('Import Directory Base: {}'.format(self.import_directory_base))
        _LOGGER.info('Export Directory Base: {}'.format(self.export_directory_base))
        _LOGGER.info('Textures Directory: {}'.format(self.textures_directory))
        _LOGGER.info('-----------------------------------------------------------------------------\n')

        if self.textures_directory and os.path.exists(self.target_file):
            file_name = os.path.basename(self.target_file)
            file_type = file_name.split('.')[-1]

            if file_type.lower() == 'fbx':
                mc.file(self.target_file, i=True, type="FBX")
            else:
                mc.file(self.target_file, o=True, force=True)

            return_dictionary = self.process_groups()
            json.dump(return_dictionary, sys.stdout)

    def process_groups(self):
        """
        This is the main function of the module and orchestrates the complete set of steps for extracting
        each subobject from the asset FBX file and generating the .material files for use in O3DE.
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
            for material_name, texture_set in asset_material_dictionary.items():
                fbx_parts_list = grp.split('_')[1:-1]
                new_fbx_name = '_'.join(fbx_parts_list)
                output_directory = os.path.join(self.export_directory_base, 'Objects', new_fbx_name)
                output_directory = output_directory.replace('\\', '/')
                _LOGGER.info(f'Output Directory: {output_directory}')
                os.makedirs(output_directory, exist_ok=True)

                # Export Material
                material_definition_name = '{}_{}.material'.format(new_fbx_name, material_name)
                self.create_object_material(output_directory, material_definition_name, texture_set)
                _LOGGER.info('+++++ MaterialName: {}   TextureSet: {}'.format(material_definition_name, texture_set))
                # Export FBX
                self.export_fbx(grp, os.path.join(output_directory, '{}.fbx'.format(new_fbx_name)))
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

    def create_object_material(self, output_directory, material_definition_name, texture_dictionary):
        """
        Using the file textures present, this function takes materials and associated texture sets
        and forms the .material file based on those values for each material/fbx found
        :param output_directory: The destination directory for the exported assets
        :param material_definition_name: The name for the exported .material file
        :param texture_dictionary: All the file textures associated with a material
        :return:
        """
        output_path = os.path.join(output_directory, material_definition_name)
        output_path = output_path.replace('\\', '/')
        if not os.path.exists(output_path):
            material_definition = collections.OrderedDict()
            material_template = self.default_material_definition.copy()
            try:
                for key, values in material_template.items():
                    if key != 'propertyValues':
                        material_definition[key] = values
                    else:
                        material_definition[key] = {}
                        for k, v in texture_dictionary.items():
                            texture_property = self.get_texture_property(k)
                            if texture_property in self.supported_material_properties:
                                material_definition[key]['{}.textureMap'.format(texture_property)] = \
                                    self.get_relative_path(v)

                        if material_definition[key].keys():
                            self.export_o3de_material(output_path, material_definition)
            except Exception as e:
                _LOGGER.info('Problem creating Material Definition: {}'.format(e))

    ##############################
    # Getters/Setters ############
    ##############################

    def get_asset_materials(self, target_group):
        """
        Pulls information about assigned materials from materials in order to coordinate file textures present
        for each material being set up for O3DE.
        :param target_group: Kitbash 3d assets contain several subobjects grouped under locators. "Target group"
        signifies a group from the total set of groups present.
        :return:
        """
        _LOGGER.info('Getting Asset Materials... Target Group: {}'.format(target_group))
        materials_dictionary = {}
        mc.select(target_group, hierarchy=True)

        target_nodes = mc.ls(sl=True, dag=True, s=True)
        sg = mc.listConnections(target_nodes, type='shadingEngine')
        materials = list(set(mc.ls(mc.listConnections(sg), materials=True)))

        for material_name in materials:
            found_textures = []
            texture_files = [x for x in mc.listConnections(material_name, plugs=1, source=1) if x.endswith('outColor')]
            for file_name in texture_files:
                try:
                    found_textures.append(mc.getAttr('{}.fileTextureName'.format(file_name.split('.')[0])))
                except Exception as e:
                    _LOGGER.info('GetAssetMaterials... Error occurred: {}'.format(e))

            texture_set = self.get_texture_set(found_textures) if found_textures else self.find_textures(material_name)
            if texture_set:
                materials_dictionary[material_name] = texture_set
        return materials_dictionary

    def get_supported_properties(self):
        template_keys = [k.split('.')[0] for k in self.default_material_definition['propertyValues'].keys()]
        return list(set(template_keys))

    def get_material_template(self, template_path):
        with open(template_path) as json_file:
            return json.load(json_file)

    def get_texture_property(self, suffix):
        """
        This can help correct for changes in file naming conventions or differences between the naming between O3DE
        material definition properties and KB3D file textures
        :param suffix: The "texture type" extracted from KB3D file texture name
        :return:
        """
        if suffix == 'height':
            suffix = 'parallax'
        elif suffix == 'basecolor':
            suffix = 'baseColor'
        return suffix

    def get_textures_directory(self):
        for (root, dirs, files) in os.walk(self.export_directory_base, topdown=True):
            for dir in dirs:
                if 'textures' in dir.lower():
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
                        texture_path = os.path.join(self.textures_directory, file)
                        texture_set[texture_type] = texture_path.replace('\\', '/')
                    except Exception as e:
                        _LOGGER.info(f'GetTextureSetFailed [{type(e)}]: {e}')
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

    def get_texture_attributes(self, texture_type, texture_dictionary):
        """
        Some textures have additional properties that need to be included in order for the texture to display properly
        as an O3DE material. This allows an entry point for adding these additional properties. This is not being
        called in the current state of the script, but I believe it will be needed once supplemental texture attributes
        surface as being needed (which was definitely the case with material version "3"
        :param texture_type: The texture attribute that the texture map corresponds to
        :param texture_dictionary: The texture listing
        :return:
        """
        target_path = os.path.normpath(texture_dictionary[texture_type])
        if texture_type == 'opacity':
            temp_dict = self.get_opacity_settings(target_path)
        elif texture_type == 'parallax':
            temp_dict = self.get_sss_settings(target_path)
        elif texture_type == 'emissive':
            temp_dict = self.get_emissive_settings(target_path)
        elif texture_type == 'basecolor':
            temp_dict = self.get_basecolor_settings(target_path)
        else:
            temp_dict = {'textureMap': self.get_relative_path(target_path)}

        return temp_dict

    def get_opacity_settings(self, target_path):
        """
        Reads Maya material to construct material opacity attributes

        :param material_name: The Maya Material name being translated for O3DE
        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting opacity settings...')
        return {'alphaSource': 'Split', 'mode': 'Cutout', 'textureMap': self.get_relative_path(target_path)}

    def get_sss_settings(self, target_path):
        """
        Reads Maya material to construct material subsurface scattering attributes

        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting Subsurface Scattering settings...')
        return {'enableSubsurfaceScattering': True, 'influenceMap': self.get_relative_path(target_path)}

    def get_emissive_settings(self, target_path):
        """
        Reads Maya material to construct material emissive attributes

        :param target_path: The path to the texture file
        :return: Attribute values as a dictionary
        """
        _LOGGER.info('Getting Emissive settings...')
        return {'enable': True, 'intensity': 4, 'textureMap': self.get_relative_path(target_path)}

    def get_basecolor_settings(self, target_path):
        """
        Reads Maya material to construct material BaseColor settings

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
        path_list = texture_path.split('/')
        file_name = path_list[-1]
        texture_list = file_name.split('_')
        base_texture_name = '_'.join(texture_list[:-1])
        return base_texture_name

    def get_relative_path(self, full_path):
        """
        Material definitions use relative paths for file textures- this function takes the full paths of assets and
        converts to the abbreviated relative path format needed for O3DE to source the files

        :param full_path: Full path to the asset
        :return:
        """
        path_parts = full_path.split('/')
        for index, part in enumerate(path_parts):
            if part == 'KB3DTextures':
                return '/'.join(path_parts[(index-1):])
        return full_path

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

    def export_o3de_material(self, output_path, material_description):
        """
        Takes one final dictionary with information gathered in the process and saves with JSON formatting into a
        .material file

        :param output:
        :param material_description:
        :return:
        """
        _LOGGER.info('Output Material File Path: {}'.format(output_path))
        with open(output_path, 'w') as material_file:
            json.dump(dict(material_description), material_file, ensure_ascii=False, indent=4)

# info_list = [target_file, self.input_directory, self.output_directory, self.all_properties_location]


_LOGGER.info(f'TargetFile: {sys.argv[1]}')
_LOGGER.info(f'InputDirectory: {sys.argv[2]}')
_LOGGER.info(f'OutputDirectory: {sys.argv[3]}')
_LOGGER.info(f'AllPropertiesLocation: {sys.argv[4]}')

ProcessFbxFile(sys.argv[1], sys.argv[2], sys.argv[3].replace('/', '\\'), sys.argv[4])

