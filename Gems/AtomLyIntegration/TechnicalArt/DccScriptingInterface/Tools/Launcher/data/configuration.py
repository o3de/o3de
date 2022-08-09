from dynaconf import settings
import data.project_constants as constants
from box import Box
import logging
import os

try:
    import config
except KeyError:
    for key, value in constants.BOOTSTRAP_VARIABLES.items():
        os.environ[key] = str(value)
finally:
    import config

_LOGGER = logging.getLogger('Tools.Dev.Windows.Launcher.configuration')


class Configuration:
    def __init__(self):
        self.environment_variables = Box({k: v for k, v in settings.items()})

    def initialize_variables(self, environment_variables):
        for key, value in environment_variables.items():
            self.set_variable(key, value)

    def query_variables(self):
        for key, value in self.environment_variables.items():
            _LOGGER.info(f'Key: {key}  Value: {value}')

    def get_variable(self, key: str) -> str:
        if key in self.environment_variables.keys():
            return self.environment_variables[key]
        return ''

    def set_variable(self, key: str, value: str):
        self.environment_variables[key] = value
        os.environ[key] = str(value)

    def set_variables(self, target_dict: dict):
        for key, value in target_dict.items():
            self.set_variable(key, value)




