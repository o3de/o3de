# coding:utf-8
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

"""! @brief The main server for handling communications/commands between the DCCsi and DCC Applications """

##
# @file server_base.py
#
# @brief The main server supporting socket communication between DCCsi tools and DCCsi supported applications
# (such as Maya and Blender).
#
# These will be expanded to enable standalone QT Controllers to operate between the O3DE python distribution and DCC
# packages and APIs, which will be launched most of the time as a detached QProcess
#
# @section ServerBase Description
# The ServerBase module is meant to provide basic functions of socket communication system, with the likelihood to
# be extended per the requirements of respective applications supported by DCCsi tools and workflows (Maya, Blender,
# Houdini, 3dsMax)


from PySide2 import QtCore, QtNetwork, QtWidgets
from PySide2.QtCore import QDataStream, Signal, Slot
from PySide2.QtNetwork import QTcpServer
import logging
import json
import sys


_MODULENAME = 'azpy.shared.server_base'
_LOGGER = logging.getLogger(_MODULENAME)
SIZEOF_UINT32 = 4


class ServerBase(QtWidgets.QWidget):

    def __init__(self, port):
        super(ServerBase, self).__init__()
        self.server = None
        self.socket = None
        self.stream = None
        self.port = port
        self.initialize()

    def initialize(self):
        self.server = QtNetwork.QTcpServer(self)
        self.server.newConnection.connect(self.establish_connection)

        if self.listen():
            _LOGGER.info(f'Server listening on port: {self.port}')
        else:
            _LOGGER.info(f'Server initialization failed')

    def listen(self):
        if not self.server.isListening():
            return self.server.listen(QtNetwork.QHostAddress.LocalHost, self.port)
        else:
            _LOGGER.info(f'Confirmation: {self.server.isListening()}')

    def establish_connection(self):
        self.socket = self.server.nextPendingConnection()
        self.socket.nextBlockSize = 0
        if self.socket.state() == QtNetwork.QTcpSocket.ConnectedState:
            self.socket.disconnected.connect(self.on_disconnected)
            self.socket.readyRead.connect(self.read)
            _LOGGER.info('Connection Established')

    def on_disconnected(self):
        self.socket.disconnected.disconnect()
        self.socket.readyRead.disconnect()
        self.socket.deleteLater()
        _LOGGER.info('Connection Disconnected')

    def read(self):
        self.stream = QDataStream(self.socket)
        self.stream.setVersion(QDataStream.Qt_5_0)
        while True:
            if self.stream.atEnd():
                break
            message = self.stream.readQString()
            data = json.loads(message)
            self.process_data(data)

    def write(self, reply):
        json_reply = json.dumps(reply)
        if self.socket.state() == QtNetwork.QTcpSocket.ConnectedState:
            # _LOGGER.info(f'SendingInfo: {json_reply}')
            data = QtCore.QByteArray(json_reply.encode())
            self.socket.write(data)
        else:
            _LOGGER.info('Connection to the server failed')

    def write_error(self, error_msg):
        reply = {
            'success': False,
            'msg': error_msg,
            'command': 'unknown'
        }
        self.write(reply)

    def process_data(self, data):
        reply = {
            'success': False
        }

        cmd = data['cmd']
        if cmd == 'ping':
            reply['success'] = True
        else:
            self.process_cmd(cmd, data, reply)

            # TODO - What is above showing up false?

            if not reply['success']:
                reply['cmd'] = cmd
                if not 'msg' in reply.keys():
                    reply['msg'] = 'Unknown Error'
        self.write(reply)

    def process_cmd(self, cmd, data, reply):
        _LOGGER.info('Process command firing in ServerBase')
        reply['msg'] = f'Invalid command: {cmd}'


if __name__ == '__main__':
    _LOGGER.info('Socket Communication- ServerBase started...')
    if not QtWidgets.QApplication.instance():
        app = QtWidgets.QApplication(sys.argv)
    else:
        app = QtWidgets.QApplication.instance()

    window = QtWidgets.QDialog()
    window.setWindowTitle("Server Base")
    window.setFixedSize(240, 150)
    QtWidgets.QPlainTextEdit(window)
    server = ServerBase(window)
    window.show()

    app.exec_()
