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
# File Description:
# This file is contains OpenImageIO operations for file texture conversions
# -------------------------------------------------------------------------
import config
from PySide2.QtCore import QDataStream, QByteArray, QIODevice, QObject, Signal, Slot
from PySide2.QtNetwork import QTcpSocket
import socket
import time
import traceback
import json
import logging
import socket

_MODULENAME = 'azpy.shared.client_base'
_LOGGER = logging.getLogger(_MODULENAME)


class ClientBase(QObject):
    client_activity_registered = Signal(dict)

    def __init__(self, port=17344):
        super().__init__()
        self.socket = QTcpSocket()
        self.socket.readyRead.connect(self.read_data)
        self.socket.connected.connect(self.connected)
        self.stream = None
        self.port = port

    def establish_connection(self):
        _LOGGER.info(f'Client connecting to port [{self.port}]')
        try:
            self.socket.connectToHost('localhost', self.port)
        except Exception as e:
            _LOGGER.info(f'CONNECTION EXCEPTION [{type(e)}] :::: {e}')
            return False
        return True

    def connected(self):
        self.stream = QDataStream(self.socket)
        self.stream.setVersion(QDataStream.Qt_5_0)

    def disconnect(self):
        try:
            self.socket.close()
            _LOGGER.info('Client disconnected.')
        except ConnectionError:
            return False
        return True

    def send(self, cmd):
        json_cmd = json.dumps(cmd)
        if self.socket.waitForConnected(5000):
            self.stream << json_cmd
        else:
            _LOGGER.info('Connection to the server failed')
        return None

    def read_data(self):
        msg = self.socket.readAll()
        converted = str(msg.data(), encoding='utf-8')
        message = json.loads(converted)

        if self.is_valid_reply(message):
            self.update_event(message)

    def update_event(self, update):
        _LOGGER.info(f'UPDATE EVENT FIRED::::: {update}')
        self.client_activity_registered.emit(update['result'])

    def ping(self):
        cmd = {'cmd': 'ping'}
        reply = self.send(cmd)
        if self.is_valid_reply(reply):
            return True
        else:
            return False

    @staticmethod
    def is_valid_reply(reply):
        _LOGGER.info(f'Is Valid Reply: {reply}')
        if not reply:
            _LOGGER.info('[ERROR] Invalid Reply')
            return False

        if not reply['success']:
            _LOGGER.info(f"[ERROR] {reply['cmd']} failed: {reply['msg']}")
            return
        return True
