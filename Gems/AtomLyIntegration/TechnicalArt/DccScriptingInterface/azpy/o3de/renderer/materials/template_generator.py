import json
import os
import sys
from jsonschema import validate
from pathlib import Path
from box import Box
import logging as _logging
import azpy.config_utils
from azpy.o3de.utils import o3de_utilities as o3de_helpers
from azpy.constants import FRMT_LOG_LONG
from SDK.Python import general_utilities as helpers

"""
Usage:
template_info = TemplateGenerator('standardPBR', 'path/to/directory', True).get_template()

Returns:
Structured Dictionary (JSON) with all material attributes listed, as well as their default values.
"""


_config = azpy.config_utils.get_dccsi_config()
settings = _config.get_config_settings()

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

module_name = 'azpy.o3de.renderer.materials.TemplateGenerator'

_log_level = int(20)
_G_DEBUG = True
if _G_DEBUG:
    _log_level = int(10)


_logging.basicConfig(level=_log_level,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(module_name)

console_handler = _logging.StreamHandler(sys.stdout)
console_handler.setLevel(_log_level)
formatter = _logging.Formatter(FRMT_LOG_LONG)
console_handler.setFormatter(formatter)
_LOGGER.addHandler(console_handler)
_LOGGER.setLevel(_log_level)
_LOGGER.debug('Initializing: {0}.'.format({module_name}))


class TemplateGenerator:
    def __init__(self, material_type, attribute_validation=False):
        self.material_template = {
            'description': '',
            'parentMaterial': '',
            'materialTypeVersion': '',
            'properties': {}
        }
        self.material_type = material_type
        self.attribute_validation = attribute_validation
        self.material_templates_directory = Path(os.environ['PATH_DCCSIG'])/'azpy/o3de/renderer/materials'
        self.material_template_path = self.get_template_path(self.material_templates_directory)
        self.template_source_directory = Path(os.environ['O3DE_DEV'])/'Gems/Atom/Feature/Common/Assets/Materials/Types'
        self.material_type_definitions = [x for x in self.template_source_directory.glob('*.materialtype')]

    def get_template(self):
        self.material_template_path = self.get_template_path(self.material_templates_directory)
        if self.attribute_validation or not self.material_template_path:
            template_path = self.get_template_path(self.template_source_directory)
            self.material_template = helpers.get_commented_json_data(template_path)
            self.set_material_template()
        return self.material_template

    def get_template_path(self, base_directory):
        try:
            return [x for x in base_directory.iterdir() if str(x.name).lower().startswith(self.material_type.lower())][0]
        except IndexError:
            return None

    def get_material_template_name(self):
        material_name_base = self.material

    def set_material_template(self):
        material_property_values = {}
        for key, value in self.material_template.items():
            if key == 'propertyLayout':
                for properties in self.material_template[key]['propertyGroups']:
                    for k, v in properties.items():
                        target_property = None
                        target_property_values = {}
                        if k == 'properties':
                            for property_list in v:
                                attr_name = property_list['name']
                                attr_values = property_list['defaultValue'] if 'defaultValue' in property_list.keys() else ''
                                target_property_values[attr_name] = attr_values
                        elif k == 'name':
                            target_property = v
                        material_property_values[target_property] = target_property_values
            else:
                if key in self.material_template.keys():
                    self.material_template[key] = value
        self.material_template['properties'] = material_property_values
        o3de_helpers.export_o3de_material(self.material_template_path, self.material_template)
        _LOGGER.info(f'MaterialTemplatePath: {self.material_template_path}')


template_generator = TemplateGenerator('standardPBR', True).get_template()
_LOGGER.info(f'Returned Template Info: {template_generator}')

