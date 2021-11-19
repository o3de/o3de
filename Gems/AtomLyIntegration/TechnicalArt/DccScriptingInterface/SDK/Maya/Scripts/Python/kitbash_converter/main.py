# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# File Description:
# This tool helps to convert KitBash3D FBX models to Lumberyard materials.
# -------------------------------------------------------------------------

# Built-ins
from functools import partial
import logging as _logging
from pathlib import Path
import json
import time
import sys
import os
import re

# azpy bootstrapping and extensions
import azpy.config_utils

_config = azpy.config_utils.get_dccsi_config()
print(_config)
# ^ this is effectively an import and retrieve of <dccsi>\config.py
# and init's access to Qt/Pyside2
# init lumberyard Qy/PySide2 access
# now default settings are extended with PySide2
# this is an alternative to "from dynaconf import settings" with Qt
settings = _config.get_config_settings(setup_ly_pyside=True)

# app_path = Path.joinpath(settings.DCCSIG_PATH, < relative path to current script dir >).resolve()

# 3rd Party (we may or do provide)
from box import Box

# lumberyard Qt/PySide2
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtWidgets import QApplication, QMessageBox
from PySide2.QtCore import Signal, Slot, QThread, QProcess, QProcessEnvironment

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

module_name = 'kitbash_converter.main'
log_file_path = os.path.join(settings.DCCSI_LOG_PATH, f'{module_name}.log')

_log_level = int(20)
_DCCSI_GDEBUG = True
if _DCCSI_GDEBUG:
    _log_level = int(10)

from azpy.constants import FRMT_LOG_LONG

_logging.basicConfig(level=_log_level,
                     format=FRMT_LOG_LONG,
                     datefmt='%m-%d %H:%M',
                     filename=log_file_path,
                     filemode='w')
_LOGGER = _logging.getLogger(module_name)

console_handler = _logging.StreamHandler(sys.stdout)
console_handler.setLevel(_log_level)
formatter = _logging.Formatter(FRMT_LOG_LONG)
console_handler.setFormatter(formatter)
_LOGGER.addHandler(console_handler)
_LOGGER.setLevel(_log_level)
_LOGGER.debug('Initializing: {0}.'.format({module_name}))

# early attach WingIDE debugger (can refactor to include other IDEs later)
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_DEV_MODE
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, True)  # flip to True to attach

if _DCCSI_DEV_MODE:
    from azpy.test.entry_test import connect_wing
    foo = connect_wing()
# to do: ^ this should be replaced with full featured azpy.dev.util
# that supports additional debuggers (pycharm, vscode, etc.)

# Things to do
# Create materials for shaders without textures attached- most likely glass
# Plants- get opacity/clipping set up correctly. Light transmission
# Handle "Split texture alpha" Break alpha out of the color and map into the materials

