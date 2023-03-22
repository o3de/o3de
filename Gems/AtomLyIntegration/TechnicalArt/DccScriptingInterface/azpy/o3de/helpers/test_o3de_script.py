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

"""! @brief MockTool is a collection of template files that can be used to start a new tool within the DCCsi. """

# Required Imports
import config
import logging
from pathlib import Path
from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Signal, Slot
from azpy.dcc.o3de.utils.o3de_client import O3DEClient
import json
import os


_LOGGER = logging.getLogger('azpy.o3de.helpers.sandbox')


class TestO3DE(QtWidgets.QWidget):
    def __init__(self, *args, **kwargs):
        super(TestO3DE, self).__init__()
        _LOGGER.info('Test O3DE Script added')

        # ENVIRONMENT INFO --------------------
        self.o3de_envars = config.get_environment_values('o3de')
        self.maya_envars = config.get_environment_values('maya')

        # SOCKET CONNECTIONS --------------------
        self.o3de_connection = None

        # CRITICAL PATHS --------------------
        self.maya_file_location = None
        self.o3de_asset_directory = None
        self.base_asset_directory = None
        self.dccsi_base = settings.get('PATH_DCCSIG')
        self.desktop_location = os.path.join(os.path.expanduser('~'), 'Desktop')
        self.registered_projects = settings.get('REGISTERED_PROJECTS')
        self.current_project = settings.get('CURRENT_PROJECT')
        self.o3de_server_script = Path('E:/Depot/o3de-gems/DCCsiLink/Editor/Scripts/dccsilink_dialog.py')
        self.o3de_scene_build_script = self.dccsi_base / 'azpy/dcc/o3de/utils/o3de_level_builder.py'
        self.target_maya_script = self.dccsi_base / 'azpy/dcc/maya/utils/maya_live_link.py'
        self.target_o3de_script = self.dccsi_base / 'azpy/dcc/o3de/utils/o3de_live_link.py'
        self.audit_maya_script = self.dccsi_base / 'azpy/dcc/maya/utils/maya_scene_audit.py'
        self.maya_scene_export_script = self.dccsi_base / 'azpy/dcc/maya/utils/maya_scene_export.py'
        self.maya_scene_info = self.get_json_data(
            "E:/Depot/o3de-engine/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/azpy/o3de/helpers/maya_scene_info.json")
        self.exported_scene_data = self.get_json_data(
            "E:/Depot/o3de-engine/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/azpy/o3de/helpers/exported_scene_data.json")

        # UI --------------------
        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.run_script_button = QtWidgets.QPushButton('Run script')
        self.run_script_button.clicked.connect(self.run_script_clicked)
        self.run_script_button.setFixedHeight(50)
        self.run_script_button.setObjectName('Primary')
        self.main_container.addWidget(self.run_script_button)
        self.clear_scene_button = QtWidgets.QPushButton('Clear Scene Elements')
        self.clear_scene_button.clicked.connect(self.clear_scene_clicked)
        self.clear_scene_button.setFixedHeight(50)
        self.clear_scene_button.setObjectName('Secondary')
        self.main_container.addWidget(self.clear_scene_button)
        self.get_connection().establish_connection()

    def get_json_data(self, target_path):
        with open(target_path) as json_file:
            return json.load(json_file)

    def get_connection(self):
        if not self.o3de_connection:
            self.o3de_connection = O3DEClient(17345)
            self.o3de_connection.client_activity_registered.connect(self.o3de_client_event_fired)
        return self.o3de_connection

    def run_socket_script(self, script_path, arguments):
        _LOGGER.info(f'SOCKET SCRIPT::> {script_path}')
        connection = self.get_connection()
        if connection.establish_connection():
            return_data = connection.run_script(script_path, arguments)
            _LOGGER.info(f'Return data: {return_data}')
            if return_data:
                return return_data
        else:
            _LOGGER.info('Failed to connect')

    ###############################
    # O3DE Operations  ############
    ###############################

    def run_o3de_script(self):
        script_path = self.o3de_scene_build_script.as_posix()
        arguments = {
            'class':                'O3DELevelBuilder',
            'target_application':   'o3de',
            'operation':            'level-build',
            'audit_info':           self.maya_scene_info,
            'export_info':          self.exported_scene_data,
            'PATH_DCCSIG':          settings.get('PATH_DCCSIG').as_posix(),
            'level_name':           'new_level'
        }
        self.run_socket_script(script_path, arguments)

    ####################################
    # Button Actions  ##################
    ####################################

    def run_script_clicked(self):
        _LOGGER.info('Run script clicked')
        if self.get_connection().establish_connection():
            self.run_script()

    def clear_scene_clicked(self):
        _LOGGER.info('Clear scene clicked')

    @Slot(dict)
    def o3de_client_event_fired(self, result):
        _LOGGER.info(f'O3DE Client event fired::::: {result}')
        if result and isinstance(result, dict):
            message = result['result']
            del result['result']

            if message == 'connected':
                self.set_connection('o3de')
            elif message == 'scene_build_ready':
                _LOGGER.info('Scene build ready')
            elif message == 'scene_build_complete':
                _LOGGER.info('Starting Live Link session...')
            elif message == 'scene_update_event':
                _LOGGER.info('O3DE scene update event')


def get_tool(*args, **kwargs):
    return TestO3DE(*args, **kwargs)


if __name__ == '__main__':
    TestO3DE()
