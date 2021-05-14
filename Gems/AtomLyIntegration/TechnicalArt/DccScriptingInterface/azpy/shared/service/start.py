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
# This is the main entry point of the Legacy Material Conversion scripts. It provides
# UI for the script as well as coordinating many of its core processes. The purpose
# of the conversion scripts is to provide a way for individuals and game teams to
# convert assets and materials previously built for legacy spec/gloss based implementation
# to the current PBR metal rough formatting, with the new '.material' files
# replacing the previous '.mtl' descriptions. The script also creates Maya working files
# with the FBX files present and adds Stingray materials for preview as well as further
# look development
# -------------------------------------------------------------------------

# -- Standard Python modules --
import logging as _logging
import socket
import json
import os

# 3rd Party
from PySide2 import QtCore
from PySide2.QtCore import Signal, Slot, QThread, QProcess, QProcessEnvironment
from pathlib import Path

# azpy bootstrapping and extensions
import constants

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'azpy.shared.service.start'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
_LOGGER.info('local_host: {}'.format(_LOCAL_HOST))
# -------------------------------------------------------------------------

# Start thread
# Start Application QProcess
# Start Initialized Open Application QProcess
# Environment variable system
# Output stream handling
# Create environment
# Create Load Progress System
# Create worker thread system

for key, value in os.environ.items():
    _LOGGER.info(f'Key: {key}   Value: {value}')


class ApplicationLauncher(QtCore.QObject):
    def __init__(self, launch_data, parent=None):
        super(ApplicationLauncher, self).__init__(parent)

        self.launch_data = launch_data
        self.application = self.launch_data['application']
        self.launcher = QProcess(self)
        self.env = None

    def initialize_launcher(self):
        self.set_environment_variables(self.application)
        self.launcher.setProcessEnvironment(self.env)
        self.launcher.setProgram(constants.DCC_LOCATIONS[self.application])
        self.launcher.setArguments(self.launch_data['arguments'])
        _LOGGER.info(f'Launching {self.application.capitalize()}...')

    def launch_application(self):
        self.launcher.startDetached()
        self.launcher.waitForFinished(-1)

    def launch_maya_standalone(self):
        self.launcher.setProcessChannelMode(QProcess.SeparateChannels)
        self.launcher.readyReadStandardOutput.connect(self.handle_stdout)
        self.launcher.readyReadStandardError.connect(self.handle_stderr)
        self.launcher.stateChanged.connect(self.handle_state)
        self.launcher.started.connect(self.process_started)
        self.launcher.finished.connect(self.process_complete)

    def handle_stderr(self):
        data = self.launcher.readAllStandardError()
        stderr = bytes(data).decode("utf8")
        _LOGGER.error(f'Process STD_ERROR: {stderr}')

    def handle_stdout(self):
        """
        This catches standard output from Maya while processing. It is used for keeping track of progress, and once
        the last FBX file in a target directory is processed, it updates the database with the newly created Maya files.
        :return:
        """
        data = self.p.readAllStandardOutput()
        stdout = bytes(data).decode("utf8")
        if stdout.startswith('{'):
            db_updates = json.loads(stdout)
            self.maya_processing_complete(db_updates)
        else:
            self.progress_event_fired(stdout)

    def handle_state(self, state):
        """
        Mainly just for an indication in the Logging statements as to whether or not the QProcess is running
        :param state:
        :return:
        """
        states = {
            QProcess.NotRunning: 'Not running',
            QProcess.Starting:   'Starting',
            QProcess.Running:    'Running'
        }
        state_name = states[state]
        _LOGGER.info(f'QProcess State Change: {state_name}')

    def get_environment_variables(self):
        if self.application == 'maya':
            return constants.MAYA_ENV.items()
        return None

    def set_environment_variables(self):
        self.env = QtCore.QProcessEnvironment.systemEnvironment()
        env_variables = self.get_environment_variables()
        for env_name, env_value in env_variables.items():
            self.env.insert(env_name, env_value)

    def process_started(self):
        _LOGGER.info(f'Connection Successful: {self.application} Process Started...')

    def process_complete(self):
        self.launcher.closeWriteChannel()
        _LOGGER.info("Process Complete.")

def initialize_maya_standalone_qprocess(self):
    """
    This sets the QProcess object up and adds all of the aspects that will remain consistent for each directory
    processed.
    :return:
    """
    launcher = QProcess()
    env = get_environment_variables('maya')
    launcher.setProcessEnvironment(env)
    launcher.setProgram(constants.MAYA_ENV['DCCSI_PY_MAYA'])

















