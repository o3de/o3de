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
from __future__ import unicode_literals
'''
Module: <DCCsi>\\azpy\\shared\\common\\base_env.py

This module packs the most basic set of environment variables.

Easy access IF you know the environment is setup.
We assume these are already set up in the envionment.

The str() tag for each ENVAR is defined in azpy.shared.common.constants
Allowing those str('tag') to easily be changed in a single location.

If they are paths for code acess we assume they were put on the sys path.
'''
# -------------------------------------------------------------------------
# built in's
import os
import sys
import json
import logging as _logging
from collections import OrderedDict

# 3rd Party
from pathlib import Path

# Lumberyard extensions
from azpy.constants import *
from azpy.shared.common.core_utils import return_stub
from azpy.shared.common.core_utils import get_stub_check_path
from azpy.shared.common.envar_utils import get_envar_default
from azpy.shared.common.envar_utils import set_envar_defaults
from azpy.shared.common.envar_utils import Validate_Envar

from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
_PACKAGENAME = 'azpy.env_base'

_logging.basicConfig(level=_logging.INFO,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M')
_LOGGER = _logging.getLogger(_PACKAGENAME)
_LOGGER.debug('Initializing: {0}.'.format({_PACKAGENAME}))

#  global space
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

# set up base totally non-functional defauls (denoted with $<ENVAR>)
# if something hasn't been set, it will stay '$<envar>'
_BASE_ENVVAR_DICT = OrderedDict()

#  project tag
_BASE_ENVVAR_DICT[ENVAR_O3DE_PROJECT] = '${0}'.format(ENVAR_O3DE_PROJECT)

#  paths
_BASE_ENVVAR_DICT[ENVAR_O3DE_DEV] = Path('${0}'.format(ENVAR_O3DE_DEV))
_BASE_ENVVAR_DICT[ENVAR_O3DE_PROJECT_PATH] = Path('${0}'.format(ENVAR_O3DE_PROJECT_PATH))
_BASE_ENVVAR_DICT[ENVAR_DCCSIG_PATH] = Path('${0}'.format(ENVAR_DCCSIG_PATH))
_BASE_ENVVAR_DICT[ENVAR_DCCSI_LOG_PATH] = Path('${0}'.format(ENVAR_DCCSI_LOG_PATH))
_BASE_ENVVAR_DICT[ENVAR_DCCSI_AZPY_PATH] = Path('${0}'.format(ENVAR_DCCSI_AZPY_PATH))
_BASE_ENVVAR_DICT[ENVAR_DCCSI_TOOLS_PATH] = Path('${0}'.format(ENVAR_DCCSI_TOOLS_PATH))

# dev env flags
_BASE_ENVVAR_DICT[ENVAR_DCCSI_GDEBUG] = '${0}'.format(ENVAR_DCCSI_GDEBUG)
_BASE_ENVVAR_DICT[ENVAR_DCCSI_DEV_MODE] = '${0}'.format(ENVAR_DCCSI_DEV_MODE)
_BASE_ENVVAR_DICT[ENVAR_DCCSI_GDEBUGGER] = '${0}'.format(ENVAR_DCCSI_GDEBUGGER)

#  default python dist
_BASE_ENVVAR_DICT[ENVAR_DCCSI_PY_VERSION_MAJOR] = '${0}'.format(ENVAR_DCCSI_PY_VERSION_MAJOR)
_BASE_ENVVAR_DICT[ENVAR_DCCSI_PY_VERSION_MINOR] = '${0}'.format(ENVAR_DCCSI_PY_VERSION_MINOR)
_BASE_ENVVAR_DICT[ENVAR_DCCSI_PYTHON_PATH] = '${0}'.format(ENVAR_DCCSI_PYTHON_PATH)
_BASE_ENVVAR_DICT[ENVAR_DCCSI_PYTHON_LIB_PATH] = '${0}'.format(ENVAR_DCCSI_PYTHON_LIB_PATH)

#  try to fetch and set the base values from the environment
#  this makes sure all envars set, are resolved on import
_BASE_ENVVAR_DICT = set_envar_defaults(_BASE_ENVVAR_DICT)
#  If they are not set in the environment they should reamin the default
#  value assigned above in the pattern $<SOME_ENVAR>

# -------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    # srun simple tests?
    test = True

    # happy print
    _LOGGER.info("# {0} #".format('-' * 72))
    _LOGGER.info('~ config_utils.py ... Running script as __main__')
    _LOGGER.info("# {0} #\r".format('-' * 72))

    # print(setEnvarDefaults(), '\r') #<-- not necissary, already called
    # print(BASE_ENVVAR_VALUES, '\r')
    _LOGGER.info('Pretty print: _BASE_ENVVAR_DICT')
    _LOGGER.debug(json.dumps(_BASE_ENVVAR_DICT,
                             indent=4, sort_keys=False,
                             ensure_ascii=False), '\r')

    #  retreive a Path type key from the Box
    foo = _BASE_ENVVAR_DICT[ENVAR_O3DE_DEV]
    _LOGGER.info('~ foo is: {0}'.format(type(foo), foo))

    # simple tests
    _ENV_TAG = 'O3DE_DEV'
    foo = get_envar_default(_ENV_TAG)
    _LOGGER.info("~ Results of getVar on tag, '{0}':'{1}'\r".format(_ENV_TAG, foo))

    envar_value = Validate_Envar(envar=_ENV_TAG)
    _LOGGER.info('~ Repr is: {0}\r'.format(str(repr(envar_value))))
    _LOGGER.info("~ Results of ValidateEnvar(envar='{0}')=='{1}'\r".format(_ENV_TAG, envar_value))

    # custom prompt
    sys.ps1 = "[azpy]>>"
