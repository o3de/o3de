import json
import os
import sys
from jsonschema import validate
from pathlib import Path
from box import Box
import logging as _logging
import azpy.config_utils
from azpy.constants import FRMT_LOG_LONG
from SDK.Python import general_utilities as helpers

"""

First use envars to target this directory:
E:\Depot\o3de_dccsi\Gems\Atom\Feature\Common\Assets\Materials\Types

For now I'll use the logging code for setup, but this really needs to be cleaned up and be a two line addition to
modules needing to use logging. 

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
    def __init__(self, material_type, output_path):
        self.material_type = material_type
        self.target_material_dictionary = {}
        self.output_path = output_path
        self.template_source_directory = Path(os.environ['O3DE_DEV'])/'Gems/Atom/Feature/Common/Assets/Materials/Types'
        self.material_type_definitions = [x for x in self.template_source_directory.glob('*.materialtype')]
        self.build_template()

    def build_template(self):
        _LOGGER.info(f'Material template base directory: {self.template_source_directory}')
        self.get_material_definition()
        self.format_material_definition()

    def get_material_definition(self):
        for target_file in self.material_type_definitions:
            if self.material_type.lower() in str(target_file).lower():
                self.target_material_dictionary = helpers.get_commented_json_data(target_file)

    def format_material_definition(self):
        for key, value in self.target_material_dictionary.items():
            _LOGGER.info(f'Key: {key}   Value: {value}')



template_generator = TemplateGenerator('standardPBR', 'path/to/directory')




