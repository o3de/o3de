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
#

"""! @brief Description here..."""
from azpy.o3de.renderer.materials import material_utilities
from SDK.Python import general_utilities as helpers
import logging as _logging
from pathlib import Path
from box import Box
import shutil
import json
import os


_LOGGER = _logging.getLogger('azpy.o3de.renderer.materials.MaterialGenerator')


class MaterialGenerator:
    debug_mode = True

    def __init__(self, input_material, output_material, material_attributes, output_base_path):
        self.input_material = input_material
        self.output_material = output_material
        self.material_attributes = material_attributes
        self.output_base_path = output_base_path
        self.asset_directory = None
        self.textures_directory = None
        self.dcc_application = None
        self.dcc_material_name = None
        self.dcc_material_settings = None
        self.default_dcc_material_settings = None
        self.texture_list = None
        self.mesh_export_location = None
        self.mesh_name = None
        self.material_name = None
        self.material_properties_dict = material_utilities.get_all_properties_description(output_material)
        self.material_definition = Box({})
        self.parse_material_attributes()
        self.transferable_properties = self.get_transferable_properties()

    def generate_material(self):
        """! This is the main entry point that is called in order to process a material. Currently this is
        only called as an override, based on DCC caller.
        """
        pass

    def parse_material_attributes(self):
        """! This organizes material attributes into a number of separate class attributes to make accessing the
        respective information a little more easily.
        """
        for key, values in self.material_attributes.items():
            for object_key, object_values in values.items():
                self.set_export_directory(object_key)
                for material_key, material_values in object_values.items():
                    self.dcc_material_name = material_key
                    for k, v in material_values.items():
                        if k == 'dcc_application':
                            self.dcc_application = v
                            self.get_default_material_settings()
                        elif k == 'textures':
                            self.texture_list = v
                        elif k == 'mesh_export_location':
                            self.mesh_export_location = v
                        elif k == 'settings':
                            self.dcc_material_settings = v

    def reset(self, input_material, output_material, material_attributes, output_base_path):
        """! Resets all material values in order to quickly process lists of materials one after the other. """
        self.__init__(input_material, output_material, material_attributes, output_base_path)

    ###################
    # Getters/Setters #
    ###################

    def get_transferable_properties(self):
        """! Gets key/value pairs for the intended material type definition that are mappable across to O3DE
        material values
        """
        if self.dcc_application:
            return material_utilities.get_transferable_properties(self.dcc_application, self.input_material)
        return None

    def get_default_material_settings(self):
        """! This gathers the default material settings for the assigned DCC material. The system compares these
        values with the found material attribute values- a change in values indicates that the attribute should be
        mapped across to the O3DE material definition if applicable
        """
        self.default_dcc_material_settings = material_utilities.get_default_material_settings(self.dcc_application,
                                                                                              self.input_material)

    def get_property_values(self, property_values):
        """! Maps found properties in DCC materials to corresponding values in an O3DE material

        @property_values All the material attributes gathered from the DCC package that could possibly be transferred
        to the new O3DE material definition... applied textures as well as procedural settings
        """
        properties_dictionary = {}
        for material_property, property_value in property_values.items():
            for key, value in self.transferable_properties.items():
                if value == material_property:
                    property_components = material_property.split('.')
                    # Handle Textures ------------------------>>
                    if property_components[1] == 'textureMap':
                        key_match = self.get_matching_key([key, property_components[0]], self.texture_list)
                        if key_match:
                            properties_dictionary[material_property] = self.texture_list[key_match]['path']
                            # TODO- Check if line below is necessary if textureMap is defined (I'm guessing no?)
                            # properties_dictionary[f'{property_components[0]}.useTexture'] = True
                    else:
                        # Handle Procedural Settings --------->>
                        try:
                            key_match = self.get_matching_key([key, property_components[0]],
                                                              self.default_dcc_material_settings['settings'])
                            target_value = self.get_default_value(key_match)
                            if target_value != 'default':
                                properties_dictionary[material_property] = target_value
                        except Exception as e:
                            _LOGGER.info(f'GetPropertyValues error[{type(e)}]: {e}')
        properties_dictionary = self.set_properties_formatting(properties_dictionary)
        return properties_dictionary

    def get_default_value(self, key):
        """! Cross-references default material attribute values with target scene materials of the same kind. If the
        setting is different, it returns the value present and otherwise returns "default". Default values indicate
        that the target property does not need to be translated to the O3DE material
        """
        if self.default_dcc_material_settings['settings'][key] != self.dcc_material_settings[key]:
            return self.dcc_material_settings[key]
        return 'default'

    def get_material_name(self):
        """! Format the material output name to conform to O3DE naming convention- the name of the fbx name and the
        applied material name, separated by an underscore
        """
        return f'{Path(self.mesh_export_location).stem}_{self.dcc_material_name}.material'

    def get_material_output_path(self):
        return Path(self.mesh_export_location).parent / self.get_material_name()

    def get_relative_path(self, asset_path):
        """! O3DE material definitions use relative paths in the definitions- this will truncate paths so that the
        asset processor finds assets based on this relative path requirement
        """
        asset_path = asset_path.replace('\\', '/')
        base_path_key = Path(self.output_base_path).name
        path_list = Path(asset_path).parts
        for index, part in enumerate(path_list):
            if part == base_path_key:
                return '/'.join(path_list[index:])

    @staticmethod
    def get_matching_key(search_list_items, target_dictionary):
        """! Texture attributes generally have the same names for types, but not always. This adds a little additional
        flexibility for finding the property texture key for DCC material values

        @search_list_items
        """
        for element in target_dictionary.keys():
            for key in search_list_items:
                if key == element:
                    return key
        return False

    def set_export_directory(self, object_name):
        """! The export directory location is offset from the base directory specified when running the export
        materials tool. This formatting helps for organizing assets to be ready for use in the engine.

        @object_name Each exported object from a DCC package will be contained in an asset directory matching the
        object's name in the scene
        """
        self.mesh_name = object_name
        self.textures_directory = Path(self.output_base_path) / object_name / 'textures'
        try:
            self.textures_directory.mkdir(parents=True)
            self.asset_directory = self.textures_directory.parent
        except FileExistsError:
            pass

    @staticmethod
    def set_properties_formatting(properties_dictionary):
        formatted_dictionary = Box({})
        for key, value in properties_dictionary.items():
            attribute_parts = key.split('.')
            if attribute_parts[0] not in formatted_dictionary.keys():
                formatted_dictionary[attribute_parts[0]] = {attribute_parts[-1]: value}
            else:
                formatted_dictionary[attribute_parts[0]].update({attribute_parts[-1]: value})
        return formatted_dictionary.to_dict()

    ##########
    # Export #
    ##########

    def export_json(self):
        """! This is a convenience function for debugging. Both an "all properties" and "dcc material data" json file
        will be output to the user desktop when debug mode class property is enabled.

        """
        # Material data from DCC package --------->>
        results_data = Path(os.environ['USERPROFILE']) / 'Desktop/dcc_material_data.json'
        helpers.set_json_data(results_data, self.material_attributes)
        # "All properties" data for target material type ---------->>
        all_properties_data = Path(os.environ['USERPROFILE']) / 'Desktop/all_properties_data.json'
        helpers.set_json_data(all_properties_data, self.material_properties_dict)

    def export_mesh(self):
        material_utilities.export_mesh(self.dcc_application, self.mesh_name, self.mesh_export_location)

    def export_textures(self):
        for texture_type, texture_info in self.texture_list.items():
            if texture_info:
                texture_path = Path(texture_info['path'])
                dst = self.textures_directory / texture_path.name
                shutil.copy(str(texture_path), str(dst))
                self.texture_list[texture_type]['path'] = self.get_relative_path(str(dst))

    def export_material(self):
        """! Takes constructed 'material_definition' dictionary with information gathered in the process and saves with
        JSON formatting into a .material file

        @param output_path This path will likely differ from the 'output_base_path' passed in class arguments, which
        provides a starting point from which output locations can branch from, and depends on number of objects being
        processed by an operation
        """
        # if self.debug_mode:
        #     self.export_json()

        material_output_path = self.get_material_output_path()
        helpers.set_json_data(material_output_path, self.material_definition)


