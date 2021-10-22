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

# Lumberyard extensions
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import *

#  3rdparty (we provide)
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
    _PACKAGENAME = 'DCCsi.SDK.substance.builder.sbsar_render'

import azpy
_LOGGER = azpy.initialize_logger(_PACKAGENAME)
_LOGGER.debug('Starting up:  {0}.'.format({_PACKAGENAME}))
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
_PATH_INPUT_SBS = Path(_PATH_MOCK_SBS, 'alien_rock_coral_formation','alien_rock_coral_formation.sbs').norm()
_PATH_COOK_OUTPUT = _PATH_MOCK_SBSAR.norm()
_PATH_RENDER_OUTPUT = _PATH_MOCK_MAT_SUB

# quick test variables, will be removed
_OUTPUT_SIZE = 9
_user_preset = ['Red Coral', 'Red Sand', 'Grey Stone']
integer1 = 0
integer2 = [0, 0]
float1 = 0.0
float2 = [0.0, 0.0]

_PYSBS_CONTEXT = pysbs_context.Context()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
@click.command()
@click.option('--sbsar_path', default=_PATH_COOK_OUTPUT, help='Sbsar directory.')
@click.option('--sbsar_name', default=_PATH_INPUT_SBS.stem, help='Sbsar name.')
@click.option('--tex_path', default=_PATH_RENDER_OUTPUT, help='Texture output path.')
@click.option('--output_size', default=9, help='512x512, 9 | 1024x1024, 10 | 2048x2048, 11')
def sbsar_render(sbsar_path, sbsar_name, tex_path, output_size):
    """ Doc String"""
    pysbs_batch.sbsrender_render(inputs=os.path.join(sbsar_path, sbsar_name + '.sbsar'),
                         # input_graph=_inputGraphPath,
                         output_path=tex_path,
                         output_name='{inputGraphUrl}_{outputNodeName}',
                         output_format='tif',
                         set_value=['$outputsize@%s,%s' % (output_size, output_size), '$randomseed@1'],
                         # use_preset = _user_preset[0]
                         no_report=True,
                         verbose=True
                                 ).wait()
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_PACKAGENAME))

    _LOGGER.debug(sbsar_render())

    # remove the logger
    del _LOGGER
# ---- END ---------------------------------------------------------------

