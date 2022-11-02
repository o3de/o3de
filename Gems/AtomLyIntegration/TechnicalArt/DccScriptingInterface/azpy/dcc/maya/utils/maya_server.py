# # coding:utf-8
# #!/usr/bin/python
# #
# # All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# # its licensors.
# #
# # For complete copyright and license terms please see the LICENSE at the root of this
# # distribution (the "License"). All use of this software is governed by the License,
# # or, if provided, by the license below or the license accompanying this file. Do not
# # remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# #
# # -- This line is 75 characters -------------------------------------------
#

"""! @brief This module is the main server for handling communications/commands between the DCCsi and Maya """

##
# @file maya_server.py
#
# @brief Socket communication between DCCsi tools and DCCsi supported applications (such as Maya and Blender) can be
# extended using the following files:
# - azpy/shared/client_base.py
# - azpy/shared/server_base.py
#
# These will be expanded to enable standalone QT Controllers to operate between the O3DE python distribution and DCC
# packages and APIs, which will be launched most of the time as a detached QProcess
#
# @section Maya Server Description
# The Maya Server can be launched alongside launching a bootstrapped Maya UI session (as opposed to a Maya Standalone
# session to allow commands to be sent directly into Maya). The most likely use would be to launch a scene file and
# then run one or more utility scripts on these files.
#
# In order for Maya communication to be possible, you must add
# snippet below to your userSetup.py file in your:
#
# Documents/maya/scripts
#
# directory This will enable commandPort communication for all
# versions of Maya. Active port is established by combining
# Maya version number with either a 0 for MEL communication or
# 1 for Python - ex. 20231 for Maya 2023/Python. Use below snippet:
#
# import maya.cmds as mc
#
# version = mc.about(version=True)
# if not mc.about(batch=True):
#     print(f'Command Port set: {version}1')
#     mc.commandPort(name=f':{version}0', sourceType='mel')
#     mc.commandPort(name=f':{version}1', sourceType='python')
#
# @section Launcher Notes
# - Comments are Doxygen compatible


import sys
import logging as _logging
import importlib.util
from pathlib import Path
from importlib import import_module
from PySide2 import QtWidgets, QtCore
from PySide2.QtCore import Signal, Slot
from azpy.shared.server_base import ServerBase
from shiboken2 import wrapInstance
from maya import OpenMayaUI as omui
import os


_MODULENAME = 'azpy.dcc.maya.utils.maya_server'
_LOGGER = _logging.getLogger(_MODULENAME)


mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(int(mayaMainWindowPtr), QtWidgets.QWidget)


class MayaServer(ServerBase):
    def __init__(self):
        super(MayaServer, self).__init__()

        self.setParent(mayaMainWindow)
        self.setWindowFlags(QtCore.Qt.Window)
        self.setObjectName('MayaServer')
        self.setWindowTitle('Maya Server')
        self.setGeometry(50, 50, 240, 150)
        self.container = QtWidgets.QVBoxLayout(self)
        self.window = QtWidgets.QPlainTextEdit()
        self.container.addWidget(self.window)
        self.cls = None
        self.show()

    def process_cmd(self, cmd, data, reply):
        """! Extends command capabilities for DCC applications. The run script command is likely the most useful
        of these commands beyond testing and verifying connections. Any commands that are unrecognized return errors
        in the reply
        """
        _LOGGER.info(f'Process command FIRED: cmd:: {cmd}   data:: {data}   reply:: {reply}')
        if cmd == 'echo':
            self.echo(data, reply)
        elif cmd == 'run_script':
            self.run_script(data, reply)
        elif cmd == 'set_title':
            self.set_title(data, reply)
        else:
            super(MayaServer, self).process_cmd(cmd, data, reply)

    def echo(self, data, reply):
        """! Tests communication channel """
        reply['result'] = data['text']
        reply['success'] = True

    def run_script(self, data, reply):
        """! Fire commands and/or python scripts to open scenes """
        _LOGGER.info(f'Run Script DATA: {data}')
        target_module = self.get_module_path(Path(data['path']))
        module_name = f"{Path(data['path']).parts[-2]}.{Path(data['path']).stem}"
        processing_data = None

        # Import module if it hasn't been done already
        if target_module not in sys.modules:
            self.handle_script_configuration(module_name, data)

        if 'class' in data['arguments']:
            target_class = getattr(import_module(module_name), data['arguments']['class'])
            cls = target_class(**data['arguments'])
            processing_data = cls.get_script_data()
        else:
            target_module.run(data['arguments'])
            processing_data = data['path']

        reply['result'] = processing_data
        reply['success'] = True

    def handle_script_configuration(self, module_name, data):
        spec = importlib.util.spec_from_file_location(module_name, Path(data['path']).as_posix())
        script = importlib.util.module_from_spec(spec)
        sys.modules[module_name] = script
        spec.loader.exec_module(script)

    def set_title(self, data, reply):
        """! Tests QT window modifications """
        self.setWindowTitle(data['title'])
        reply['result'] = True
        reply['success'] = True

    def get_module_path(self, script_path):
        path_list = list(script_path.parts)
        start_index = path_list.index('DccScriptingInterface')
        if start_index != -1:
            _LOGGER.info(type(path_list))
            path_list[-1] = script_path.stem
            return '.'.join(path_list[start_index+1:])
        return None

    @Slot(object)
    def return_scene_data(self, data):
        _LOGGER.info(f'%%%%%% Return Scene Data Slot Fired %%%%%%\n{data}')


def launch():
    _LOGGER.info('Starting Maya Communication Server...')
    MayaServer()

