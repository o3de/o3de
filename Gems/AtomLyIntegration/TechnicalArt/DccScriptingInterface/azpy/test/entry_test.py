# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
from __future__ import unicode_literals

# -------------------------------------------------------------------------
import sys
import os
import site

# note: some modules not available in py2.7 unless we boostrap with config.py
# See example:
#"dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\SDK\Lumberyard\Scripts\set_menu.py"
from pathlib import Path

# -------------------------------------------------------------------------
_BOOT_CHECK = False  # set true to test breakpoint in this module directly

import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

#  global space
# To Do: update to dynaconf dynamic env and settings?
_G_DEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULENAME = 'azpy.test.entry_test'

_log_level = int(20)
if _G_DEBUG:
    _log_level = int(10)
_LOGGER = azpy.initialize_logger(_MODULENAME,
                                 log_to_file=False,
                                 default_log_level=_log_level)
_LOGGER.debug('Starting:: {}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def main(verbose=_G_DEBUG, connect_debugger=True):
    _LOGGER.info('{}'.format('-' * 74))
    _LOGGER.info('entry_test.main()')
    _LOGGER.info('Root test import successful:')
    _LOGGER.info('~   {}'.format(__file__))

    if connect_debugger:
        status = connect_wing()
        _LOGGER.info(status)
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def connect_wing():
    _LOGGER.info('{0}'.format('-' * 74))
    _LOGGER.info('entry_test.connect_wing()')

    try:
        _WINGHOME = os.environ['WINGHOME']  # test
        _LOGGER.info('~   WINGHOME: {0}'.format(_WINGHOME))
    except Exception as e:
        _LOGGER.warning(e)
        from azpy.constants import PATH_DEFAULT_WINGHOME
        _WINGHOME = PATH_DEFAULT_WINGHOME
        os.environ['WINGHOME'] = PATH_DEFAULT_WINGHOME
        _LOGGER.info('set WINGHOME: {0}'.format(_WINGHOME))
        pass

    _TRY_PY27 = None
    try:
        # this is py3
        import importlib
        spec = importlib.util.spec_from_file_location("wDBstub",
                                                      Path(_WINGHOME,
                                                           'wingdbstub.py'))
        wDBstub = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(wDBstub)
        _LOGGER.info('~   Success: imported wDBstub (py3)')
    except Exception as e:
        _TRY_PY27 = True
        _LOGGER.warning(e)
        _LOGGER.warning('warning: import wDBstub, FAILED (py3)')
        pass
    
    if _TRY_PY27:
        # this is a little bit hacky but can cleanup later
        try:
            os.path.exists(_WINGHOME)
            site.addsitedir(_WINGHOME)
            import wingdbstub as wDBstub
            _LOGGER.info('~   Success: imported wDBstub (py2)')
        except Exception as e:
            _LOGGER.warning(e)
            _LOGGER.warning('Error: import wDBstub, FAILED (py2)')
            raise(e)

    try:
        wDBstub.Ensure(require_connection=1, require_debugger=1)
        _LOGGER.info('~   Success: wDBstub.Ensure()')
    except Exception as e:
        _LOGGER.warning(e)
        _LOGGER.warning('Error: wDBstub.Ensure()')
        _LOGGER.warning('Check that wing ide is running')

    try:
        wDBstub.debugger.StartDebug()
        # leave a tag, so another boostrap can check if already set
        os.environ["DCCSI_DEBUGGER_ATTACHED"] = 'True'
        _LOGGER.info('~   Success: wDBstub.debugger.StartDebug()')
    except Exception as e:
        _LOGGER.warning(e)
        _LOGGER.warning('Error: wDBstub.debugger.StartDebug()')
        _LOGGER.warning('Check that wing ide is running, and not attached to another process')

    _LOGGER.info('{0}'.format('-' * 74))

    # a hack to allow a place to drop a breakpoint on boot
    while _BOOT_CHECK:
        _LOGGER.debug(str('SPAM'))

    return
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    _G_DEBUG = True
    main(verbose=_G_DEBUG, connect_debugger=_G_DEBUG)
