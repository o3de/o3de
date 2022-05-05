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


_LOGGER = _logging.getLogger('azpy.o3de.renderer.materials.o3de_material_generator')


class MaterialGenerator:
    debug_mode = True

    def __init__(self, input, output, attributes, base_path, options, master_paths):
        self.input_material = input
        self.output_material = output
        self.material_attributes = attributes
        self.output_base_path = base_path
        self.export_options = options
        self.master_paths = master_paths
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
        self.parent_hierarchy = None
        self.material_properties_dict = material_utilities.get_all_properties_description(output)
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
        for object_key, material_data in self.material_attributes.items():
            for material_key, material_values in material_data.items():
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
                    elif k == 'parent_hierarchy':
                        self.parent_hierarchy = v
            self.set_export_directory(object_key)

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
                    _LOGGER.info(f'MaterialProperty: {material_property}')
                    property_components = material_property.split('.')
                    # Handle Textures ------------------------>>
                    if property_components[1] == 'textureMap':
                        key_match = self.get_matching_key([key, property_components[0]], self.texture_list)
                        if key_match:
                            try:
                                target_path = self.texture_list[key_match]['path']
                                _LOGGER.info(f'\n\n++++++++++++++ MaterialProperty: {material_property}')
                                _LOGGER.info(f'--> {key_match} -- {target_path}')
                                properties_dictionary[material_property] = target_path
                            except TypeError:
                                pass
                            except KeyError:
                                pass
                    else:
                        # Handle Procedural Settings --------->>
                        # Please note- this section is meant to catch changes to default shader values. If a change
                        # is found to the values, an attempt to carry over the value to a transferable O3DE material
                        # property will be made. After much troubleshooting, I believe that the Stingray material
                        # creation bug I found for creating new StingrayPBS materials in Standalone mode is also
                        # present in IDE-driven Maya sessions. If you create an instance of this material
                        # programmatically several key attributes needed to assign texture maps as well as procedural
                        # settings are missing in the attributes. This passage should work for all other material types,
                        # but will currently not work on StingrayPBS
                        try:
                            key_match = self.get_matching_key([key, property_components[0]],
                                                              self.default_dcc_material_settings['settings'])
                            target_value = self.get_default_value(key_match)
                            if target_value != 'default':
                                properties_dictionary[material_property] = target_value
                        except KeyError:
                            pass
        properties_dictionary = self.set_properties_formatting(properties_dictionary)
        _LOGGER.info(f'++++++++ PropertiesDictionary: {properties_dictionary}')
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
        if not self.parent_hierarchy:
            return Path(self.mesh_export_location).parent / self.get_material_name()
        else:
            return Path(self.output_base_path) / self.parent_hierarchy[-1] / \
                   f'{self.parent_hierarchy[-1]}_{self.dcc_material_name}.material'

    def get_relative_path(self, asset_path):
        """! O3DE material definitions use relative paths in the definitions- this will truncate paths so that the
        asset processor finds assets based on this relative path requirement
        """
        path_list = Path(asset_path).parts
        for index, part in enumerate(path_list):
            if part == 'Assets':
                target_index = index + 1
                return '/'.join(path_list[target_index:])

    # Below is the version I was using WITHOUT exporting assets into Gem Formatted directories. You should probably
    # include functionality to create Gem or just create converted assets that would be distributed ad hoc
    # def get_relative_path(self, asset_path):
    #     """! O3DE material definitions use relative paths in the definitions- this will truncate paths so that the
    #     asset processor finds assets based on this relative path requirement
    #     """
    #     asset_path = asset_path.replace('\\', '/')
    #     base_path_key = Path(self.output_base_path).name
    #     path_list = Path(asset_path).parts
    #     for index, part in enumerate(path_list):
    #         if part == base_path_key:
    #             return '/'.join(path_list[index:])

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
        object_name = self.parent_hierarchy[-1] if self.parent_hierarchy else object_name
        self.textures_directory = Path(self.output_base_path) / object_name / 'textures'
        try:
            self.textures_directory.mkdir(parents=True)
        except FileExistsError:
            pass
        self.asset_directory = self.textures_directory.parent

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
        if 'Preserve Grouped Objects' not in self.export_options:
            material_utilities.export_mesh(self.dcc_application, self.mesh_name, self.mesh_export_location,
                                           self.export_options)

    def export_textures(self):
        for texture_type, texture_info in self.texture_list.items():
            if texture_info:
                src = Path(texture_info['path'])
                t = str(src).replace('\\', '/')
                dst = Path(self.output_base_path) / self.master_paths[t]['object'] / 'textures' / src.name
                os.makedirs(os.path.dirname(str(dst)), exist_ok=True)

                if not dst.is_file():
                    try:
                        shutil.copy(str(src), str(dst))
                        self.texture_list[texture_type]['path'] = self.get_relative_path(str(dst))
                    except FileNotFoundError:
                        _LOGGER.info(f'Texture [{self.texture_list[texture_type]["path"]}] not found. Skipping...')
                else:
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
        _LOGGER.info(f'ExportingMaterial..... {material_output_path}')
        helpers.set_json_data(material_output_path, self.material_definition)