class FBXConverter(QtWidgets.QDialog):

    def __init__(self, parent=None):
        super(FBXConverter, self).__init__(parent)

        self.app = QtWidgets.QApplication.instance()
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 600, 400)
        # self.setFixedWidth(600)
        self.setObjectName('FBXConverter')
        self.setWindowTitle('FBX to Lumberyard')
        self.isTopLevel()
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowMinMaxButtonsHint)

        self.default_search_path = os.path.normpath(Path(__file__).parents[8] / 'AtomContent')
        self.autodesk_directory = Path(os.environ['ProgramFiles']) / 'Autodesk'
        self.default_material_definition = 'standardPBR.template.material'
        self.processed_material_information = {}
        self.relative_export_path = 'Objects'
        self.mayapy_path = self.get_mayapy_path()
        self.single_asset_file = ''
        self.flagged_material_information = None
        self.input_directory = None
        self.output_directory = None
        self.process = None
        self.process_results = {}
        self.audit_list = []
        # self.current_fbx = None
        self.current_audit_index = 0
        self.bold_font = QtGui.QFont("Helvetica", 8, QtGui.QFont.Black)
        self.bold_title_font = QtGui.QFont("Helvetica", 10, QtGui.QFont.Black)
        self.small_font = QtGui.QFont("Helvetica", 7, QtGui.QFont.Black)
        self.line = QtWidgets.QLabel()
        self.separator_layout = QtWidgets.QHBoxLayout()

        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setContentsMargins(0, 0, 0, 0)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.setLayout(self.main_container)
        self.content_layout = QtWidgets.QVBoxLayout()
        self.content_layout.setContentsMargins(8, 10, 8, 0)
        self.content_layout.setAlignment(QtCore.Qt.AlignTop)
        self.main_container.addLayout(self.content_layout)

        # Asset Type Combobox
        self.header_layout = QtWidgets.QHBoxLayout()
        self.asset_type_combobox = QtWidgets.QComboBox()
        self.asset_type_combobox.setFixedHeight(35)
        self.asset_type_combobox.setStyleSheet('padding-left: 10px;')
        self.asset_type_items = ['Kitbash3d', 'Basic Maya']
        self.asset_type_combobox.addItems(self.asset_type_items)
        self.header_layout.addWidget(self.asset_type_combobox)

        # Reset All Button
        self.reset_all_button = QtWidgets.QPushButton('Reset All')
        self.reset_all_button.clicked.connect(self.reset_all_clicked)
        self.reset_all_button.setFont(self.bold_font)
        self.reset_all_button.setFixedSize(85, 35)
        self.header_layout.addWidget(self.reset_all_button)
        self.content_layout.addLayout(self.header_layout)

        # Set Directory Path
        self.target_directory_layout = QtWidgets.QVBoxLayout()
        self.target_directory_layout.setAlignment(QtCore.Qt.AlignTop)
        self.target_directory_groupbox = QtWidgets.QGroupBox()
        self.target_directory_groupbox.setLayout(self.target_directory_layout)
        self.target_directory_layout.setContentsMargins(10, 10, 10, 10)

        # Input Directory
        self.input_field_layout = QtWidgets.QHBoxLayout()
        self.input_directory_label = QtWidgets.QLabel('Input Directory')
        self.input_directory_label.setFixedWidth(75)
        self.input_field_layout.addWidget(self.input_directory_label)
        self.input_path_field = QtWidgets.QLineEdit()
        self.input_path_field.textChanged.connect(self.set_io_directories)
        self.input_path_field.setFixedHeight(25)
        self.input_field_layout.addWidget(self.input_path_field)
        self.set_input_button = QtWidgets.QPushButton('Set')
        self.set_input_button.clicked.connect(partial(self.set_directory_button_clicked, 'input'))
        self.set_input_button.setFixedSize(40, 25)
        self.input_field_layout.addWidget(self.set_input_button)
        self.target_directory_layout.addLayout(self.input_field_layout)
        self.target_directory_layout.addSpacing(8)

        # File handling radio buttons
        self.file_options_layout = QtWidgets.QHBoxLayout()
        self.radio_button_layout = QtWidgets.QHBoxLayout()
        self.radio_button_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.radio_button_group = QtWidgets.QButtonGroup()
        self.use_directory_radio_button = QtWidgets.QRadioButton('Directory')
        self.use_directory_radio_button.setChecked(True)
        self.use_directory_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_group.addButton(self.use_directory_radio_button)
        self.radio_button_layout.addWidget(self.use_directory_radio_button)
        self.radio_button_layout.addSpacing(10)
        self.use_file_radio_button = QtWidgets.QRadioButton('Single File')
        self.use_file_radio_button.setFixedWidth(80)
        self.use_file_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_layout.addWidget(self.use_file_radio_button)
        self.radio_button_group.addButton(self.use_file_radio_button)
        self.file_options_layout.addLayout(self.radio_button_layout)
        self.target_directory_layout.addLayout(self.file_options_layout)
        self.content_layout.addWidget(self.target_directory_groupbox)

        # Relative Output Path
        self.output_directory_widget = QtWidgets.QWidget()
        self.output_field_layout = QtWidgets.QHBoxLayout(self.output_directory_widget)
        self.output_field_layout.setAlignment(QtCore.Qt.AlignRight)
        self.output_field_layout.addStretch()
        self.output_field_layout.setContentsMargins(0, 0, 0, 0)
        self.output_directory_color_frame = QtWidgets.QFrame(self.output_directory_widget)
        self.output_directory_color_frame.setGeometry(0, 0, 5000, 30)
        self.output_directory_color_frame.setStyleSheet('background-color:rgb(210,210,210);')
        self.output_directory_button = QtWidgets.QPushButton(self.relative_export_path)
        self.output_directory_button.clicked.connect(self.set_relative_path_clicked)
        self.output_directory_button.setFixedHeight(30)
        self.output_directory_button.setFont(self.small_font)
        self.output_directory_button.setStyleSheet(
                'QPushButton {border:None; color:rgb(125,125,125); padding-right:10px; background-color:rgb(210,210,210); '
                'text-align:right;} QPushButton:hover { color: black;}')
        self.output_field_layout.addWidget(self.output_directory_button)
        self.file_options_layout.addWidget(self.output_directory_widget)

        # Navigate Listings
        self.review_layout = QtWidgets.QVBoxLayout()
        self.review_layout.setContentsMargins(10, 10, 10, 10)
        self.review_groupbox = QtWidgets.QGroupBox()
        self.review_groupbox.setLayout(self.review_layout)
        self.title_layout = QtWidgets.QHBoxLayout()
        self.review_layout.addLayout(self.title_layout)
        self.object_title_layout = QtWidgets.QHBoxLayout()
        self.object_title_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.title_label = QtWidgets.QLabel('FBX Title')
        self.title_label.setFont(self.bold_title_font)
        self.object_title_layout.addWidget(self.title_label)
        self.object_title_layout.addSpacing(3)
        self.jump_button = QtWidgets.QPushButton('Jump')
        self.jump_button.clicked.connect(self.jump_button_clicked)
        self.jump_button.setFixedSize(32, 22)
        self.object_title_layout.addWidget(self.jump_button)
        self.hidden_button = QtWidgets.QPushButton()
        self.hidden_button.setFixedSize(0,0)
        self.object_title_layout.addWidget(self.hidden_button)
        self.object_title_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.title_layout.addSpacing(5)
        self.title_layout.addLayout(self.object_title_layout)

        self.arrow_button_layout = QtWidgets.QHBoxLayout()
        self.arrow_button_layout.setAlignment(QtCore.Qt.AlignRight)
        self.back_button = QtWidgets.QToolButton()
        self.back_button.setFixedSize(37, 27)
        self.back_button.setArrowType(QtCore.Qt.LeftArrow)
        self.back_button.clicked.connect(self.back_button_clicked)
        self.forward_button = QtWidgets.QToolButton()
        self.forward_button.setFixedSize(37, 27)
        self.forward_button.setArrowType(QtCore.Qt.RightArrow)
        self.forward_button.clicked.connect(self.forward_button_clicked)
        self.arrow_button_layout.addWidget(self.back_button)
        self.arrow_button_layout.addWidget(self.forward_button)
        self.title_layout.addLayout(self.arrow_button_layout)
        self.content_layout.addWidget(self.review_groupbox)
        self.separator_bar_layout = self.get_separator_bar()
        self.review_layout.addLayout(self.separator_bar_layout)

        self.materials_control_layout = QtWidgets.QHBoxLayout()
        self.material_combobox = QtWidgets.QComboBox()
        self.material_combobox.currentIndexChanged.connect(self.material_combobox_changed)
        self.material_combobox_items = []
        self.material_combobox.setFixedHeight(30)
        self.materials_control_layout.addWidget(self.material_combobox)
        self.review_layout.addLayout(self.materials_control_layout)

        self.update_material_button = QtWidgets.QPushButton('Update Material')
        self.update_material_button.setFixedSize(100, 30)
        self.update_material_button.clicked.connect(self.update_material_clicked)
        self.materials_control_layout.addWidget(self.update_material_button)

        # Results Parameters
        self.material_attributes = {}
        self.results_grid_widget = QtWidgets.QWidget()
        self.scroll_area = QtWidgets.QScrollArea(self.results_grid_widget)
        self.scroll_area.setMinimumHeight(300)
        self.review_layout.addWidget(self.scroll_area)
        self.scroll_area.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.scrollAreaWidgetContents = QtWidgets.QWidget()
        self.results_grid_layout = QtWidgets.QGridLayout()
        self.results_grid_layout.setColumnStretch(0,-1)
        self.scrollAreaWidgetContents.setLayout(self.results_grid_layout)
        self.scroll_area.setWidgetResizable(True)
        self.results_grid_layout.setContentsMargins(0, 0, 0, 0)
        self.results_grid_layout.setSpacing(0)
        index = 0
        for key, values in self.get_material_attributes().items():
            attribute_list = [key, values]
            self.material_attributes[key] = AttributeListing(attribute_list)
            self.material_attributes[key].attribute_changed.connect(self.material_attribute_updated)
            self.results_grid_layout.addWidget(self.material_attributes[key], index, 0)
            index += 1

        self.scrollAreaWidgetContents.setGeometry(QtCore.QRect(0, 0, 600, 1000))
        self.scroll_area.setWidget(self.scrollAreaWidgetContents)

        # Process Files Button
        self.process_files_button = QtWidgets.QPushButton('Process Files')
        self.process_files_button.setFont(self.bold_font)
        self.process_files_button.clicked.connect(self.process_files_clicked)
        self.process_files_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.process_files_button.setFixedHeight(60)
        self.process_files_button.setEnabled(False)
        self.content_layout.addWidget(self.process_files_button)

        # Messaging Bar
        self.message_widget = QtWidgets.QWidget()
        self.message_readout_layout = QtWidgets.QHBoxLayout(self.message_widget)
        self.message_readout_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.message_readout_layout.setContentsMargins(0, 0, 0, 0)
        self.message_color_frame = QtWidgets.QFrame(self.message_widget)
        self.message_color_frame.setGeometry(0, 0, 5000, 1000)
        self.message_color_frame.setStyleSheet('background-color:rgb(35, 35, 35);')
        self.message_readout_layout.addSpacing(10)
        self.message_readout_label = QtWidgets.QLabel('Ready.')
        self.message_readout_label.setFixedHeight(30)
        self.message_readout_label.setStyleSheet('color:rgb(0,255,0);')
        self.message_readout_layout.addWidget(self.message_readout_label)
        self.main_container.addWidget(self.message_widget)
        self.initialize_window()

    def initialize_window(self):
        self.set_ui_visibility(False)
        self.initialize_qprocess()
        self.input_path_field.setFocus()

    def process_single_asset(self, target_fbx):
        """
        This function is executed for both individual and directory processing- it separates
        actions into a single process for splitting sources files and generating materials.
        :param target_fbx:
        :return:
        """
        self.process_results[target_fbx.name] = self.start_maya_session(target_fbx)
        if self.use_file_radio_button.isChecked():
            _LOGGER.info(f'Process finished -- Results: {self.process_results}')
            self.display_message('Process Completed.')
            self.app.processEvents()
            time.sleep(5)
            self.display_message('Ready.')
            self.app.processEvents()

    def process_directory(self, directory_path):
        """
        Sets script for directory processing, this function will collect all fbx files found
        in a targeted directory and send each individually to single asset processing above.
        :param directory_path:
        :return:
        """
        _LOGGER.info(f'Processing Directory: {directory_path}')
        for target_file in directory_path.iterdir():
            if target_file.suffix.lower() == '.fbx':
                _LOGGER.info(f'TargetFBX from Process Directory: {target_file}')
                self.process_single_asset(target_file)
        self.display_message('Process Completed.')
        self.app.processEvents()
        time.sleep(5)
        self.display_message('Ready.')
        self.app.processEvents()
        _LOGGER.info(f'Process finished -- Results: {self.process_results}')

    def display_message(self, message):
        """
        Sets informational bar at the bottom of the window that indicates what is being processed.
        :param message:
        :return:
        """
        self.message_readout_label.setText(message)
        self.app.processEvents()

    def initialize_qprocess(self):
        """
        This sets the QProcess object up and adds all of the aspects that will remain consistent for each directory
        processed.
        :return:
        """
        self.p = QProcess()
        env = [env for env in QtCore.QProcess.systemEnvironment() if not env.startswith('PYTHONPATH=')]
        env.append(f'MAYA_LOCATION={os.path.dirname(self.mayapy_path)}')
        env.append(f'PYTHONPATH={os.path.dirname(self.mayapy_path)}')
        self.p.setEnvironment(env)
        self.p.setProgram(str(self.mayapy_path))
        self.p.setProcessChannelMode(QProcess.SeparateChannels)
        self.p.readyReadStandardOutput.connect(self.handle_stdout)
        self.p.readyReadStandardError.connect(self.handle_stderr)
        self.p.stateChanged.connect(self.handle_state)
        self.p.started.connect(self.processStarted)
        self.p.finished.connect(self.cleanup)

    def processStarted(self):
        _LOGGER.info('Maya Standalone Process Started....')

    def handle_stderr(self):
        pass
        # data = self.p.readAllStandardError()
        # stderr = bytes(data).decode("utf8")
        # _LOGGER.info(f'STD_ERROR_FIRED: {stderr}')

    def handle_stdout(self):
        """
        This catches standard output from Maya while processing. It is used for keeping track of progress, and once
        the last FBX file in a target directory is processed, it updates the database with the newly created Maya files.
        :return:
        """
        data = self.p.readAllStandardOutput()
        stdout = bytes(data).decode("utf8")
        if stdout.startswith('{'):
            _LOGGER.info('QProcess Complete.')
            self.processed_material_information = json.loads(stdout)
            self.set_audit_list()

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

    def cleanup(self):
        self.p.closeWriteChannel()
        _LOGGER.info("Source File Conversion Complete")
        self.set_audit_window()
        self.process_files_button.setEnabled(False)

    def start_maya_session(self, target_fbx):
        """
        This starts the exchange between the standalone QT UI and Maya Standalone to process FBX files.
        The information sent to Maya is the FBX file for processing, as well as the base directory and
        relative destination paths. There is a pipe that can be used to shuttle processed information
        back to the script, but at this stage it is not being used.
        :param target_fbx:
        :return:
        """
        self.display_message(f'Working: {target_fbx.name}')
        self.app.processEvents()
        if self.mayapy_path:
            try:
                info_list = [os.path.normpath(target_fbx), self.input_directory, self.relative_export_path]
                script_path = os.path.normpath(os.path.join(os.getcwd(), 'process_fbx_file.py'))
                _LOGGER.info(f'Script path: {script_path}')
                _LOGGER.info(f'Mayapy path: {self.mayapy_path}')
                _LOGGER.info(f'InfoList: {info_list}')
                command = [script_path]
                for entry in info_list:
                    command.append(entry)
                _LOGGER.info(f'++++++ Command: {command}')
                self.p.setArguments(command)
                self.p.start()
                self.p.waitForFinished(-1)
            except Exception as e:
                _LOGGER.warning(f'Error creating Maya Files: {e}')
                return None
        else:
            try:
                mayapy_path = file_browser.getOpenFileName(self, "Please Set Path to \'mayapy.exe\'", self.autodesk_directory,
                                                         'Maya Python Executable (mayapy.exe)')
            except Exception:
                msg = QMessageBox()
                msg.setIcon(QMessageBox.Critical)
                msg.setText("You must have Maya installed to use this script!")
                msg.setWindowTitle("Maya Not Found")
                msg.setStandardButtons(QMessageBox.Ok)

    def export_lumberyard_material(self, output, material_description):
        """
        Takes one final dictionary with information gathered in the process and saves with JSON formatting into a
        .material file

        :param output:
        :param material_description:
        :return:
        """
        with open(output, 'w', encoding='utf-8') as material_file:
            json.dump(material_description, material_file, ensure_ascii=False, indent=4)

    def get_separator_bar(self):
        """ Convenience function for UI layout element """
        self.line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        self.line.setLineWidth(1)
        self.line.setFixedHeight(10)
        self.separator_layout.addWidget(self.line)
        return self.separator_layout

    def get_mayapy_path(self):
        """
        Finds the most current version of Maya for use of its standalone python capabilities.
        :return:
        """
        maya_versions = [int(x.name[-4:]) for x in self.autodesk_directory.iterdir() if x.name.startswith('Maya')]
        if maya_versions:
            target_version = f'Maya{max(maya_versions)}'
            return os.path.normpath(self.autodesk_directory / target_version / 'bin' / 'mayapy.exe')
        else:
            return None

    def get_material_attributes(self):
        attribute_dictionary = {}
        self.default_material_definition = self.get_material_definition(self.default_material_definition)
        for key, values in self.default_material_definition['properties'].items():
            attribute_dictionary[key] = values
        return attribute_dictionary

    def get_material_definition(self, material_file):
        _LOGGER.debug('get_material_definition, os.getcwd() is: {}'.format(os.getcwd()))
        
        # build a resolved absolute path because cwd may be unknown and change running externally
        material_file = Path(settings.DCCSIG_PATH,
                             'SDK', 'Maya', 'Scripts', 'Python', 'kitbash_converter',
                             material_file).resolve()
        
        material_definition = {}
        with open(str(material_file)) as json_file:
            material_definition = json.load(json_file)
        return material_definition

    def get_audit_index(self, target_index):
        fbx_name = self.audit_list[target_index][0]
        fbx_materials = self.audit_list[target_index][1]
        return fbx_name, fbx_materials

    def set_buttons(self):
        self.update_material_button.setEnabled(False)
        if self.current_audit_index == 0:
            self.back_button.setEnabled(False)
            self.forward_button.setEnabled(True)
        elif self.current_audit_index == len(self.audit_list)-1:
            self.forward_button.setEnabled(False)
            self.back_button.setEnabled(True)
        else:
            self.forward_button.setEnabled(True)
            self.back_button.setEnabled(True)


    def set_io_directories(self):
        """
        This is constantly running when user interacts with the source and destination fields in the UI, validating that
        the paths are legitimate. In the event that there are malformed or non-existing paths- it disables the processing
        button.
        :return:
        """
        input_directory = self.input_path_field.text()
        unlocked = True

        if self.use_directory_radio_button.isChecked():
            if os.path.isdir(input_directory.strip()):
                self.input_directory = Path(input_directory.strip())
            else:
                unlocked = False
                # Check Relative Output Path
                output_path = os.path.join(self.input_directory, self.output_directory_button.text())
                if os.path.isdir(output_path):
                    _LOGGER.info('Directory is valid: {}'.format(output_path))
                    unlocked = True
                else:
                    unlocked = False
        else:
            if os.path.isfile(input_directory.strip()):
                target_path = Path(input_directory.strip()).parent / self.output_directory_button.text()
                if target_path.is_dir():
                    self.input_directory = Path(input_directory.strip())
                    unlocked = True
            else:
                unlocked = False

        self.process_files_button.setEnabled(unlocked)
        self.process_files_button.setFocus()

    def set_audit_list(self):
        for key, value in self.processed_material_information.items():
            material_files = []
            for file in os.listdir(value['asset_location']):
                if file.endswith('.material'):
                    material_files.append(os.path.join(value['asset_location'], file))
            self.audit_list.append([key, material_files])
        # Add Materials to Combobox
        self.set_material_combobox(self.current_audit_index)

    def set_audit_window(self, material_override=None):
        self.set_ui_visibility(True)
        fbx_name, fbx_materials = self.get_audit_index(self.current_audit_index)
        material_file = material_override if material_override else fbx_materials[0]
        self.title_label.setText(fbx_name)
        target_definition = self.get_material_definition(material_file)
        _LOGGER.info(f'TargetDefinition:::: {target_definition}')
        for key in self.material_attributes.keys():
            if key in target_definition['properties'].keys():
                _LOGGER.info('++++++++++++++++ TargetDefinition [{}]: {}'.format(key, target_definition['properties'][key]))
                self.material_attributes[key].set_material_attributes(key, target_definition['properties'][key])
            else:
                _LOGGER.info(f'{key} NOT IN Definition Keys!!!!')
                self.material_attributes[key].set_material_attributes(key, None)

    def set_material_combobox(self, object_index):
        self.material_combobox.blockSignals(True)
        self.material_combobox.clear()
        self.material_combobox_items.clear()
        for item in self.audit_list[object_index][1]:
            self.material_combobox_items.append(item.split('\\')[-1])
        self.material_combobox.addItems(self.material_combobox_items)
        self.material_combobox.setCurrentIndex(0)
        self.material_combobox.blockSignals(False)

    def set_ui_visibility(self, is_visible):
        ui_controls = [
            self.back_button,
            self.forward_button,
            self.material_combobox,
            self.update_material_button,
            self.jump_button,
            self.title_label,
            self.line]

        for item in ui_controls:
            item.setVisible(is_visible)

    ##############################
    # Button Actions #############
    ##############################

    def set_directory_button_clicked(self, target):
        """
        Launches a file browser dialog window for specifying file paths

        :param target: Target sets which directory is being set- input or output
        :return:
        """
        if self.input_path_field.text != '':
            if self.use_directory_radio_button.isChecked():
                file_path = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Directory', self.default_search_path,
                                                                       QtWidgets.QFileDialog.ShowDirsOnly)
                if file_path != '':
                    self.input_path_field.setText(file_path) if target == 'input' else self.output_path_field.setText(
                        file_path)
            else:
                if target == 'input':
                    file_browser = QtWidgets.QFileDialog()
                    file_path = file_browser.getOpenFileName(self, 'Set Target File', self.default_search_path,
                                                             'FBX Files (*.fbx)')
                    if file_path != '':
                        self.input_path_field.setText(file_path[0])
                        self.single_asset_file = file_path
                else:
                    file_path = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Directory',
                                                                           self.desktop_location,
                                                                           QtWidgets.QFileDialog.ShowDirsOnly)
                    if file_path != '':
                        self.output_path_field.setText(file_path)
            self.set_io_directories()

    def radio_clicked(self):
        """
        Reconfigures the UI when user toggles between single file and directory processing.
        :return:
        """
        self.input_path_field.setText('')
        signal_sender = self.sender()
        if signal_sender.text() == 'Directory':
            self.input_directory_label.setText('Input Directory')
            input_path = '' if not self.input_directory else str(self.input_directory)
            self.input_path_field.setText(input_path)
            self.file_target_method = 'Directory'
        else:
            self.input_directory_label.setText('Input FBX File')
            input_path = '' if not self.input_directory else str(self.input_directory)
            self.input_path_field.setText(str(self.single_asset_file))
            self.file_target_method = 'File'

    def forward_button_clicked(self):
        new_index = self.current_audit_index + 1
        if new_index <= len(self.audit_list):
            self.current_audit_index = new_index
            self.set_material_combobox(self.current_audit_index)
            self.set_audit_window()
            self.set_buttons()

    def back_button_clicked(self):
        new_index = self.current_audit_index - 1
        if new_index >= 0:
            self.current_audit_index = new_index
            self.set_material_combobox(self.current_audit_index)
            self.set_audit_window()
            self.set_buttons()

    def process_files_clicked(self):
        """
        Starts the conversion process once a target directory/file and output folder has been chosen in the UI
        :return:
        """
        invalid_path = False
        if self.use_directory_radio_button.isChecked():
            directory_path = Path(self.input_path_field.text())
            if os.path.isdir(directory_path):
                self.input_directory = os.path.normpath(directory_path)
                self.process_directory(directory_path)
                self.hidden_button.setFocus()
            else:
                invalid_path = 'Please enter a valid directory path.'

        else:
            fbx_path = self.input_path_field.text()
            if os.path.exists(fbx_path) and fbx_path.endswith('.fbx'):
                self.input_directory = os.path.normpath(os.path.dirname(fbx_path))
                self.process_single_asset(Path(fbx_path))
                self.hidden_button.setFocus()
            else:
                invalid_path = 'Please enter a valid FBX path.'

        if invalid_path:
            msg = QtWidgets.QMessageBox()
            msg.setIcon(QtWidgets.QMessageBox.Warning)
            msg.setText(invalid_path)
            msg.setWindowTitle('Bad Path')
            msg.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msg.exec_()

    def set_relative_path_clicked(self):
        """
        This button action keeps output file locations positioned in the best accessible locations. If the user
        attempts to assign the relative output path to a path thats base is NOT the input directory, a warning
        dialog box will prompt to select a compatible path. The path that is set by default ('Objects/KB3D/CityKit')
        gives the recommended formatted location.
        :return:
        """
        input_path = os.path.normpath(self.input_directory)
        search_path = self.default_search_path if not os.path.isdir(input_path) else input_path
        file_path = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Output Directory', search_path,
                                                               QtWidgets.QFileDialog.ShowDirsOnly)

        if os.path.isfile(file_path):
            file_path = os.path.normpath(Path(file_path).parent)
        else:
            file_path = os.path.normpath(Path(file_path))

        if file_path.index(input_path) != -1:
            self.relative_export_path = file_path.replace('{}\\'.format(input_path), '')
            self.output_directory_button.setText(self.relative_export_path)
        else:
            msg = QtWidgets.QMessageBox()
            msg.setIcon(QtWidgets.QMessageBox.Warning)
            msg.setText('You must choose a subdirectory of \nthe input directory location!')
            msg.setWindowTitle('Bad Location')
            msg.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msg.exec_()

    def jump_button_clicked(self):
        _LOGGER.info('Jump button clicked')
        jumpable_objects = [x[0] for x in self.audit_list]
        self.modal_dialog = JumpDialog(jumpable_objects, self)
        self.modal_dialog.jump_object_selected.connect(self.jump_object_selected)
        self.modal_dialog.show()
        self.modal_launched = True

    def material_combobox_changed(self):
        _LOGGER.info('Material Combobox changed')
        fbx_name, fbx_materials = self.get_audit_index(self.current_audit_index)
        target_material = [y for y in fbx_materials if y.endswith(self.material_combobox.currentText())]
        self.set_audit_window(target_material[0])

    def reset_all_clicked(self):
        msg = QtWidgets.QMessageBox()
        msg.setIcon(QtWidgets.QMessageBox.Warning)
        msg.setText('Resetting the window deletes all recorded file and \nmaterial information. Do you want to continue?')
        msg.setWindowTitle('Confirm Reset')
        msg.setStandardButtons(QtWidgets.QMessageBox.Ok | QtWidgets.QMessageBox.Cancel)
        return_value = msg.exec_()

        if return_value == QtWidgets.QMessageBox.Ok:
            self.scroll_area.verticalScrollBar().setValue(0)
            for value in self.material_attributes.values():
                value.setVisible(False)
            self.current_audit_index = 0
            self.audit_list = []
            self.set_buttons()
            self.process_files_button.setEnabled(True)
            self.processed_material_information = {}
            self.set_ui_visibility(False)
            self.input_path_field.setText('')

    def update_material_clicked(self):
        _LOGGER.info('Update material clicked')
        fbx_name, fbx_materials = self.get_audit_index(self.current_audit_index)
        destination_file = [x for x in fbx_materials if x.endswith(self.material_combobox.currentText())]
        material_definition = {}
        for key, value in self.default_material_definition.items():
            if key != 'properties':
                material_definition[key] = value
            else:
                temp_dict = {}
                for k, v in value.items():
                    target_attribute = self.material_attributes[k]
                    if target_attribute.custom_attribute_values:
                        temp_dict[k] = target_attribute.get_update_values()
                material_definition[key] = temp_dict
        _LOGGER.info('Updated Material Definition: {}'.format(json.dumps(material_definition, indent=4, sort_keys=False)))
        _LOGGER.info(f'Destination: {destination_file[0]}')
        self.export_lumberyard_material(destination_file[0], material_definition)


    @Slot(str)
    def jump_object_selected(self, selected_object):
        self.modal_launched = False
        for index, listing in enumerate(self.audit_list):
            if listing[0] == selected_object:
                self.current_audit_index = index
                self.set_material_combobox(self.current_audit_index)
                self.set_audit_window()
                self.set_buttons()

    @Slot(bool)
    def material_attribute_updated(self):
        _LOGGER.info('MaterialAttributeUpdated')
        self.update_material_button.setEnabled(True)
        self.hidden_button.setFocus()


