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
import logging as _logging
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# set up global space, logging etc.
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# we boostrap access to some lib site-packages
from dynaconf import settings
from pathlib import Path

_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, settings.DCCSI_GDEBUG)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, settings.DCCSI_DEV_MODE)

_MODULENAME = 'DCCsi.SDK.substance.builder.sbsar_utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Starting up:  {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# substance automation toolkit (aka pysbs)
# To Do: manage with dynaconf environment
from azpy.constants import PATH_SAT_INSTALL_PATH
_SAT_INSTALL_PATH = Path(PATH_SAT_INSTALL_PATH).resolve()
site.addsitedir(str(_SAT_INSTALL_PATH))  # 'install' is the folder I created

# Susbstance
import pysbs.batchtools as pysbs_batch
import pysbs.context as _pysbs_context

_PYSBS_CONTEXT = _pysbs_context.Context()
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
def cook_sbsar(input_sbs, cook_output_path):
    """ Doc String"""
    input_sbs = Path(input_sbs).resolve()
    cook_output_path = Path(cook_output_path).resolve()
    if not cook_output_path.exists():
        try:
            cook_output_path.mkdir()
        except:
            _LOGGER.warning('Could not mkdir: {}'.format(cook_output_path))
    output_name = input_sbs.stem
    pysbs_batch.sbscooker(quiet=True,
                          inputs=str(input_sbs),
                          includes=_PYSBS_CONTEXT.getDefaultPackagePath(),
                          alias=_PYSBS_CONTEXT.getUrlAliasMgr().getAllAliases(),
                          output_path=str(cook_output_path),
                          output_name=output_name,
                          compression_mode=2).wait()
    new_file = Path(cook_output_path, output_name + '.sbsar').resolve()
    if new_file.exists():
        return new_file
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def info_sbsar(input_path):
    """ Doc String"""
    sbsar_info = []
    in_file = Path(input_path).resolve()
    input_n_output = pysbs_batch.sbsrender_info(input=str(in_file),
                                                  stdout=subprocess.PIPE)

    for info in input_n_output.stdout.read().splitlines():
        sbsar_info.append(info.decode('utf-8'))
        
    return sbsar_info
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def output_info(cook_output_path, output_name, output_type):
    """ Doc String"""
    info_lists = []
    input_file = Path(cook_output_path, output_name + '.sbsar').resolve()
    input_n_output = pysbs_batch.sbsrender_info(input=str(input_file),
                                                 stdout=subprocess.PIPE)
    
    for info_list in input_n_output.stdout.read().splitlines():
        info_lists.append(info_list.decode('utf-8'))

    tex_maps, params, output_size, presets, inputs = [], [], [], [], []

    for info_list in info_lists:
        if 'OUTPUT' in info_list:
            tex_maps.append(info_list.split(' ')[3])
        elif 'INPUT $' in info_list:
            params.append(info_list.split(' ')[3:])
        elif 'PRESET' in info_list:
            presets.append(info_list.split('PRESET ')[1])
        elif 'INPUT' in info_list and not '$' in info_list:
            inputs.append(info_list.split(' ')[3:])

    output_info = {'tex_maps': tex_maps,
                    'params': params,
                    'output_size': output_size,
                    'presets': presets,
                    'inputs': inputs}
    return output_info[output_type]
