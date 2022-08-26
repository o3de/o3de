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
# session to allow commands to be sent directly into Maya. The most likely use would be to launch a scene file and
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

import config
import sys
import time
import logging as _logging

from PySide2 import QtWidgets
from azpy.shared.server_base import ServerBase


_MODULENAME = 'azpy.dcc.maya.utils.maya_server'
_LOGGER = _logging.getLogger(_MODULENAME)


class MayaServer(ServerBase):
    PORT = 17337

    def __init__(self, parent_window):
        super(MayaServer, self).__init__(parent_window)

        self.window = parent_window

    def process_cmd(self, cmd, data, reply):
        """! Extends command capabilities for DCC applications. The run script command is likely the most useful
        of these commands beyond testing and verifying connections. Any commands that are unrecognized return errors
        in the reply
        """
        if cmd == 'echo':
            self.echo(data, reply)
        elif cmd == 'run_script':
            self.run_script(data, reply)
        elif cmd == 'set_title':
            self.set_title(data, reply)
        elif cmd == 'sleep':
            self.sleep(data, reply)
        else:
            super(MayaServer, self).process_cmd(cmd, data, reply)

    def echo(self, data, reply):
        """! Tests communication channel """
        reply['result'] = data['text']
        reply['success'] = True

    def run_script(self, data, reply):
        """! Fires commands and/or python scripts to open scenes """
        reply['result'] = data['text']
        reply['success'] = True

    def set_title(self, data, reply):
        """! Tests QT window modifications """
        self.window.setWindowTitle(data['title'])
        reply['result'] = True
        reply['success'] = True

    def sleep(self, data, reply):
        """! Tests communication channel """
        for i in range(6):
            _LOGGER.info(f'Sleeping: {i}')
            time.sleep(1)

        reply['result'] = True
        reply['success'] = True


def launch():
    _LOGGER.info('MayaServer firing')
    app = QtWidgets.QApplication(sys.argv)

    window = QtWidgets.QDialog()
    window.setWindowTitle('Maya Server')
    window.setFixedSize(240, 150)

    QtWidgets.QPlainTextEdit(window)
    MayaServer(window)
    window.show()
    app.exec_()

