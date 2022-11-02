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

##
# @file main.py
#
# @brief It is difficult to predict in what form tools developed in the DCCsi will take, but a few elements will
# remain constant when creating a new tool. Environment will usually play a significant role in tools creation. The
# DCCsi is supported with Dynaconf for setting environments- main settings are created and maintained within both the
# config.py and settings.json files in the base directory of the DCCsi, but per tool settings can be created by way of
# and ".env" file. A sample of this file has been included in the sample MockTool directory.
#
# Logging is also an important requirement for developing tools and workflows to assist in debugging. The formatting
# is very easy to follow. See below for an example of the setup. By changing the "_MODULENAME" path to correspond with
# a tool's module path is all that is needed. By adding logging statements (also included below), formatted logging
# output will be enabled.
#
# The last important element to point out is that each tool should include the three imports listed below:
#  - The "config" module ensures that Python has access to O3DE- utilized third party libraries, and establishes the
#    Dynaconf settings
#  - Logging must be imported in order to access the main logging system that supports the DCCsi. No new configurations
#    are necessary. Users can leverage the default settings, or adjust the existing configuration with their own
#    desired settings when developing tools.
#  - The last import of "settings" connects the established Dynaconf environment. The best access of environment
#    settings can be achieved by using a combination of settings established in the .env file, as well as leveraging
#    DCCsi default settings, as well as targeted DCCsi environment settings that exist specific to development needs.
#
#  Examples of each of the elements above can be found below, and tailored to the needs of a project.
#
# @section Launcher Notes
# - Comments are Doxygen compatible

# Required Imports
import config
import logging
from pathlib import Path
from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from SDK.Python import general_utilities as utils
from azpy.shared import qt_process

# O3DE Imports
import azlmbr.bus as bus
import azlmbr.components as components
import azlmbr.editor as editor
import azlmbr.entity as entity
import azlmbr.math as math

from PySide2.QtCore import Qt
from PySide2.QtGui import QDoubleValidator


# Logging Formatting
_MODULENAME = 'Tools.Python.MockTool.main'
_LOGGER = logging.getLogger(_MODULENAME)


