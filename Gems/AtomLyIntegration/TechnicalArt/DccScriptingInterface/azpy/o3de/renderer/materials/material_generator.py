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

"""! @brief This script when called will export an all properties dictionary for the value passed to the first
 argument (material type). The second value is a boolean that allows you to either use the most current
 values, generated at runtime from our materialtype files from which the material editor sources, or to just
 leverage previously generated files that have been saved to the DCCsi codebase. The default is set to True. """

import os
from pathlib import Path
from box import Box
import logging as logging
import azpy.config_utils
from dynaconf import settings
from azpy.o3de.utils import o3de_utilities as o3de_helpers
from azpy.constants import FRMT_LOG_LONG
from SDK.Python import general_utilities as helpers


_LOGGER = logging.getLogger('DCCsi.azpy.o3de.renderer.materials.template_generator')


class MaterialGenerator:
    def __init__(self, **kwargs):
        self.material_definition = Box({
            'materialType': '',
            'materialTypeVersion': '',
            'propertyValues': {}
        })

    #     self.template_source_directory = Path(kwargs['O3DE_DEV']) / 'Gems/Atom/Feature/Common/Assets/Materials/Types'
    #     self.material_type_definitions = [x for x in self.template_source_directory.glob('*.materialtype')]
    #     self.material_name = self.get_material_name(kwargs['material_type'])
    #     _LOGGER.info(f'MaterialName [{self.material_name}]   Location: {self.template_source_directory}')
    #     self.material_type_dictionary = {}
    #     self.material_templates_directory = Path(kwargs['PATH_DCCSIG'])/'azpy/o3de/renderer/materials'
    #     self.material_template_path = self.get_material_definition_path(self.template_source_directory)
    #
    # def get_material(self):
    #     if self.material_template_path:
    #         template_path = self.get_material_definition_path(self.template_source_directory)
    #         self.material_type_dictionary = helpers.get_commented_json_data(template_path)
    #         return None
    #     #     self.set_material_template()
    #     #     return self.material_template
    #     # else:
    #     #     raise Exception('MATERIAL TYPE ERROR: Template file type not found.')
    #
    # def get_material_definition_path(self, base_directory):
    #     try:
    #         material_prefix = self.material_name.split('_')[0]
    #         for file in base_directory.iterdir():
    #             filename = file.name.lower()
    #             if filename.startswith(material_prefix.lower()) and filename.endswith('.materialtype'):
    #                 return file
    #     except IndexError:
    #         return None

    # def get_material_name(self, material_type):
    #     for material in self.material_type_definitions:
    #         if material.name.lower().startswith(material_type.lower()):
    #             template_name = material.name.split('.')[0] + '_template.material'
    #             return template_name
    #     return None
    #
    # def set_material_template(self):
    #     property_exclusion_list = ['defaultValue', 'description', 'enumValues', 'displayName', 'type', 'connection']
    #     self.material_template['materialType'] = f'Materials/Types/{self.material_template_path.name}'
    #     property_list = {}
    #     _LOGGER.info(f'MaterialTypeDictionary::: {self.material_type_dictionary}')
    #     for key, value in self.material_type_dictionary.items():
    #         if key == 'propertyLayout':
    #             for property_listing in value['propertyGroups']:
    #                 listing_name = property_listing['name']
    #                 for element in property_listing['properties']:
    #                     for element_name, element_value in element.items():
    #                         if element_name not in property_exclusion_list:
    #                             # Name / default value ----->>
    #                             if element_name == 'name':
    #                                 element_name = element_value
    #                                 element_value = ''
    #                                 for k, v in element.items():
    #                                     if k == 'defaultValue':
    #                                         element_value = v
    #                                         break
    #                             property_list[f'{listing_name}.{element_name}'] = element_value
    #     property_list = {key: value for key, value in sorted(property_list.items())}
    #     self.material_template['propertyValues'] = property_list
    #     o3de_helpers.export_o3de_material(self.material_name, self.material_template)


# Usage:
# material_generator = MaterialGenerator('StandardPBR', **kwargs).get_material()


