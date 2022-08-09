import importlib.util
from dynaconf import settings
import logging
import sys


_LOGGER = logging.getLogger('azpy.config_utils')


def load_preconfigured_settings(target_file):
    settings.load_file(path=f'{target_file}')


def load_module_by_path(module_name, module_path):
    spec = importlib.util.spec_from_file_location(module_name, module_path)
    location = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = location
    spec.loader.exec_module(location)


def test_import(target_module: str, set_variable=None, import_module=False):
    spec = importlib.util.find_spec(target_module)
    module_access = False
    if target_module in sys.modules:
        module_access = True
    elif spec is not None:
        module_access = True
        if import_module:
            try:
                module = importlib.util.module_from_spec(spec)
                sys.modules[target_module] = module
                spec.loader.exec_module(module)
            except Exception as e:
                _LOGGER.info(f'Import failed: {e}')
    if set_variable:
        settings.set(set_variable, module_access)
    _LOGGER.info(f"ModuleAccess[{target_module}] ::: {module_access}")
    return module_access


def attach_debugger():
    _LOGGER.info('Attach debugger fired')