class MockTool(QtWidgets.QWidget):
    def __init__(self, *args, **kwargs):
        super(MockTool, self).__init__()
        _LOGGER.info('MockTool Added')
        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.main_container.addSpacing(15)
        self.bold_font = QtGui.QFont("Helvetica", 9, QtGui.QFont.Bold)

        # Example Widgets
        self.sample_text = Path(settings.get('PATH_DCCSI_TOOLS') / 'Python/MockTool/Resources/sample_text.txt')
        self.env_file_location = Path(settings.get('PATH_DCCSI_TOOLS') / 'Python/MockTool/.env')
        self.resources_directory = Path(settings.get('PATH_DCCSI_TOOLS') / 'Python/MockTool/Resources')
        self.example_text_edit = QtWidgets.QLabel(self.get_sample_text())
        self.example_text_edit.setWordWrap(True)
        self.main_container.addWidget(self.example_text_edit)
        self.main_container.addSpacing(15)
        self.test_button = QtWidgets.QPushButton('QPushButton styled with the O3DE Stylesheet')
        self.test_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.test_button.clicked.connect(self.sample_button_clicked)
        self.test_button.setFixedHeight(25)
        self.test_button.setObjectName('Primary')
        self.main_container.addWidget(self.test_button)
        self.main_container.addSpacing(15)

        # Add separator bar
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QFrame()
        self.line.setStyleSheet('color: rgb(20, 20, 20);')
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setLineWidth(1)
        self.separator_layout.addWidget(self.line)
        self.main_container.addLayout(self.separator_layout)
        self.main_container.addSpacing(10)

        # These access environment variables
        self.env = self.get_environment_values()

        # DCC Specific Script control
        self.dccsi_path = settings.get('PATH_DCCSIG')
        self.maya_audit_script = Path(f'{self.dccsi_path}/azpy/dcc/maya/utils/maya_scene_audit.py').as_posix()
        self.blender_audit_script = Path(f'{self.dccsi_path}/azpy/dcc/blender/utils/blender_scene_audit.py').as_posix()
        self.houdini_audit_script = Path(f'{self.dccsi_path}/azpy/dcc/houdini/utils/houdini_scene_audit.py').as_posix()
        self.dcc_info = {
            'Maya': {'envar': 'MAYA_PY', 'id': 0, 'script': self.maya_audit_script, 'ext': '.mb'},
            'Blender': {'envar': 'BLENDER_PY', 'id': 1, 'script': self.blender_audit_script, 'ext': '.blend'},
            'Houdini': {'envar': 'HOUDINI_PY', 'id': 2, 'script': self.houdini_audit_script, 'ext': '.hip'}
        }
        self.dcc_label = QtWidgets.QLabel('DCC Application Control')
        self.dcc_label.setFont(self.bold_font)
        self.main_container.addWidget(self.dcc_label)
        self.app_connect_text = QtWidgets.QLabel(self.get_app_connect_text())
        self.app_connect_text.setWordWrap(True)
        self.main_container.addWidget(self.app_connect_text)
        self.main_container.addSpacing(15)
        self.dcc_button_groupbox = QtWidgets.QGroupBox()
        self.dcc_button_groupbox.setStyleSheet(
            "QGroupBox { border: 1px solid rgb(40, 40, 40); background-color: rgb(120, 120, 120)};"
            "QGroupBox:QRadioButton {color: red;};")
        self.radio_button_group = QtWidgets.QButtonGroup()
        self.group_box_layout = QtWidgets.QVBoxLayout()
        self.dcc_button_groupbox.setLayout(self.group_box_layout)
        self.radio_0 = QtWidgets.QRadioButton('Maya')
        self.radio_button_group.addButton(self.radio_0, 0)
        self.group_box_layout.addWidget(self.radio_0)
        self.radio_1 = QtWidgets.QRadioButton('Blender')
        self.radio_button_group.addButton(self.radio_1, 1)
        self.group_box_layout.addWidget(self.radio_1)
        self.radio_2 = QtWidgets.QRadioButton('Houdini')
        self.radio_button_group.addButton(self.radio_2, 2)
        self.group_box_layout.addWidget(self.radio_2)
        self.main_container.addWidget(self.dcc_button_groupbox)
        self.checkbox_layout = QtWidgets.QHBoxLayout()
        self.main_container.addSpacing(5)
        self.open_process_checkbox = QtWidgets.QCheckBox('Leave Application/Process Open')
        self.open_process_checkbox.setChecked(True)
        self.main_container.addWidget(self.open_process_checkbox)
        self.main_container.addSpacing(10)
        self.dcc_test_button = QtWidgets.QPushButton('Run DCC Connection Test')
        self.dcc_test_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.dcc_test_button.clicked.connect(self.run_connection_clicked)
        self.dcc_test_button.setFixedHeight(25)
        self.dcc_test_button.setObjectName('Primary')
        self.main_container.addWidget(self.dcc_test_button)
        self.get_dcc_apps()
        # self.test_environment()

    def test_environment(self):
        # For project specific variables created OUTSIDE the DCCsi Dynaconf default environments (using the
        # Dynaconf .env process), access variables using this:
        self.get_env_value()

        # Some environment variables are stored as custom environments within the DCCsi Dynaconf settings.json system
        # and are therefore not accessible by default. In order to gain access to those environments, run the following
        # function with the argument matching the custom environment name
        self.get_environment_values('maya')

    def get_sample_text(self):
        _LOGGER.info(utils.get_txt_data(self.sample_text))
        return utils.get_txt_data(self.sample_text)

    def get_app_connect_text(self):
        txt = 'Below are DCC Applications that the DCCsi can connect with to run utilities, tools, and productivity ' \
              'scripts. Select one of the radio buttons and click the "Execute" button to run the connection test. ' \
              'The test script will launch the DCC Application of choice and run a simple audit of scene contents, ' \
              'and will print them out to the console. This will provide you with a starting point for using the ' \
              'DCCsi socket connection capabilities to connect with your target application to run productivity ' \
              'tools that you create. If any of the buttons are disabled, this is because the application was not ' \
              'detected as currently installed on your system.'
        return txt

    def get_dcc_apps(self):
        system_apps = []
        for key, values in self.dcc_info.items():
            target_button = self.radio_button_group.button(values['id'])
            target_application = values['envar']
            if not self.env[target_application]:
                target_button.setEnabled(False)
            else:
                system_apps.append(target_button)
        if system_apps:
            system_apps[0].setChecked(True)
        else:
            self.dcc_test_button.setEnabled(False)

    def get_env_value(self):
        """! Access variables from the .env file. If this tool is launched by the Launcher tool, Dynaconf will
         not know to read and add in .env values, which is provided for below.
         """
        # Extract .env values manually if tool run through DCCsi Launcher. Set 'env_file_location'
        # class attribute path accordingly for your tool's location
        if Path.cwd().parts[-1] == 'DccScriptingInterface':
            envars = utils.parse_env_file(self.env_file_location)
            for key, values in envars.items():
                _LOGGER.info(f'Setting [{key}] ::: {values}')
                target_path = Path(settings.get(values[0])) / values[1]
                settings.set(key, target_path)

        # Get Settings Value
        _LOGGER.info(f"Access the DYNACONF_ICON_PATH value from the .env file: {settings.get('ICON_PATH')}")
        _LOGGER.info(f"Access the DYNACONF_DB_PATH value from the .env file: {settings.get('DB_PATH')}")

        # Check if Settings Value is present - if it isn't will return the second value (in this case "404")
        _LOGGER.info(f"Accessing Dynaconf Base Directory Variable: {settings.get('NO_KEY_AVAILABLE', 404)}")

    def get_dcc_test_info(self):
        test_info = {}
        target_id = self.radio_button_group.checkedId()
        test_info['app'] = self.radio_button_group.button(target_id).text()
        test_info['exe'] = self.env[self.dcc_info[test_info['app']]['envar']]
        if self.open_process_checkbox.isChecked():
            test_info['exe'] = self.env[self.dcc_info[test_info['app']]['envar'].replace('PY', 'EXE')]
        test_info['script'] = self.dcc_info[test_info['app']]['script']
        extension = self.dcc_info[test_info['app']]['ext']
        test_info['file'] = (self.resources_directory / f'test_scene{extension}').as_posix()
        return test_info

    def get_environment_values(self, target_environment=None):
        """ Gather environment variables stored in Dynaconf """
        envars = {}
        for key, value in settings.items():
            if target_environment:
                if key in settings.from_env(target_environment):
                    envars[key] = value
            else:
                envars[key] = value
        return envars

    def format_qprocess_environment(self, target_environment, script_data=None):
        data = [script_data] if script_data else []
        for key, value in target_environment.items():
            data.append(f'{key}={value}')
        return data

    def sample_button_clicked(self):
        _LOGGER.info('Sample Button Clicked')

    def run_connection_clicked(self):
        connection_info = self.get_dcc_test_info()
        environment = self.get_environment_values(connection_info['app'].lower())
        env_data = self.format_qprocess_environment(environment, f"SCRIPT={connection_info['script']}")
        _LOGGER.info(f"Test Started::: Connecting to [{connection_info['app']}]: {connection_info['exe']}")
        _LOGGER.info(f'Data check: {env_data}')
        app = qt_process.QtProcess(connection_info['exe'], connection_info['file'], env_data)
        app.start_process(socket_connection=True)


def get_tool(*args, **kwargs):
    _LOGGER.info('Get tool fired')
    return MockTool(*args, **kwargs)


if __name__ == '__main__':
    MockTool()


