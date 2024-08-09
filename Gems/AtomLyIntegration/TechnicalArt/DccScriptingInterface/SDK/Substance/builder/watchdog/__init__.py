# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -- This line is 75 characters -------------------------------------------
# The __init__.py files help guide import statements without automatically
#  built-ins
import os
import sys
import site
import time

# -------------------------------------------------------------------------
# we don't have access yet to the DCCsi Lib\site-packages
# (1) this will give us import access to dccsi and azpy import
_PATH_DCCSIG = os.getenv('PATH_DCCSIG', os.getcwd())  # always?, doubtful
# ^^ this assume that the \DccScriptingInterface is the cwd!!! (launch there)
site.addsitedir(_PATH_DCCSIG)

# Lumberyard extensions
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE
from azpy.constants import *

# 3rdparty
from unipath import Path
from watchdog.observers import Observer
from watchdog.events import PatternMatchingEventHandler
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
if _PACKAGENAME == '__main__':
    _PACKAGENAME = 'DCCsi.SDK.substance.builder.watchdog'

import azpy
_LOGGER = azpy.initialize_logger(_PACKAGENAME)
_LOGGER.debug('Starting up:  {0}.'.format({_PACKAGENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# check some env var tags (fail if no, likely means no proper code access)from collections import OrderedDict
from collections import OrderedDict
_SYNTH_ENV_DICT = OrderedDict()
_SYNTH_ENV_DICT = azpy.synthetic_env.stash_env(_SYNTH_ENV_DICT)
_O3DE_DEV = _SYNTH_ENV_DICT[ENVAR_O3DE_DEV]
_PATH_O3DE_PROJECT = _SYNTH_ENV_DICT[ENVAR_PATH_O3DE_PROJECT]


# -------------------------------------------------------------------------
class MyHandler(PatternMatchingEventHandler):
    patterns = ["*.sbsar", "*.txt"]

    def process(self, event):
        """
        event.event_type
            'modified' | 'created' | 'moved' | 'deleted'
        event.is_directory
            True | False
        event.src_path
            path/to/observed/file
        """
        self.outputName = event.src_path.split(".sbsar")[0].split("/")[-1]
        self.outputCookPath = event.src_path.split(self.outputName)
        self.outputRenderPath = Path(_PATH_O3DE_PROJECT, 'Assets', 'Textures', 'Substance').norm()
        _LOGGER.debug(self.outputCookPath, self.outputName, self.outputRenderPath)

        pysbs_batch.sbsrender_info(input=event.src_path)
        pysbs_batch.sbsrender_render(inputs=event.src_path,
                             # inputs=os.path.join(self.outputCookPath, self.outputName + '.sbsar'),
                             # input_graph=_inputGraphPath,
                             output_path=self.outputRenderPath,
                             output_name='{inputGraphUrl}_{outputNodeName}',
                             output_format='tif',
                             # set_value=['$outputsize@%s,%s' % (_outputSize, _outputSize), '$randomseed@1'],
                             # use_preset = _user_preset[0]
                             no_report=True,
                             verbose=True
                                     ).wait()
        # print(self.outputName, self.outputCookPath)
        # builder.output_info(self.outputCookPath, self.outputName)
        # print(builder.output_info(event.src_path.split("/")[0], event.src_path.split("/")[-1].split(".sbsar")[0])['_inputs'])
        # pysbs_batch.sbsrender_render(
        #     input = event.src_path,
        #     output_path = 'C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/Materials/Substance/textures',
        #     output_name = '{inputGraphUrl}_{outputNodeName}',
        #     output_format = 'tif',
        #     # set_value=['$outputsize@9,9', '$randomseed@3']
        #     set_value = '$outputsize@10,10',
        #     use_preset = ['Red Coral', 'Red Sand', 'Grey Stone'][0]
        # ).wait()
        # pysbs_batch.sbsrender_render(
        #     input = event.src_path,
        #     output_path = 'C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/Materials/Substance/textures',
        #     output_name = '{inputGraphUrl}_{outputNodeName}',
        #     output_format = 'tif',
        #     # set_value=['$outputsize@9,9', '$randomseed@3']
        #     set_value = '$outputsize@10,10',
        #     use_preset = 'Red Sand'
        # ).wait()

        # print("Hello World!!!")

    def on_modified(self, event):
        self.process(event)

    def on_created(self, event):
        self.process(event)


if __name__ == '__main__':
    """Run this file as main"""
    _TYPE_TAG = 'MODULE_TEST'

    _TEST_APP_NAME = '{0}-{1}'.format(_TOOL_TAG, _TYPE_TAG)

    _LOGGER = setup_config_logging(_TEST_APP_NAME)
    _LOGGER.debug("Test Run:: {0}.".format({_PACKAGENAME}))
    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_TOOL_TAG))

    args = sys.argv[1:]
    observer = Observer()
    observer.schedule(MyHandler(), path=args[0] if args else '.')
    observer.start()

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()

    observer.join()



