# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
# -- Standard Python modules --
import sys
import os
import inspect
import logging as _logging

# -- External Python modules --
# none

# -- Extension Modules --
import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# --------------------------------------------------------------------------
# -- Global Definitions --
_DCCSI_G_DCC_APP = None

# set up global space, logging etc.
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_MODULENAME = 'azpy.dev.utils.check.maya_app'
_LOGGER = _logging.getLogger(_MODULENAME)
# -------------------------------------------------------------------------


###########################################################################
## These mini-functions need to be defined, before they are called
# -------------------------------------------------------------------------
# run this, if we are in Maya
def set_dcc_app(dcc_app='maya'):
    """
    azpy.dev.utils.check.maya.set_dcc_app()
    this will set global _DCCSI_G_DCC_APP = 'maya'
    and os.environ["DCCSI_G_DCC_APP"] = 'maya'
    """
    _DCCSI_G_DCC_APP = dcc_app

    _LOGGER.info('Setting DCCSI_G_DCC_APP to: {0}'.format(dcc_app))

    return _DCCSI_G_DCC_APP
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def clear_dcc_app(dcc_app=False):
    """
    azpy.dev.utils.check.maya.set_dcc_app()
    this will set global _DCCSI_G_DCC_APP = False
    and os.environ["DCCSI_G_DCC_APP"] = False
    """
    _DCCSI_G_DCC_APP = dcc_app

    _LOGGER.info('Setting DCCSI_G_DCC_APP to: {0}'.format(dcc_app))

    return _DCCSI_G_DCC_APP
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def validate_state(DCCSI_G_DCC_APP=_DCCSI_G_DCC_APP):
    '''
    This will detect if we are running in Maya or not,
    then will call either, set_dcc_app('maya') or clear_dcc_app(dcc_app=False)
    '''

    if _DCCSI_GDEBUG:
        _LOGGER.debug(autolog())

    try:
        import maya.cmds as cmds
        DCCSI_G_DCC_APP = set_dcc_app('maya')
    except ImportError as e:
        _LOGGER.warning('Can not perform: import maya.cmds as cmds')
        DCCSI_G_DCC_APP = clear_dcc_app()
    else:
        try:
            if cmds.about(batch=True):
                DCCSI_G_DCC_APP = set_dcc_app('maya')
        except AttributeError as e:
            _LOGGER.warning("maya.cmds module isn't fully loaded/populated, "
                            "(cmds populates only in batch, maya.standalone, or maya GUI)")
            # NO Maya
            DCCSI_G_DCC_APP=clear_dcc_app()

    return DCCSI_G_DCC_APP
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def autolog():
    '''Automatically log the current function details.'''
    # Get the previous frame in the stack, otherwise it would
    # be this function!!!
    func = inspect.currentframe().f_back.f_back.f_code
    # Dump the message + the name of this function to the log.
    output = ('{module} AUTOLOG:\r'
              'Called from::\n{0}():\r'
              'In file: {1},\r'
              'At line: {2}\n'
              ''.format(func.co_name,
                        func.co_filename,
                        func.co_firstlineno,
                        module=_MODULENAME))
    return output
#-------------------------------------------------------------------------


# -------------------------------------------------------------------------
# run the check on import
_DCCSI_G_DCC_APP = validate_state()
# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # there are not really tests to run here due to this being a list of
    # constants for shared use.
    _DCCSI_GDEBUG = True
    _DCCSI_DEV_MODE = True
    _LOGGER.setLevel(_logging.DEBUG)  # force debugging

    ## reduce cyclical azpy imports
    ## it only has a basic logger configured, add log to console
    #_handler = _logging.StreamHandler(sys.stdout)
    #_handler.setLevel(_logging.DEBUG)
    #FRMT_LOG_LONG = "[%(name)s][%(levelname)s] >> %(message)s (%(asctime)s; %(filename)s:%(lineno)d)"
    #_formatter = _logging.Formatter(FRMT_LOG_LONG)
    #_handler.setFormatter(_formatter)
    #_LOGGER.addHandler(_handler)
    #_LOGGER.debug('Loading: {0}.'.format({_MODULENAME}))

    # happy print
    from azpy.constants import STR_CROSSBAR
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info('{} ... Running script as __main__'.format(_MODULENAME))
    _LOGGER.info(STR_CROSSBAR)

    _DCCSI_G_DCC_APP = validate_state()
    _LOGGER.info('Is Maya Running? _DCCSI_G_DCC_APP = {}'.format(_DCCSI_G_DCC_APP))
