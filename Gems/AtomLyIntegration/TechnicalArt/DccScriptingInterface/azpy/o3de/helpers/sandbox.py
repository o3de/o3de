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
from azpy.shared import qt_process
from azpy.dcc.maya.utils.maya_client import MayaClient
from azpy.dcc.o3de.utils.o3de_client import O3DEClient
import os


_LOGGER = logging.getLogger('azpy.o3de.helpers.sandbox')


class Sandbox(QtWidgets.QWidget):
    def __init__(self, *args, **kwargs):
        super(Sandbox, self).__init__()
        _LOGGER.info('Sandbox Added')

        self.o3de_connection = None
        self.o3de_envars = config.get_environment_values('o3de')
        _LOGGER.info(f'O3DE Envars: {self.o3de_envars}')
        self.maya_connection = None
        self.maya_file_location = None
        self.o3de_asset_directory = None
        self.base_asset_directory = None
        self.maya_scene_audit = {}
        self.scene_assets_transferred = None
        self.dccsi_base = settings.get('PATH_DCCSIG')
        self.desktop_location = os.path.join(os.path.expanduser('~'), 'Desktop')
        self.registered_projects = settings.get('REGISTERED_PROJECTS')
        self.current_project = settings.get('CURRENT_PROJECT')
        self.maya_envars = config.get_environment_values('maya')
        _LOGGER.info(f'Maya Envars:: {self.maya_envars}')
        self.target_maya_script = self.dccsi_base / 'azpy/dcc/maya/utils/maya_live_link.py'
        self.target_o3de_script = self.dccsi_base / 'azpy/dcc/o3de/utils/o3de_live_link.py'
        self.audit_maya_script = self.dccsi_base / 'azpy/dcc/maya/utils/maya_scene_audit.py'
        self.maya_scene_export_script = self.dccsi_base / 'azpy/dcc/maya/utils/maya_scene_export.py'
        self.boldFont = QtGui.QFont("Plastique", 8, QtGui.QFont.Bold)

        # MAYA SPECIFIC CONTROLS --------------------
        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.maya_sync_label = QtWidgets.QLabel('MAYA CONNECTION')
        self.maya_sync_label.setFont(self.boldFont)
        self.main_container.addWidget(self.maya_sync_label)
        self.main_container.addSpacing(5)

        # Maya browse for file lockup
        self.maya_file_label = QtWidgets.QLabel('Target Maya File')
        self.main_container.addWidget(self.maya_file_label)
        self.set_file_container = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.set_file_container)
        self.maya_file_location_field = QtWidgets.QLineEdit()
        self.maya_file_location_field.textChanged.connect(self.validate_maya_file)
        self.maya_file_location_field.setFixedHeight(25)
        self.set_file_container.addWidget(self.maya_file_location_field)
        self.set_maya_button = QtWidgets.QPushButton('Set')
        self.set_maya_button.clicked.connect(self.set_file_clicked)
        self.set_maya_button.setFixedSize(35, 25)
        self.set_file_container.addWidget(self.set_maya_button)
        self.set_maya_button.setObjectName('Primary')

        # Maya connection indicator
        self.maya_connected_label = QtWidgets.QLabel('Not Connected')
        self.main_container.addWidget(self.maya_connected_label)
        self.maya_connected_label.setStyleSheet('color: red')
        self.main_container.addSpacing(5)

        # Maya Connection Buttons
        self.maya_buttons_container = QtWidgets.QHBoxLayout()
        self.maya_buttons_container.setAlignment(QtCore.Qt.AlignLeft)
        self.main_container.addLayout(self.maya_buttons_container)
        self.start_maya_button = QtWidgets.QPushButton('Start Maya Connection')
        self.start_maya_button.setFixedSize(150, 25)
        self.start_maya_button.clicked.connect(self.start_maya_clicked)
        self.start_maya_button.setObjectName('Primary')
        self.maya_buttons_container.addWidget(self.start_maya_button)
        self.test_maya_button = QtWidgets.QPushButton('Test Maya Connection')
        self.test_maya_button.setFixedSize(150, 25)
        self.test_maya_button.clicked.connect(self.test_maya_clicked)
        self.test_maya_button.setEnabled(False)
        self.test_maya_button.setObjectName('Primary')
        self.maya_buttons_container.addWidget(self.test_maya_button)
        self.main_container.addSpacing(10)

        # Separator bar
        self.main_container.addLayout(self.get_separator_layout())
        self.main_container.addSpacing(10)

        # O3DE SPECIFIC CONTROLS --------------------
        self.o3de_sync_label = QtWidgets.QLabel('O3DE CONNECTION')
        self.o3de_sync_label.setFont(self.boldFont)
        self.main_container.addWidget(self.o3de_sync_label)
        self.main_container.addSpacing(5)

        # O3DE browse for repository Lockup
        self.o3de_repository_label = QtWidgets.QLabel('Repository Base Location')
        self.main_container.addWidget(self.o3de_repository_label)
        self.set_repo_container = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.set_repo_container)
        self.repo_location_field = QtWidgets.QLineEdit()
        self.repo_location_field.textChanged.connect(self.validate_repository_location)
        self.repo_location_field.setFixedHeight(25)
        self.set_repo_container.addWidget(self.repo_location_field)
        self.set_repo_button = QtWidgets.QPushButton('Set')
        self.set_repo_button.clicked.connect(self.set_file_clicked)
        self.set_repo_button.setFixedSize(35, 25)
        self.set_repo_container.addWidget(self.set_repo_button)
        self.set_repo_button.setObjectName('Primary')

        # Browse Asset Transfer Location Lockup
        self.asset_transfer_label = QtWidgets.QLabel('Conversion Asset Base Directory')
        self.main_container.addWidget(self.asset_transfer_label)
        self.set_asset_location_container = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.set_asset_location_container)
        self.asset_location_field = QtWidgets.QLineEdit()
        self.asset_location_field.textChanged.connect(self.validate_asset_location)
        self.asset_location_field.setFixedHeight(25)
        self.set_asset_location_container.addWidget(self.asset_location_field)
        self.set_asset_location_button = QtWidgets.QPushButton('Set')
        self.set_asset_location_button.clicked.connect(self.set_directory_clicked)
        self.set_asset_location_button.setFixedSize(35, 25)
        self.set_asset_location_button.setObjectName('Primary')
        self.set_asset_location_container.addWidget(self.set_asset_location_button)

        # O3DE Comboboxes
        self.o3de_combobox_layout = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.o3de_combobox_layout)

        # Select project
        self.project_combobox_layout = QtWidgets.QVBoxLayout()
        self.o3de_combobox_layout.addLayout(self.project_combobox_layout)
        self.select_project_label = QtWidgets.QLabel('Select Project')
        self.project_combobox_layout.addWidget(self.select_project_label)
        self.project_combobox = QtWidgets.QComboBox()
        self.project_combobox.setFixedHeight(25)
        self.project_combobox_layout.addWidget(self.project_combobox)

        # Select level
        self.level_combobox_layout = QtWidgets.QVBoxLayout()
        self.o3de_combobox_layout.addLayout(self.level_combobox_layout)
        self.select_level_label = QtWidgets.QLabel('Select Level')
        self.level_combobox_layout.addWidget(self.select_level_label)
        self.level_combobox = QtWidgets.QComboBox()
        self.level_combobox.addItem('Create New')
        self.level_combobox.setFixedHeight(25)
        self.level_combobox_layout.addWidget(self.level_combobox)

        # O3DE connection indicator
        self.o3de_connected_label = QtWidgets.QLabel('Not Connected')
        self.main_container.addWidget(self.o3de_connected_label)
        self.o3de_connected_label.setStyleSheet('color: red')
        self.main_container.addSpacing(5)

        # O3DE connection buttons
        self.o3de_buttons_container = QtWidgets.QHBoxLayout()
        self.o3de_buttons_container.setAlignment(QtCore.Qt.AlignLeft)
        self.main_container.addLayout(self.o3de_buttons_container)
        self.start_o3de_button = QtWidgets.QPushButton('Start O3DE Connection')
        self.start_o3de_button.setFixedSize(150, 25)
        self.start_o3de_button.clicked.connect(self.start_o3de_clicked)
        self.start_o3de_button.setObjectName('Primary')
        self.o3de_buttons_container.addWidget(self.start_o3de_button)

        self.test_o3de_button = QtWidgets.QPushButton('Test O3DE Connection')
        self.test_o3de_button.setFixedSize(150, 25)
        self.test_o3de_button.clicked.connect(self.test_o3de_clicked)
        self.test_o3de_button.setObjectName('Primary')
        self.test_o3de_button.setEnabled(False)
        self.o3de_buttons_container.addWidget(self.test_o3de_button)
        self.main_container.addSpacing(10)

        # Separator bar
        self.main_container.addLayout(self.get_separator_layout())
        self.main_container.addSpacing(10)

        self.main_container.addSpacing(15)
        self.start_live_link_button = QtWidgets.QPushButton('Start Live Link Session')
        self.start_live_link_button.setObjectName('Primary')
        self.start_live_link_button.setFixedHeight(25)
        self.start_live_link_button.clicked.connect(self.start_live_link_clicked)
        self.main_container.addWidget(self.start_live_link_button)
        # self.initialize_connections()

        # For testing... remove this eventually:
        self.maya_file_location = self.dccsi_base / 'Tools/Python/MockTool/Resources/test_scene.mb'
        self.base_asset_directory = 'E:/Depot/o3de-gems/DCCsiLink/Assets'
        self.maya_file_location_field.setText(self.maya_file_location.as_posix())
        self.asset_location_field.setText(self.base_asset_directory)
        self.o3de_repository_location = Path('E:/Depot/o3de-engine')
        self.repo_location_field.setText(self.o3de_repository_location.as_posix())

    def get_separator_layout(self):
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QFrame()
        self.line.setStyleSheet('color: rgb(20, 20, 20);')
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setLineWidth(1)
        self.separator_layout.addWidget(self.line)
        return self.separator_layout

    def initialize_connections(self):
        if self.get_connection('maya'):
            self.start_maya_button.setEnabled(False)
            self.test_maya_button.setEnabled(True)
        if self.get_connection('o3de'):
            self.start_o3de_button.setEnabled(False)
            self.test_o3de_button.setEnabled(True)

    def get_connection(self, target_application):
        if target_application == 'maya':
            if not self.maya_connection:
                self.maya_connection = MayaClient(17344)
                if self.maya_connection.establish_connection():
                    self.maya_connection.client_activity_registered.connect(self.maya_client_event_fired)
                    self.set_connection_flag('maya', True)
                else:
                    self.maya_connection = None
                    self.set_connection_flag('maya', False)
                    _LOGGER.info(f'Maya client failed to connect- click the Start Maya Connection Button to establish '
                                 f'a connection.')
            return self.maya_connection
        else:
            if not self.o3de_connection:
                self.o3de_connection = O3DEClient(17345)
                if self.o3de_connection.establish_connection():
                    self.set_connection_flag('o3de', True)
                else:
                    self.o3de_connection = None
                    self.set_connection_flag('o3de', False)
                    _LOGGER.info(f'O3DE Client failed to connect- click the Start O3DE Connection Button to establish'
                                 f' a connection.')
            return self.o3de_connection

    def run_socket_script(self, target_application, script_path, arguments):
        connection = self.get_connection(target_application)
        if connection:
            return_data = connection.run_script(script_path, arguments)
            _LOGGER.info(f'Return data: {return_data}')
            if return_data:
                return return_data
        else:
            _LOGGER.info('Failed to connect')

    def set_connection_flag(self, target_application, activated):
        target_label = self.maya_connected_label if target_application == 'maya' else self.o3de_connected_label
        target_label.setText('Connected') if activated else target_label.setText('Not Connected')
        target_label.setStyleSheet('color: rgb(0, 255, 0);') if activated else target_label.setStyleSheet('color: red')

    def format_qprocess_environment(self, target_environment, script_data=None):
        data = [script_data] if script_data else []
        for key, value in target_environment.items():
            data.append(f'{key}={value}')
        data.append(f'PATH_DCCSIG={self.dccsi_base.as_posix()}')
        return data

    ###############################
    # Maya Operations  ############
    ###############################

    def validate_maya_file(self):
        file = Path(self.maya_file_location_field.text())
        if file.suffix in ['.ma', '.mb', '.fbx'] and file.exists():
            self.maya_file_location = file.as_posix()
            if not self.maya_connection:
                self.start_maya_button.setEnabled(True)
        else:
            self.maya_file_location = None
            self.start_maya_button.setEnabled(False)

    def audit_maya_file(self):
        script_path = self.audit_maya_script.as_posix()
        arguments = {
            'class':              'MayaSceneAuditor',
            'target_application': 'maya',
            'target_files':       'current',
            'operation':          'audit'
        }
        self.run_socket_script('maya', script_path, arguments)

    def start_asset_export(self):
        script_path = self.maya_scene_export_script.as_posix()
        arguments = {
            'class':                'MayaSceneExporter',
            'export_location':      self.o3de_asset_directory,
            'operation':            'sceneExport',
            'audit_info':           self.maya_scene_audit,
            'PATH_DCCSIG':          settings.get('PATH_DCCSIG').as_posix(),
            'O3DE_DEV':             settings.get('O3DE_DEV').as_posix()
        }
        self.run_socket_script('maya', script_path, arguments)

    def start_maya_live_link(self):
        if self.maya_scene_audit:
            script_path = self.target_maya_script.as_posix()
            arguments = {
                'class':              'MayaLiveLink',
                'target_application': 'maya',
                'target_files':       'current',
                'operation':          'live-link'
            }
            arguments.update(self.maya_scene_audit)
            self.run_socket_script('maya', script_path, arguments)

    ###############################
    # O3DE Operations  ############
    ###############################

    def validate_repository_location(self):
        self.project_combobox.clear()
        target_path = Path(self.repo_location_field.text()).as_posix()
        if target_path in settings.get('REGISTERED_ENGINE'):
            self.o3de_repository_location = target_path
            project_list = []
            for project in self.registered_projects:
                project_name = Path(project).parts[-1]
                if project_name != self.current_project:
                    project_list.append(project)
                else:
                    project_list.insert(0, project_name)
            self.project_combobox.addItems(project_list)
            if project_list:
                if not self.o3de_connection:
                    self.start_o3de_button.setEnabled(True)
                    _LOGGER.info('Valid Repository Location has been entered.')
        else:
            self.o3de_repository_location = None
            self.start_o3de_button.setEnabled(False)

    def validate_asset_location(self):
        asset_path = self.asset_location_field.text()
        if Path(asset_path).is_dir():
            self.o3de_asset_directory = Path(asset_path).as_posix()
        return False

    def start_o3de_live_link(self, target_files='current'):
        script_path = self.target_o3de_script.as_posix()
        arguments = {
            'class':              'O3DELiveLink',
            'target_application': 'o3de',
            'target_files':       target_files,
            'operation':          'live-link'
        }
        arguments.update(self.maya_scene_audit)
        self.run_socket_script('o3de', script_path, arguments)

    ####################################
    # Button Actions  ##################
    ####################################

    def set_file_clicked(self):
        dialog = QtWidgets.QFileDialog()
        dialog.setDirectory(settings.get('PATH_DCCSIG').as_posix())
        dialog.setWindowTitle('Select Maya File')
        dialog.setNameFilters(['Maya Files (*.ma *.mb)', 'FBX Files (*.fbx)'])
        dialog.setFileMode(QtWidgets.QFileDialog.ExistingFile)
        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            self.maya_file_location = dialog.selectedFiles()[0]
            self.maya_file_location_field.setText(self.maya_file_location)

    def set_directory_clicked(self):
        file_path = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Asset Directory', self.desktop_location,
                                                               QtWidgets.QFileDialog.ShowDirsOnly)
        if file_path != '':
            self.o3de_asset_directory = file_path
            self.asset_location_field.setText(self.o3de_asset_directory)

    def start_maya_clicked(self):
        _LOGGER.info('Start Maya clicked')
        connection_info = {
            'exe': settings.get('MAYA_EXE'),
            'script': self.target_maya_script,
            'file': self.maya_file_location
        }
        self.maya_envars['O3DE'] = settings.get('O3DE_DEV')
        self.maya_envars['PATH_DCCSIG'] = settings.get('PATH_DCCSIG')
        env_data = self.format_qprocess_environment(self.maya_envars, f"SCRIPT={connection_info['script']}")
        app = qt_process.QtProcess(connection_info['exe'], connection_info['file'], env_data)
        app.start_process(socket_connection=True)
        self.set_connection_flag('maya', True)
        self.start_maya_button.setEnabled(False)
        self.test_maya_button.setEnabled(True)

    def test_maya_clicked(self):
        _LOGGER.info('Test Maya clicked')
        if self.get_connection('maya'):
            self.maya_connection.echo('Echo test')

    def start_o3de_clicked(self):
        _LOGGER.info('Start O3DE clicked')
        exe = (Path(settings.get('PATH_O3DE_BIN')) / 'Editor.exe').as_posix()
        command = ['--runpython', self.target_o3de_script.as_posix(),
                   '--runpythonargs', 'blacktestparam']

        app = qt_process.QtProcess(exe, None, [])
        app.start_o3de_process(command)
        self.set_connection_flag('o3de', True)
        self.start_o3de_button.setEnabled(False)
        self.test_o3de_button.setEnabled(True)

    def test_o3de_clicked(self):
        _LOGGER.info('Test O3DE clicked')
        if self.get_connection('o3de'):
            self.o3de_connection.echo('Echo test')

    def start_live_link_clicked(self):
        _LOGGER.info('LiveLink started')
        if not self.maya_scene_audit:
            self.audit_maya_file()
        elif not self.scene_assets_transferred:
            self.start_asset_export()
        else:
            self.start_maya_live_link()

    @Slot(dict)
    def maya_client_event_fired(self, result):
        _LOGGER.info(f'Maya Client event fired::::: {result}')
        if result:
            operation = result['cmd']
            del result['cmd']
            if operation == 'audit':
                self.maya_scene_audit = result
                self.start_asset_export()
            elif operation == 'update_event':
                _LOGGER.info(f'UpdateEvent::: {result}')

    @Slot(dict)
    def o3de_client_event_fired(self, result):
        _LOGGER.info(f'O3DE Client event fired::::: {result}')
        if result:
            operation = result['cmd']
            del result['cmd']

            if operation == 'scene_sync_ready':
                _LOGGER.info('Starting Live Link session...')
            elif operation == 'scene_build_ready':
                _LOGGER.info('Scene build ready')
            elif operation == 'update_event':
                _LOGGER.info(f'UpdateEvent::: {result}')


def get_tool(*args, **kwargs):
    return Sandbox(*args, **kwargs)


if __name__ == '__main__':
    Sandbox()