class JumpDialog(QtWidgets.QDialog):
    jump_object_selected = Signal(str)

    def __init__(self, object_list, parent=None):
        super(JumpDialog, self).__init__(parent)

        self.setWindowTitle('Jump To FBX')
        self.setObjectName('JumpToFbx')
        self.object_list = object_list
        self.main_layout = QtWidgets.QVBoxLayout()
        popupWidth = 300
        popupHeight = 300
        self.setGeometry(200, 200, popupWidth, popupHeight)
        self.verticalContainer = QtWidgets.QVBoxLayout(self)
        self.setModal(True)

        # Filter Find Widget
        self.filter_find_layout = QtWidgets.QHBoxLayout()
        self.verticalContainer.addLayout(self.filter_find_layout)
        self.filter_label = QtWidgets.QLabel('Filter:')
        self.filter_entry_field = QtWidgets.QLineEdit()
        self.filter_entry_field.textChanged.connect(self.filter_object_list)
        self.filter_entry_field.setFixedHeight(24)
        self.filter_find_layout.addWidget(self.filter_label)
        self.filter_find_layout.addWidget(self.filter_entry_field)

        # Separation Line
        self.separatorLayout1 = QtWidgets.QHBoxLayout()
        self.line1 = QtWidgets.QLabel()
        self.line1.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        self.line1.setLineWidth(1)
        self.line1.setFixedHeight(10)
        self.separatorLayout1.addWidget(self.line1)
        self.verticalContainer.addLayout(self.separatorLayout1)

        # ASIN Source Table
        self.search_table = QtWidgets.QTableWidget()
        self.search_table.setColumnCount(1)
        self.search_table.horizontalHeader().setStretchLastSection(True)
        self.search_table.horizontalHeader().hide()
        self.search_table.verticalHeader().hide()
        self.verticalContainer.addWidget(self.search_table)

        # Jump Button
        self.jump_button = QtWidgets.QPushButton('Jump to Selected')
        self.jump_button.clicked.connect(self.jump_button_clicked)
        self.jump_button.setFixedHeight(40)
        self.verticalContainer.addWidget(self.jump_button)
        self.populate_search_table()

    def populate_search_table(self):
        for index, entry in enumerate(self.object_list):
            self.search_table.insertRow(index)
            item = QtWidgets.QTableWidgetItem(entry)
            item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
            self.search_table.setItem(index, 0, item)
            if index % 2 == 0:
                self.search_table.item(index, 0).setBackground(QtGui.QColor(240, 240, 240))
        self.search_table.resizeRowsToContents()

    def filter_object_list(self):
        '''
        Used for long listings of FBX objects for finding target folders more quickly
        '''
        self.search_table.setRowCount(0)
        for fbx_object in self.object_list:
            if self.filter_entry_field.text() in fbx_object:
                count = self.search_table.rowCount()
                self.search_table.insertRow(count)
                item = QtWidgets.QTableWidgetItem(fbx_object)
                self.search_table.setItem(count, 0, item)
                if count % 2 == 0:
                    self.search_table.item(count, 0).setBackground(QtGui.QColor(240, 240, 240))
        self.search_table.resizeRowsToContents()

    def jump_button_clicked(self):
        selected_object = self.search_table.selectedItems()
        target_object = selected_object[0].text()
        self.jump_object_selected.emit(target_object)
        self.close()


