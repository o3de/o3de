# coding:utf-8
#!/usr/bin/python
#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# -- This line is 75 characters -------------------------------------------
# -- Standard Python modules --
import sys
import os
import socket
import time
import logging as _logging

# -- External Python modules --
from PySide2 import QtWidgets
from shiboken2 import wrapInstance
from maya import OpenMayaUI as omui

shared_path = os.path.join(os.environ['DCCSIG_PATH'], 'azpy', 'shared')
from azpy.shared.server_base import ServerBase

# -- Extension Modules --
# -

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.maya.utils.maya_server'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


# -------------------------------------------------------------------------
class MayaServer(ServerBase):
    PORT = 17337

    def __init__(self, parent_window):
        super(MayaServer, self).__init__(parent_window)

        self.setParent(mayaMainWindow)
        self.window = parent_window

    def process_cmd(self, cmd, data, reply):
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
        reply['result'] = data['text']
        reply['success'] = True

    def run_script(self, data, reply):
        reply['result'] = data['text']
        reply['success'] = True

    def set_title(self, data, reply):
        self.window.setWindowTitle(data['title'])
        reply['result'] = True
        reply['success'] = True

    def sleep(self, data, reply):
        for i in range(6):
            _LOGGER.info('Sleeping::: {}'.format(i))
            time.sleep(1)

        reply['result'] = True
        reply['success'] = True


def delete_instances():
    '''
    Finds all previously opened instances of the transfer plugin and deletes them before creating a new instance
    '''
    for obj in mayaMainWindow.children():
        if str(type(obj)) == "<class 'maya_server.MayaServer'>":
            if obj.__class__.__name__ == "MayaServer":
                obj.setParent(None)
                obj.deleteLater()


def start_server():
    delete_instances()
    window = QtWidgets.QDialog()
    window.setWindowTitle('Maya Server')
    window.setFixedSize(240, 150)
    window.move(50, 50)
    window_layout = QtWidgets.QHBoxLayout(window)
    window_layout.setContentsMargins(3, 3, 3, 3)
    text_field = QtWidgets.QPlainTextEdit()
    window_layout.addWidget(text_field)
    MayaServer(window)
    window.show()
