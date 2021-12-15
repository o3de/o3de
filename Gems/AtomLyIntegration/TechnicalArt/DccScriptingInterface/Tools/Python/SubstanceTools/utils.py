import os
import site
import importlib
from importlib import import_module
from importlib.util import spec_from_file_location, module_from_spec
from pathlib import Path

# we need to set up basic access to the DCCsi
_MODULE_PATH = os.path.realpath(__file__)  # To Do: what if frozen?
_PATH_DCCSIG = os.path.normpath(os.path.join(_MODULE_PATH, '../../../..'))
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', _PATH_DCCSIG)
site.addsitedir(_PATH_DCCSIG)


# _settings.setenv()  # doing this will add the additional DYNACONF_ envars
def get_dccsi_config(PATH_DCCSIG=_PATH_DCCSIG):
    """Convenience method to set and retreive settings directly from module."""

    # we can go ahead and just make sure the the DCCsi env is set
    # _config is SO generic this ensures we are importing a specific one
    _spec_dccsi_config = importlib.util.spec_from_file_location("dccsi._config",
                                                                Path(PATH_DCCSIG,
                                                                     "config.py"))
    _dccsi_config = importlib.util.module_from_spec(_spec_dccsi_config)
    _spec_dccsi_config.loader.exec_module(_dccsi_config)

    return _dccsi_config


# set and retrieve the base env context/_settings on import
_config = get_dccsi_config()
settings = _config.get_config_settings(enable_o3de_python=True, enable_o3de_pyside2=True)
# _config.test_pyside2()