# ++++++++++++++++-------------------------------------------------->>
# MAYA EXTENSION +------------------------------------------------------>>
# ++++++++++++++++-------------------------------------------------->>

class MayaMaterialGenerator(MaterialGenerator):
    def __init__(self, input_material, output_material, material_attributes, output_base_path):
        super().__init__(input_material, output_material, material_attributes, output_base_path)

    def generate_material(self):
        _LOGGER.info('\n\n+++++++++++++++++++++++++++++++++++++\nMayaMaterialGenerator instance created\n'
                     '+++++++++++++++++++++++++++++++++++++\n\n')
        _LOGGER.info(f'InputMaterial: {self.input_material}')
        _LOGGER.info(f'MaterialAttributes: {self.material_attributes}')
        _LOGGER.info(f'OutputBasePath: {self.output_base_path}')
        _LOGGER.info(f'Transferable properties: {self.transferable_properties}')

        self.export_mesh()
        self.export_textures()
        self.material_name = self.get_material_name()

        for key, values in self.material_properties_dict.items():
            if key != 'propertyValues':
                self.material_definition[key] = values
            else:
                self.material_definition[key] = self.get_property_values(values)

        try:
            self.export_material()
            return {self.mesh_export_location: {self.material_name: self.material_definition}}
        except Exception as e:
            _LOGGER.info(f'Exception [{type(e)}]: {e}')
            return {}


