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
# import logging as _logging

# -- External Python modules --
from PySide2 import QtWidgets

shared_path = os.path.join(os.environ['DCCSIG_PATH'], 'azpy', 'shared')
from azpy.shared import server_base

# -- Extension Modules --
# -

# --------------------------------------------------------------------------
# -- Global Definitions --
# _MODULENAME = 'azpy.maya.utils.maya_server'
# _LOGGER = _logging.getLogger(_MODULENAME)

# _LOCAL_HOST = socket.gethostbyname(socket.gethostname())
# _LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class MayaServer(server_base.ServerBase):
    PORT = 17337

    def __init__(self, parent_window):
        super(MayaServer, self).__init__(parent_window)

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
            print('Sleeping::: {}'.format(i))
            # _LOGGER.info(f'Sleeping::: {i}')
            time.sleep(1)

        reply['result'] = True
        reply['success'] = True


def start_server():
    print('Starting server')
    app = QtWidgets.QApplication(sys.argv)

    window = QtWidgets.QDialog()
    window.setWindowTitle('Maya Server')
    window.setFixedSize(240, 150)

    QtWidgets.QPlainTextEdit(window)
    MayaServer(window)
    window.show()
    app.exec_()