# ++++++++++++++++-------------------------------------------------->>
# MAYA EXTENSION +------------------------------------------------------>>
# ++++++++++++++++-------------------------------------------------->>

class MayaMaterialGenerator(MaterialGenerator):
    def __init__(self, input, output, attributes, base_path, options, master_paths):
        super().__init__(input, output, attributes, base_path, options, master_paths)

    def generate_material(self):
        _LOGGER.info('\n\n+++++++++++++++++++++++++++++++++++++\nMayaMaterialGenerator instance created\n'
                     '+++++++++++++++++++++++++++++++++++++\n\n')
        _LOGGER.info(f'InputMaterial: {self.input_material}')
        _LOGGER.info(f'MaterialAttributes: {self.material_attributes}')
        _LOGGER.info(f'OutputBasePath: {self.output_base_path}')
        _LOGGER.info(f'Transferable properties: {self.transferable_properties}')
        _LOGGER.info(f'Export options: {self.export_options}')

        self.export_mesh()
        self.export_textures()
        self.material_name = self.get_material_name()

        _LOGGER.info(f'MaterialName: {self.material_name}')
        _LOGGER.info(f'MaterialsPropertiesDict: {self.material_properties_dict}')

        for key, values in self.material_properties_dict.items():
            self.material_definition[key] = values if key != 'propertyValues' else self.get_property_values(values)
        _LOGGER.info(f'MaterialDefinition: {self.material_definition}')

        try:
            _LOGGER.info(f'MaterialExport [{self.material_name}]: {self.material_definition}')
            self.export_material()
            return {self.mesh_export_location: {self.material_name: self.material_definition}}
        except Exception as e:
            _LOGGER.info(f'Exception [{type(e)}]: {e}')
            return {}


# +++++++++++++++++++----------------------------------------------->>
# BLENDER EXTENSION +------------------------------------------------------>>
# +++++++++++++++++++----------------------------------------------->>

class BlenderMaterialGenerator(MaterialGenerator):
    def __init__(self, input, output, attributes, base_path, options, master_paths):
        super().__init__(input, output, attributes, base_path, options, master_paths)

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


def get_dcc_application(target_dictionary):
    for scene_name, translation_values in target_dictionary.items():
        for object_key, material_data in translation_values.items():
            for material_name, material_values in material_data.items():
                return material_values['dcc_application']


def export_grouped_objects(material_info, output_path, dcc_application, export_options):
    for key, values in material_info.items():
        parents = get_scene_export_hierarchy(values)
        if not parents:
            output_path = Path(output_path) / key / f'{key}.fbx'
            os.makedirs(os.path.dirname(str(output_path)), exist_ok=True)
            material_utilities.export_mesh(dcc_application, key, output_path, export_options)


def get_scene_export_hierarchy(translation_values):
    for material_name, material_values in translation_values.items():
        return material_values['parent_hierarchy']


def export_standard_pbr_materials(material_info, base_path, options, master_paths):
    exported_materials = {}
    dcc_application = get_dcc_application(material_info)
    for scene_name, translation_values in material_info.items():
        if 'Preserve Grouped Objects' in options:
            export_grouped_objects(translation_values, base_path, dcc_application, options)

        for object_key, material_data in translation_values.items():
            _LOGGER.info(f'\n\nObjectKey: {object_key}')
            t = None
            for material_name, material_values in material_data.items():
                _LOGGER.info(f'-----> MaterialName: {material_name}  MaterialValues: {material_values}')
                input = material_values['material_type']
                _LOGGER.info(f'------------> InputMaterial: {input}')
                attributes = {object_key: material_data}
                _LOGGER.info(f'------------> MaterialAttributes: {attributes}')
                _LOGGER.info(f'------------> OutputPath: {base_path}')
                _LOGGER.info(f'------------> Export Options: {options}')
                _LOGGER.info(f'------------> DccApplication: {dcc_application}')

                if dcc_application == 'Maya':
                    t = MayaMaterialGenerator(input, 'standardPBR', attributes, base_path, options, master_paths)
                elif dcc_application == 'Blender':
                    t = BlenderMaterialGenerator(input, 'standardPBR', attributes, base_path, options, master_paths)

                material_definition = t.generate_material()
                _LOGGER.info(f'MaterialDefinition: {material_definition}')
                exported_materials.update(material_definition)
    return exported_materials