# +++++++++++++++++++----------------------------------------------->>
# BLENDER EXTENSION +------------------------------------------------------>>
# +++++++++++++++++++----------------------------------------------->>

class BlenderMaterialGenerator(MaterialGenerator):
    def __init__(self, input_material, output_material, material_attributes, output_base_path):
        super().__init__(input_material, output_material, material_attributes, output_base_path)

        self.transferable_properties = material_utilities.get_transferable_properties('Blender', self.input_material)

    def generate_material(self):
        _LOGGER.info('\n\n++++++++++++++++++++++++++++++++++++++++\nBlenderMaterialGenerator instance fired\n'
                     '++++++++++++++++++++++++++++++++++++++++\n\n')
        _LOGGER.info(f'InputMaterial: {self.input_material}')
        _LOGGER.info(f'MaterialAttributes: {self.material_attributes}')
        _LOGGER.info(f'OutputBasePath: {self.output_base_path}')
        _LOGGER.info(f'Transferable properties: {self.transferable_properties}')
        return self.material_definition


# ++++++++++++++++++++++++------------------------------------->>
# TRANSLATION ASSIGNMENT +----------------------------------------->>
# ++++++++++++++++++++++++------------------------------------->>

def export_standard_pbr_materials(material_info, output_path):
    exported_materials = {}
    maya_translator = None
    blender_translator = None
    for scene_name, scene_values in material_info.items():
        translator = None
        for material_key, material_values in scene_values.items():
            for k, v in material_values.items():
                dcc_application = v['dcc_application']
                input_material = v['material_type']
                output_material = 'standardPBR'

                if dcc_application == 'Maya':
                    if maya_translator:
                        maya_translator.reset(input_material, output_material, material_info, output_path)
                    else:
                        maya_translator = MayaMaterialGenerator(input_material, output_material, material_info,
                                                                output_path)
                    translator = maya_translator
                elif dcc_application == 'Blender':
                    if blender_translator:
                        blender_translator.reset(input_material, output_material, material_info, output_path)
                    else:
                        blender_translator = BlenderMaterialGenerator(input_material, output_material, material_info,
                                                                      output_path)
                    translator = blender_translator

                material_definition = translator.generate_material()
                exported_materials.update(material_definition)

    return exported_materials