class AttributeListing(QtWidgets.QWidget):
    ''' This creates a search result entry in the combiner tab '''
    attribute_changed = Signal(bool)

    def __init__(self, attribute_values, parent=None):
        super(AttributeListing, self).__init__(parent)
        self.attribute_values = None
        self.custom_attribute_values = {}
        self.bold_font = QtGui.QFont("Helvetica", 8, QtGui.QFont.Black)
        self.attribute_values = attribute_values
        self.listing_layout = QtWidgets.QVBoxLayout(self)
        self.listing_layout.addStretch(1)
        self.listing_layout.setAlignment(QtCore.Qt.AlignTop)
        self.message_color_frame = QtWidgets.QFrame(self)
        self.message_color_frame.setGeometry(0, 0, 5000, 5000)
        self.message_color_frame.setStyleSheet('background-color:rgb(125, 125, 125);')
        self.listing_title = QtWidgets.QLabel(self.get_attribute_title())
        self.listing_title.setFont(self.bold_font)
        self.listing_title.setStyleSheet('color: white;')
        self.listing_layout.addWidget(self.listing_title)
        self.attributes_table = QtWidgets.QTableWidget()
        self.attributes_table.itemChanged.connect(self.table_item_update)
        self.attributes_table.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.attributes_table.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.attributes_table.setColumnCount(2)
        self.attributes_table.setFocusPolicy(QtCore.Qt.NoFocus)
        header = self.attributes_table.horizontalHeader()
        header.setFont(self.bold_font)
        header.setStyleSheet('color: rgb(125, 125, 125);')
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        vertical_header = self.attributes_table.verticalHeader()
        vertical_header.setVisible(False)
        self.attributes_table.setHorizontalHeaderLabels(['  Attribute Name  ', 'Attribute Value'])
        self.attributes_table.setAlternatingRowColors(True)
        self.listing_layout.addWidget(self.attributes_table)
        self.setVisible(False)

    def populate_attributes_table(self):
        """
        Feeds texture information into the texture table of the UI when inspecting FBX asset information
        :param table_items: The texture list to be displayed- contains PBR texture type and file paths
        :return:
        """
        self.attributes_table.setRowCount(0)
        self.attributes_table.blockSignals(True)
        row = 0
        for key, value in self.attribute_values[1].items():
            self.attributes_table.insertRow(row)
            self.attributes_table.setRowHeight(row, 25)
            for column in range(2):
                custom_value = False
                cell_value = key if column == 0 else str(value)
                bold_font = False
                item = QtWidgets.QTableWidgetItem(cell_value)
                item.setTextAlignment(QtCore.Qt.AlignHCenter | QtCore.Qt.AlignVCenter | QtCore.Qt.AlignCenter)
                if column == 0:
                    item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                else:
                    if self.custom_attribute_values:
                        if key in self.custom_attribute_values.keys():
                            _LOGGER.info(f'KEY MATCH FOUND=====================>> {key}')
                            font = QtGui.QFont()
                            font.setBold(True)
                            item.setFont(font)
                            item.setText(str(self.custom_attribute_values[key]))
                            custom_value = True
                self.attributes_table.setItem(row, column, item)
                if custom_value:
                    self.attributes_table.item(row, column).setBackground(QtGui.QColor(255, 0, 255))
                    self.attributes_table.item(row, column).setForeground(QtGui.QColor(255, 255, 255))

            row += 1
        self.attributes_table.setMinimumSize(self.get_minimum_table_size())
        self.attributes_table.blockSignals(False)

    def set_material_attributes(self, fbx_name, custom_attribute_values):
        self.fbx_name = fbx_name
        self.custom_attribute_values = custom_attribute_values if custom_attribute_values else None
        self.populate_attributes_table()
        self.setVisible(True)

    def get_minimum_table_size(self):
        w = 150
        h = self.attributes_table.horizontalHeader().height()
        for i in range(self.attributes_table.rowCount()):
            h += self.attributes_table.rowHeight(i)
        return QtCore.QSize(w, h)

    def get_attribute_title(self):
        title_dictionary = {
            'general': 'General',
            'ambientOcclusion': 'Ambient Occlusion',
            'baseColor': 'Base Color',
            'emissive': 'Emission',
            'metallic': 'Metallic',
            'roughness': 'Roughness',
            'specularF0': 'SpecularF0',
            'normal': 'Normal',
            'subsurfaceScattering': 'Subsurface Scattering',
            'opacity': 'Opacity',
            'uv': 'UV'
        }
        return title_dictionary[self.attribute_values[0]]

    def get_update_values(self):
        _LOGGER.info(f'Getting Update Values for: {self.attribute_values[0]}')
        if self.custom_attribute_values:
            for key, value in self.custom_attribute_values.items():
                self.custom_attribute_values[key] = self.format_material_value(value)
        return self.custom_attribute_values

    def format_material_value(self, value):
        """
        In order for the formatting of the information to work properly once converted into a JSON-like material
        definition, information needs to be properly formatted... not all information can be gathered as strings. This
        function reviews the format of the information and makes changes if necessary for the information to carry
        over properly in the definition.
        :param value: The value under review for formatting issues
        :return:
        """
        if isinstance(value, list):
            converted_list = []
            for item in value:
                item_value = 0 if item is 0 else float(item)
                converted_list.append(item)
            return converted_list
        elif isinstance(value, str):
            if value.lower() in ['true', 'false']:
                return bool(value.capitalize())
            elif value.isnumeric():
                return 0 if value is 0 else float(value)
            else:
                pattern = r'\[[^\]]*\]'
                test_brackets = re.sub(pattern, '', value)
                if test_brackets[0].isnumeric():
                    return float(test_brackets)
                else:
                    _LOGGER.info(f'Unknown Value Cannot Be Formatted: [{value}] --->> Type: {type(value)}')
                return value
        else:
            return value


    def table_item_update(self):
        self.attribute_changed.emit(True)
        current_row = self.attributes_table.currentRow()
        target_attribute = self.attributes_table.item(current_row, 0).text()
        _LOGGER.info(f'Item changed===> {target_attribute}')
        default_value = self.attribute_values[1][target_attribute]
        new_value = self.attributes_table.item(current_row, 1).text()
        _LOGGER.info(f'TargetAttribute: {target_attribute} --> NewValue: {new_value}   DefaultValue: {default_value}')
        if str(new_value) != str(default_value):
            if not self.custom_attribute_values:
                self.custom_attribute_values = {target_attribute: new_value}
            else:
                self.custom_attribute_values[target_attribute] = new_value
        else:
            if target_attribute in self.custom_attribute_values.keys():
                del self.custom_attribute_values[target_attribute]
        self.populate_attributes_table()


def launch_kitbash_converter():
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    converter = FBXConverter()
    converter.show()
    sys.exit(app.exec_()) 


if __name__ == '__main__':
    launch_kitbash_converter()
