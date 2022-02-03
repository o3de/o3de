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
from __future__ import unicode_literals
import os
import site
import logging as _logging

# note: some modules not available in py2.7 unless we boostrap with config.py
# See example:
#"dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface\SDK\Lumberyard\Scripts\set_menu.py"
from pathlib import Path
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.test.entry_test'
_BOOT_CHECK = False  # set true to test breakpoint in this module directly

from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import ENVAR_DCCSI_LOGLEVEL
from azpy.constants import ENVAR_DCCSI_GDEBUGGER
from azpy.constants import FRMT_LOG_LONG

_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)
_DCCSI_GDEBUGGER = env_bool(ENVAR_DCCSI_GDEBUGGER, 'WING')

# default loglevel to info unless set
_DCCSI_LOGLEVEL = int(env_bool(ENVAR_DCCSI_LOGLEVEL, _logging.INFO))
if _DCCSI_GDEBUG:
    # override loglevel if runnign debug
    _DCCSI_LOGLEVEL = _logging.DEBUG
    
# set up module logging
#for handler in _logging.root.handlers[:]:
    #_logging.root.removeHandler(handler)
    
# configure basic logger
# note: not using a common logger to reduce cyclical imports
_logging.basicConfig(level=_DCCSI_LOGLEVEL,
                    format=FRMT_LOG_LONG,
                    datefmt='%m-%d %H:%M')

_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def main(verbose=_DCCSI_GDEBUG, connect_debugger=True):
    if verbose:
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
        _LOGGER.info(e)
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
    _DCCSI_GDEBUG = True
    main(verbose=_DCCSI_GDEBUG, connect_debugger=_DCCSI_GDEBUG)
