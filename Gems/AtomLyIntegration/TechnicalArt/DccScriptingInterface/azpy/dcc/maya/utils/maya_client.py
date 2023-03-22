# -*- coding: utf-8 -*-
# !/usr/bin/python
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

import logging
from azpy.shared.client_base import ClientBase
from PySide2 import QtNetwork
from PySide2.QtCore import Signal, QDataStream
import time
import json


_MODULENAME = 'azpy.dcc.maya.utils.maya_client'
_LOGGER = logging.getLogger(_MODULENAME)


class MayaClient(ClientBase):
    client_activity_registered = Signal(dict)

    def __init__(self, port):
        super(MayaClient, self).__init__(port)

        # LiveLink Socket
        self.broadcast_server = None
        self.broadcast_socket = None
        self.broadcast_stream = None
        self.broadcast_port = 17350
        self.initialize()

    def echo(self, text):
        cmd = {
            'cmd': 'echo',
            'text': text
        }
        self.send(cmd)

    def verify_connection(self):
        cmd = {
            'cmd': 'verify_connection',
        }
        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return 'connected'

    def run_command(self, command):
        cmd = {
            'cmd': 'run_command',
            'arguments': command
        }

        self.send(cmd)

    def run_script(self, target_path, script_arguments=None):
        cmd = {
            'cmd':   'run_script',
            'path': target_path,
            'arguments': script_arguments
        }

        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return reply['result']
        else:
            return None

    def update_event(self, update):
        self.client_activity_registered.emit(update)

    # +++++++++++++++++++++++++++--->>
    # LiveLink Broadcast Server +----->>
    # +++++++++++++++++++++++++++--->>

    def initialize(self):
        self.broadcast_server = QtNetwork.QTcpServer(self)
        self.broadcast_server.newConnection.connect(self.establish_broadcast_connection)

        if self.listen():
            _LOGGER.info(f'Broadcast Server listening on port: {self.broadcast_port}')
        else:
            _LOGGER.info(f'Server initialization failed')

    def listen(self):
        if not self.broadcast_server.isListening():
            return self.broadcast_server.listen(QtNetwork.QHostAddress.LocalHost, self.broadcast_port)
        else:
            _LOGGER.info(f'Confirmation: {self.broadcast_server.isListening()}')

    def establish_broadcast_connection(self):
        self.broadcast_socket = self.broadcast_server.nextPendingConnection()
        self.broadcast_socket.nextBlockSize = 0
        if self.broadcast_socket.state() == QtNetwork.QTcpSocket.ConnectedState:
            self.broadcast_socket.disconnected.connect(self.on_disconnected)
            self.broadcast_socket.readyRead.connect(self.read)

    def on_disconnected(self):
        self.broadcast_socket.disconnected.disconnect()
        self.broadcast_socket.readyRead.disconnect()
        self.broadcast_socket.deleteLater()

    def read(self):
        self.broadcast_stream = QDataStream(self.broadcast_socket)
        self.broadcast_stream.setVersion(QDataStream.Qt_5_0)
        while True:
            if self.broadcast_stream.atEnd():
                break
            message = self.broadcast_stream.readQString()
            data = json.loads(message)
            self.update_event(data)
