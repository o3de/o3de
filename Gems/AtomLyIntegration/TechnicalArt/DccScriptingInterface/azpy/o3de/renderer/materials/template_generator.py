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
import logging as _logging
import DccScriptingInterface.azpy.config_utils
from DccScriptingInterface.azpy.o3de.utils import o3de_utilities as o3de_helpers
from DccScriptingInterface.azpy.constants import FRMT_LOG_LONG
from DccScriptingInterface.azpy import general_utils as helpers


_config = DccScriptingInterface.azpy.config_utils.get_dccsi_config()
settings = _config.get_config_settings()

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

module_name = 'azpy.o3de.renderer.materials.TemplateGenerator'

_log_level = int(20)
_G_DEBUG = False
if _G_DEBUG:
    _log_level = int(10)


_logging.basicConfig(level=_log_level,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(module_name)

# console_handler = _logging.StreamHandler(sys.stdout)
# console_handler.setLevel(_log_level)
# formatter = _logging.Formatter(FRMT_LOG_LONG)
# console_handler.setFormatter(formatter)
# _LOGGER.addHandler(console_handler)
# _LOGGER.setLevel(_log_level)
# _LOGGER.debug('Initializing: {0}.'.format({module_name}))


class TemplateGenerator:
    def __init__(self, material_type, attribute_validation=True):
        self.material_template = Box({
            'description': '',
            'parentMaterial': '',
            'materialType': '',
            'materialTypeVersion': '',
            'propertyValues': {}
        })
        self.template_source_directory = Path(os.environ['O3DE_DEV']) / 'Gems/Atom/Feature/Common/Assets/Materials/Types'
        self.material_type_definitions = [x for x in self.template_source_directory.glob('*.materialtype')]
        self.material_name = self.get_material_name(material_type)
        self.material_type_dictionary = {}
        self.attribute_validation = attribute_validation
        self.material_templates_directory = Path(os.environ['PATH_DCCSIG'])/'azpy/o3de/renderer/materials'
        self.material_template_path = self.get_material_definition_path(self.template_source_directory)

    def get_template(self):
        if self.material_template_path:
            if self.attribute_validation:
                template_path = self.get_material_definition_path(self.template_source_directory)
                self.material_type_dictionary = helpers.get_commented_json_data(template_path)
                self.set_material_template()
                return self.material_template
        else:
            raise Exception('MATERIAL TYPE ERROR: Template file type not found.')

    def get_material_definition_path(self, base_directory):
        try:
            material_prefix = self.material_name.split('_')[0]
            for file in base_directory.iterdir():
                filename = file.name.lower()
                if filename.startswith(material_prefix.lower()) and filename.endswith('.materialtype'):
                    return file
        except IndexError:
            return None

    def get_material_name(self, material_type):
        for material in self.material_type_definitions:
            if material.name.lower().startswith(material_type.lower()):
                template_name = material.name.split('.')[0] + '_template.material'
                return template_name
        return None

    def set_material_template(self):
        property_exclusion_list = ['defaultValue', 'description', 'enumValues', 'displayName', 'type', 'connection']
        self.material_template['materialType'] = f'Materials/Types/{self.material_template_path.name}'
        property_list = {}
        for key, value in self.material_type_dictionary.items():
            if key == 'propertyLayout':
                for property_listing in value['propertyGroups']:
                    listing_name = property_listing['name']
                    for element in property_listing['properties']:
                        for element_name, element_value in element.items():
                            if element_name not in property_exclusion_list:
                                # Name / default value ----->>
                                if element_name == 'name':
                                    element_name = element_value
                                    element_value = ''
                                    for k, v in element.items():
                                        if k == 'defaultValue':
                                            element_value = v
                                            break
                                property_list[f'{listing_name}.{element_name}'] = element_value
        property_list = {key: value for key, value in sorted(property_list.items())}
        self.material_template['propertyValues'] = property_list
        o3de_helpers.export_o3de_material(self.material_name, self.material_template)


# Usage:
# template_generator = TemplateGenerator('StandardPBR', True).get_template()