# To Do: Make preset a optional argument.
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
def render_sbsar(cook_output_path, output_texture_type, sbsar_name,
                 render_to_output_path, use_preset, output_size, random_seed):
    """ Render textures maps from SBSAR"""
    # _tex_output = ['diffuse', 'basecolor', 'normal']
    # print(_tex_output)
    cook_output_path = Path(cook_output_path).resolve()
    texture_path = Path(render_to_output_path).resolve()
    sbsar_file = Path(cook_output_path, sbsar_name + '.sbsar').resolve()
    
    if output_info(cook_output_path, sbsar_name, 'presets'):
        if use_preset == -1:
            preset_base = '_'
            pysbs_batch.sbsrender_render(inputs=str(sbsar_file),
                                         # input_graph=_inputGraphPath,
                                         input_graph_output=str(output_texture_type),
                                         output_path=str(texture_path),
                                         output_name=sbsar_name + preset_base + '{outputNodeName}',
                                         output_format='tif',
                                         set_value=['$outputsize@{},{}'.format(output_size, output_size),
                                                    '$randomseed@{}'.format(random_seed)],
                                         no_report=True,
                                         verbose=True
                                         ).wait()
        else:
            preset_name = output_info(cook_output_path, sbsar_name, 'presets')[use_preset]
            preset_base = '_'
            pysbs_batch.sbsrender_render(inputs=str(sbsar_file),
                                         # input_graph=_inputGraphPath,
                                         input_graph_output=str(output_texture_type),
                                         output_path=str(texture_path),
                                         output_name=sbsar_name + preset_base + preset_name.replace(' ', '') + '_{outputNodeName}',
                                         output_format='tif',
                                         set_value=['$outputsize@{},{}'.format(output_size, output_size),
                                            '$randomseed@{}'.format(random_seed)],
                                         use_preset=preset_name,
                                         no_report=True,
                                         verbose=True
                                         ).wait()
    else:
        pysbs_batch.sbsrender_render(inputs=str(sbsar_file),
                                     # input_graph=_inputGraphPath,
                                     input_graph_output=str(output_texture_type),
                                     output_path=str(texture_path),
                                     output_name=sbsar_name + '_{outputNodeName}',
                                     output_format='tif',
                                     set_value=['$outputsize@{},{}'.format(output_size, output_size),
                                        '$randomseed@{}'.format(random_seed)],
                                     no_report=True,
                                     verbose=True
                                     ).wait()
# --------------------------------------------------------------------------


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == "__main__":
    """Run this file as main"""
    # ---------------------------------------------------------------------
    # Defining CONSTANTS
    # To Do: shouldn't need this _SYNTH_ENV_DICT (replace with dynaconf config)
    from azpy import synthetic_env
    _SYNTH_ENV_DICT = synthetic_env.stash_env()

    from azpy.constants import ENVAR_DCCSIG_PATH
    from azpy.constants import ENVAR_O3DE_PROJECT_PATH
    
    # grab a specific path from the base_env
    _PATH_DCCSI = _SYNTH_ENV_DICT[ENVAR_DCCSIG_PATH]

    # use DCCsi as the project path for this test
    _O3DE_PROJECT_PATH = _PATH_DCCSI
    
    _PROJECT_ASSETS_PATH = Path(_O3DE_PROJECT_PATH, 'Assets').resolve()
    _PROJECT_MATERIALS_PATH = Path(_PROJECT_ASSETS_PATH, 'Materials').resolve()
    
    # this will combine two parts into a single path (object)
    # It also returnd the fixed-up version (norm)
    _PATH_OUTPUT = Path(_PROJECT_MATERIALS_PATH, 'Fabric')
    _PATH_INPUT_SBS = Path(_PROJECT_MATERIALS_PATH, 'Fabric', 'fabric.sbsar')
    # ---------------------------------------------------------------------
    
    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_MODULENAME))
    
    _LOGGER.debug('_SYNTH_ENV_DICT: {}'.format(_SYNTH_ENV_DICT))

    _LOGGER.info('presets: {}'.format(output_info(_PATH_OUTPUT,
                                                  _PATH_INPUT_SBS.stem,
                                                  'presets')))

    _LOGGER.info('params: {}'.format(output_info(_PATH_OUTPUT,
                                                 _PATH_INPUT_SBS.stem,
                                                 'params')))

    _LOGGER.info('inputs: {}'.format(output_info(_PATH_OUTPUT,
                                                 _PATH_INPUT_SBS.stem,
                                                 'inputs')))
    
    _LOGGER.info('tex_maps: {}'.format(output_info(_PATH_OUTPUT,
                                                   _PATH_INPUT_SBS.stem,
                                                   'tex_maps')))
    
    _LOGGER.info(info_sbsar(Path(_PATH_INPUT_SBS)))
    new_file = cook_sbsar(_PATH_INPUT_SBS, Path(_PATH_OUTPUT, '.tests'))
    if new_file:
        _LOGGER.info('Cooked out: {}'.format(new_file))
        render_sbsar(cook_output_path=Path(_PATH_OUTPUT, '.tests'),
                     output_texture_type='basecolor',
                     sbsar_name=_PATH_INPUT_SBS.stem,
                     render_to_output_path=Path(_PATH_OUTPUT, '.tests'),
                     use_preset=-1,
                     output_size=256,
                     random_seed=1001)

    # remove the logger
    del _LOGGER
# ---- END ---------------------------------------------------------------

