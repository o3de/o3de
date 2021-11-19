# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -- This line is 75 characters -------------------------------------------
"""Empty Doc String"""  # To Do: add documentation
# -------------------------------------------------------------------------
#  built-ins
import os
import sys
import site
import subprocess
import logging

# Lumberyard extensions
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import *

#  3rdparty
from unipath import Path
import click
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# substance automation toolkit (aka pysbs)
# To Do: manage with dynaconf environment
_PYSBS_DIR_PATH = Path(PATH_PROGRAMFILES_X64,
                       'Allegorithmic',
                       'Substance Automation Toolkit',
                       'Python API',
                       'install').resolve()

site.addsitedir(str(_PYSBS_DIR_PATH))  # 'install' is the folder I created

# Susbstance
import pysbs.batchtools as pysbs_batch
import pysbs.context as pysbs_context
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up global space, logging etc.
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, False)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, False)

_PACKAGENAME = __name__
if _PACKAGENAME is '__main__':
    _PACKAGENAME = 'DCCsi.SDK.substance.builder.sbsar_info'

import azpy
_LOGGER = azpy.initialize_logger(_PACKAGENAME)
_LOGGER.debug('Starting up:  {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global space debug flag
_DCCSI_GDEBUG = os.getenv(ENVAR_DCCSI_GDEBUG, False)

#  global space debug flag
_DCCSI_DEV_MODE = os.getenv(ENVAR_DCCSI_DEV_MODE, False)

_MODULE_PATH = Path(__file__)

_ORG_TAG = 'Amazon_Lumberyard'
_APP_TAG = 'DCCsi'
_TOOL_TAG = 'sdk.substance.builder.sbsar_info'
_TYPE_TAG = 'module'

_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = _TOOL_TAG
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Defining CONSTANTS
# To Do: shouldn't need this _BASE_ENVVAR_DICT (replace with dynaconf config)
from collections import OrderedDict
_SYNTH_ENV_DICT = OrderedDict()
_SYNTH_ENV_DICT = azpy.synthetic_env.stash_env(_SYNTH_ENV_DICT)
# grab a specific path from the base_env
_PATH_DCCSI = _SYNTH_ENV_DICT[ENVAR_DCCSIG_PATH]
_O3DE_PROJECT_PATH = _SYNTH_ENV_DICT[ENVAR_O3DE_PROJECT_PATH]

# build some reuseable path parts
_PATH_MOCK_ASSETS = Path(_O3DE_PROJECT_PATH, 'Assets').norm()
_PATH_MOCK_SUBLIB = Path(_PATH_MOCK_ASSETS, 'SubstanceSource').norm()

_PATH_MOCK_SBS = Path(_PATH_MOCK_SUBLIB, 'sbs').norm()
_PATH_MOCK_SBSAR = Path(_PATH_MOCK_SUBLIB, 'sbsar').norm()

_PATH_MOCK_MAT = Path(_PATH_MOCK_ASSETS, 'Textures').norm()
_PATH_MOCK_MAT_SUB = Path(_PATH_MOCK_MAT, 'Substance').norm()

# this will combine two parts into a single path (object)
# It also returnd the fixed-up version (norm)
_PATH_INPUT_SBS = Path(_PATH_MOCK_SBS, 'bronze_yellow.sbs').norm()
_PATH_COOK_OUTPUT = _PATH_MOCK_SBSAR.norm()
_PATH_RENDER_OUTPUT = _PATH_MOCK_MAT_SUB


# quick test variables, will be removed
_PYSBS_CONTEXT = pysbs_context.Context()
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def output_info(_outputCookPath, _outputName):
    """ SBSAR information"""
    info_lists = []
    input_n_output = pysbs_batch.sbsrender_info(input=os.path.join(_outputCookPath, _outputName + '.sbsar'),
                                        stdout=subprocess.PIPE)
    for info_list in input_n_output.stdout.read().splitlines():
        info_lists.append(info_list.decode('utf-8'))

    _outputs, _params, _presets, _inputs, _input, _input_type = [], [], [], [], [], []

    for info_list in info_lists:
        if 'OUTPUT' in info_list:
            _outputs.append(info_list.split(' ')[3])
        elif 'INPUT $' in info_list:
            _params.append(info_list.split(' ')[3:])
        elif 'PRESET' in info_list:
            _presets.append(info_list.split('PRESET ')[1])
        elif 'INPUT' in info_list and (not '$'in info_list):
            _inputs.append(info_list.split(' ')[3:])
            _input.append(info_list.split(' ')[3:][0])
            _input_type.append(info_list.split(' ')[3:][1])

    info_list = {'texmaps': _outputs,
                 'params': _params,
                 'presets': _presets,
                 'inputs': _inputs,
                 'input': _input,
                 'input_type': _input_type}

    return info_list
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
@click.command()
@click.option('--sbsar_path', default=_PATH_COOK_OUTPUT, help='Sbsar directory.')
@click.option('--sbsar_name', default=_PATH_INPUT_SBS.stem, help='Sbsar name.')
@click.option('--output_type', default='inputs', help='Output Type: \n'
              'texmaps | presets | inputs | input | input_type | params')
def sbsar_info(sbsar_path, sbsar_name, output_type):
    click.echo(output_info(sbsar_path, sbsar_name)[output_type])


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_PACKAGENAME))
    _LOGGER.debug("Test Run:: {0}.".format({_MODULENAME}))
    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_TOOL_TAG))

    _LOGGER.debug(sbsar_info())

    # remove the logger
    del _LOGGER
# ---- END ---------------------------------------------------------------

