# coding:utf-8
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
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



#  built-ins
import collections
from collections import abc
import subprocess
import logging as _logging
try:
    import pathlib
except:
    import pathlib2 as pathlib
from pathlib import *
import shelve
# import socket
import shutil
import math
import json
import time
import csv
import sys
import os
import re
from functools import partial
import xml.etree.ElementTree as ET
from shutil import copyfile

# azpy bootstrapping and extensions
import azpy.config_utils

_config = azpy.config_utils.get_dccsi_config()
print(_config)
# ^ this is effectively an import and retrieve of <dccsi>\config.py
# and init's access to Qt/Pyside2
# init lumberyard Qy/PySide2 access
# now default settings are extended with PySide2
# this is an alternative to "from dynaconf import settings" with Qt
settings = _config.get_config_settings(enable_o3de_pyside2=True)


# 3rd Party (we may or do provide)
from box import Box
import OpenImageIO as oiio
from OpenImageIO import ImageInput, ImageOutput
from OpenImageIO import ImageBuf, ImageSpec, ImageBufAlgo
# lumberyard Qt/PySide2
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtWidgets import QApplication
from PySide2.QtCore import Signal, Slot, QThread, QProcess, QProcessEnvironment
# local tool imports
import lumberyard_data
import image_conversion
import constants
import utilities

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

module_name = 'legacy_asset_converter.main'
log_file_path = os.path.join(settings.DCCSI_LOG_PATH, f'{module_name}.log')
if not os.path.exists(settings.DCCSI_LOG_PATH):
    os.makedirs(settings.DCCSI_LOG_PATH, exist_ok=True)

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


class LegacyFilesConverter(QtWidgets.QDialog):

    def __init__(self, parent=None):
        super(LegacyFilesConverter, self).__init__(parent)

        self.app = QtWidgets.QApplication.instance()

        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 600, 800)
        self.setObjectName('LegacyFilesConverter')
        self.setWindowTitle('Legacy Converter')
        self.isTopLevel()
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowMinMaxButtonsHint)
        self.desktop_location = os.path.join(os.path.expanduser('~'), 'Desktop')
        self.local_host = '127.0.0.1'
        self.port = 54321
        self.p = None
        self.modal_dialog = None
        self.logger_enabled = True
        self.image_key_dictionary = {
            'ddn': ['ddn'],
            'ddna': ['ddna'],
            'diffuse': ['diffuse', 'diff', 'd'],
            'emissive': ['emis', 'emissive', 'e', 'emiss'],
            'specular': ['spec', 'specular'],
            'scattering': ['scattering', 'sss'],
            'normal': ['normal'],
            'basecolor': ['basecolor', 'albedo']
        }
        self.legacy_key_identifiers = {'Diffuse': 'diff', 'Bumpmap': 'ddna', 'Specular': 'spec'}
        self.autodesk_directory = Path(os.environ['ProgramFiles']) / 'Autodesk'
        self.headers = ('dirname dirpath fbxname mayafile designer diff ddna spec emi basecolor metallic normal '
                        'emissive status synced').split(' ')
        self.initialization_files = ['workspace.mel', 'Launch_Cmd.bat', 'Launch_Maya_2020.bat', 'Launch_WingIDE-7-1.bat',
                                     'Project_Env.bat']
        self.mayapy_path = None
        self.single_asset_file = ''
        self.input_directory = None
        self.output_directory = None
        self.input_file = None
        self.load_events = None
        self.image_output_type = '.tif'
        self.directory_audit = Box({})
        self.section_data = {}
        self.template_lumberyard_material = {}
        self.default_material_definition = 'standardPBR.template.material'
        self.log_file_location = os.path.join(Path.cwd(), 'output.log')
        self.directory_dictionary = {}
        self.materials_dictionary = {}
        self.materials_db = {} #shelve.open('materialsdb', protocol=2) # Do not save materialDB on disk as can lead to issue on reopen
        self.file_target_method = 'Directory'
        self.selected_directory_index = -1
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QLabel()

        self.small_font = QtGui.QFont("Helvetica", 8, QtGui.QFont.Normal)
        self.medium_font = QtGui.QFont("Helvetica", 10, QtGui.QFont.Normal)
        self.bold_font = QtGui.QFont("Arial", 8, QtGui.QFont.Black)
        self.bold_font_large = QtGui.QFont("Helvetica", 12, QtGui.QFont.Bold)
        self.title_font = QtGui.QFont("Helvetica", 22, QtGui.QFont.Bold)
        self.setStyleSheet('QWidget {font: "Helvetica"}')

        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setContentsMargins(10, 10, 10, 0)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.setLayout(self.main_container)
        self.content_layout = QtWidgets.QVBoxLayout()
        self.content_layout.setAlignment(QtCore.Qt.AlignTop)
        self.main_container.addLayout(self.content_layout)

        # Set Directory Path
        self.target_directory_layout = QtWidgets.QVBoxLayout()
        self.target_directory_layout.setAlignment(QtCore.Qt.AlignTop)
        self.target_directory_groupbox = QtWidgets.QGroupBox("Target Files")
        self.target_directory_groupbox.setLayout(self.target_directory_layout)

        # ---- Input Directory
        self.input_field_layout = QtWidgets.QHBoxLayout()
        self.input_directory_label = QtWidgets.QLabel('Input Directory')
        self.input_directory_label.setFont(self.bold_font)
        self.input_directory_label.setFixedWidth(80)
        self.input_field_layout.addWidget(self.input_directory_label)
        self.input_path_field = QtWidgets.QLineEdit('E:/LY/spectra_atom/dev/Gems/AtomStarterGame/Environment/Assets/Objects/AAGun')
        self.input_path_field.textChanged.connect(self.set_io_directories)
        self.input_path_field.setFixedHeight(25)
        self.input_field_layout.addWidget(self.input_path_field)
        self.set_input_button = QtWidgets.QPushButton('Set')
        self.set_input_button.clicked.connect(partial(self.set_directory_button_clicked, 'input'))
        self.set_input_button.setFixedSize(40, 25)
        self.input_field_layout.addWidget(self.set_input_button)
        self.target_directory_layout.addLayout(self.input_field_layout)

        # ---- Output Directory
        self.output_field_layout = QtWidgets.QHBoxLayout()
        self.output_directory_label = QtWidgets.QLabel('Output Directory')
        self.output_directory_label.setFont(self.bold_font)
        self.output_directory_label.setFixedWidth(80)
        self.output_field_layout.addWidget(self.output_directory_label)
        self.output_path_field = QtWidgets.QLineEdit('C:/Users/benblac/Desktop/test')
        self.output_path_field.textChanged.connect(self.set_io_directories)
        self.output_path_field.setFixedHeight(25)
        self.output_field_layout.addWidget(self.output_path_field)
        self.set_output_button = QtWidgets.QPushButton('Set')
        self.set_output_button.clicked.connect(partial(self.set_directory_button_clicked, 'output'))
        self.set_output_button.setFixedSize(40, 25)
        self.output_field_layout.addWidget(self.set_output_button)
        self.target_directory_layout.addLayout(self.output_field_layout)

        self.file_options_layout = QtWidgets.QHBoxLayout()
        self.radio_button_layout = QtWidgets.QHBoxLayout()
        self.radio_button_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.radio_button_group = QtWidgets.QButtonGroup()
        self.use_directory_radio_button = QtWidgets.QRadioButton('Use Directory')
        self.use_directory_radio_button.setChecked(True)
        self.use_directory_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_group.addButton(self.use_directory_radio_button)
        self.radio_button_layout.addWidget(self.use_directory_radio_button)
        self.radio_button_layout.addSpacing(10)
        self.use_file_radio_button = QtWidgets.QRadioButton('Single File')
        self.use_file_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_layout.addWidget(self.use_file_radio_button)

        self.radio_button_group.addButton(self.use_file_radio_button)
        self.target_directory_layout.addSpacing(5)
        self.file_options_layout.addLayout(self.radio_button_layout)
        self.target_directory_layout.addLayout(self.file_options_layout)
        self.main_container.addWidget(self.target_directory_groupbox)

        # Set Actions
        self.actions_layout = QtWidgets.QHBoxLayout()
        self.actions_layout.setAlignment(QtCore.Qt.AlignTop)
        self.actions_groupbox = QtWidgets.QGroupBox("Actions")
        self.actions_groupbox.setLayout(self.actions_layout)

        # Left Column
        self.left_checkbox_items = QtWidgets.QVBoxLayout()
        self.left_checkbox_items.setAlignment(QtCore.Qt.AlignTop)
        self.actions_layout.addLayout(self.left_checkbox_items)
        self.convert_textures_checkbox = QtWidgets.QCheckBox('Convert Legacy Textures')
        self.convert_textures_checkbox.setChecked(True)
        self.left_checkbox_items.addWidget(self.convert_textures_checkbox)
        self.create_material_files_checkbox = QtWidgets.QCheckBox('Create .material files')
        self.create_material_files_checkbox.setChecked(True)
        self.left_checkbox_items.addWidget(self.create_material_files_checkbox)
        self.create_maya_files_checkbox = QtWidgets.QCheckBox('Create Maya Files')
        self.left_checkbox_items.addWidget(self.create_maya_files_checkbox)

        # Center Column
        self.center_checkbox_items = QtWidgets.QVBoxLayout()
        self.center_checkbox_items.setAlignment(QtCore.Qt.AlignTop)
        self.actions_layout.addLayout(self.center_checkbox_items)
        self.create_logs_checkbox = QtWidgets.QCheckBox('Create Output Logs')
        self.create_logs_checkbox.setChecked(True)
        self.center_checkbox_items.addWidget(self.create_logs_checkbox)
        self.create_checklist_checkbox = QtWidgets.QCheckBox('Create Asset Checklists')
        self.create_checklist_checkbox.setChecked(True)
        self.center_checkbox_items.addWidget(self.create_checklist_checkbox)
        self.reprocess_checkbox = QtWidgets.QCheckBox('Reprocess Existing Files')
        self.center_checkbox_items.addWidget(self.reprocess_checkbox)

        # Right Column
        self.right_checkbox_items = QtWidgets.QVBoxLayout()
        self.right_checkbox_items.setAlignment(QtCore.Qt.AlignTop)
        self.actions_layout.addLayout(self.right_checkbox_items)
        self.clean_material_names_checkbox = QtWidgets.QCheckBox('Clean Material Names')
        self.clean_material_names_checkbox.setChecked(False)
        self.right_checkbox_items.addWidget(self.clean_material_names_checkbox)
        self.skip_white_texture_materials = QtWidgets.QCheckBox('Skip White Texture Materials')
        self.skip_white_texture_materials.setChecked(True)
        self.right_checkbox_items.addWidget(self.skip_white_texture_materials)
        self.main_container.addWidget(self.actions_groupbox)

        self.process_files_button = QtWidgets.QPushButton('Process Files')
        self.process_files_button.clicked.connect(self.process_files_clicked)
        self.process_files_button.setFixedHeight(40)
        self.process_files_button.setEnabled(False)
        self.main_container.addWidget(self.process_files_button)
        self.main_container.addLayout(self.get_separator_bar())

        ####################################
        # Stacked Layout ###################
        ####################################

        self.window_tabs = QtWidgets.QTabWidget()
        self.main_container.addWidget(self.window_tabs)

        # +++++++++++++++++++---->>
        # DATA TAB --------------->>
        # +++++++++++++++++++---->>

        # Data tab base
        self.audit_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.audit_tab_content, 'Data')
        self.audit_layout = QtWidgets.QVBoxLayout(self.audit_tab_content)
        self.audit_layout.setAlignment(QtCore.Qt.AlignTop)
        self.audit_layout.setContentsMargins(10, 10, 10, 10)
        self.audit_color_frame = QtWidgets.QFrame(self.audit_tab_content)
        self.audit_color_frame.setGeometry(0, 0, 5000, 5000)
        self.audit_color_frame.setStyleSheet('background-color:rgb(235, 235, 235);')

        ##############################
        # Data Display Component
        ##############################

        self.data_results_layout = QtWidgets.QHBoxLayout()
        self.audit_layout.addLayout(self.data_results_layout)

        # Display target combobox
        self.display_combobox_layout = QtWidgets.QVBoxLayout()
        self.display_combobox_layout.setAlignment(QtCore.Qt.AlignTop)
        self.display_combobox = QtWidgets.QComboBox()
        self.display_combobox.setFixedSize(125, 25)

        self.display_items = ('Show All', 'Show By Directory', 'Single FBX File')
        self.display_combobox.addItems(self.display_items)
        self.display_combobox.currentIndexChanged.connect(self.display_combobox_changed)
        self.display_combobox_layout.addWidget(self.display_combobox)
        self.data_results_layout.addLayout(self.display_combobox_layout)

        # Display stacked secondary
        self.results_stacked_widget = QtWidgets.QStackedWidget()
        self.data_results_layout.addWidget(self.results_stacked_widget)
        self.results_stacked_widget.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Maximum)

        # Directory combobox
        self.combobox_results_widget = QtWidgets.QWidget()
        self.combobox_results_widget.setFixedHeight(25)
        self.combobox_results_layout = QtWidgets.QVBoxLayout(self.combobox_results_widget)
        self.combobox_results_layout.setContentsMargins(7, 0, 0, 0)
        self.directory_combobox = QtWidgets.QComboBox()
        self.directory_combobox.setEnabled(False)
        self.directory_combobox.setFixedHeight(25)
        self.directory_combobox.currentIndexChanged.connect(self.directory_combobox_changed)
        self.directories_items = ('')
        self.combobox_results_layout.addWidget(self.directory_combobox)
        self.results_stacked_widget.addWidget(self.combobox_results_widget)

        # Search Widget
        self.search_results_widget = QtWidgets.QWidget()
        self.container = QtWidgets.QVBoxLayout()
        self.container.setContentsMargins(0, 0, 0, 0)
        self.search_results_layout = QtWidgets.QHBoxLayout()
        self.container.addLayout(self.search_results_layout)
        self.search_results_layout.setAlignment(QtCore.Qt.AlignRight)
        self.search_results_layout.setContentsMargins(10, 0, 0, 0)
        self.search_results_widget.setLayout(self.container)
        self.find_field = QtWidgets.QLineEdit()
        self.find_field.setFixedSize(250, 25)
        self.search_results_layout.addWidget(self.find_field)
        self.search_button = QtWidgets.QPushButton('Search')
        self.search_results_layout.addWidget(self.search_button)
        self.search_button.clicked.connect(self.search_button_clicked)
        self.search_button.setFixedSize(50, 27)
        self.results_stacked_widget.addWidget(self.search_results_widget)

        self.asset_data_layout = QtWidgets.QVBoxLayout()
        self.asset_data_window = QtWidgets.QTextEdit('Ready.')
        self.asset_data_layout.addWidget(self.asset_data_window)
        self.audit_layout.addLayout(self.asset_data_layout)

        self.progress_bar_layout = QtWidgets.QHBoxLayout()
        self.progress_bar = QtWidgets.QProgressBar()
        self.progress_bar.setFixedHeight(25)
        self.progress_bar.setTextVisible(False)
        self.progress_bar_layout.addWidget(self.progress_bar)
        self.audit_layout.addLayout(self.progress_bar_layout)

        # +++++++++++++++++++---->>
        # ASSET TAB --------------->>
        # +++++++++++++++++++---->>

        self.asset_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.asset_tab_content, 'Asset')
        self.asset_layout = QtWidgets.QVBoxLayout(self.asset_tab_content)
        self.asset_layout.setContentsMargins(10, 10, 10, 10)
        self.asset_layout.setSpacing(0)
        self.asset_layout.setAlignment(QtCore.Qt.AlignTop)
        self.asset_color_frame = QtWidgets.QFrame(self.asset_tab_content)
        self.asset_color_frame.setGeometry(0, 0, 5000, 5000)
        self.asset_color_frame.setStyleSheet('background-color:rgb(235, 235, 235);')

        # Panel Content ###########
        self.panel_layout = QtWidgets.QVBoxLayout()
        self.asset_layout.addLayout(self.panel_layout)

        # Directory
        self.file_directory_layout = QtWidgets.QHBoxLayout()
        self.panel_layout.addLayout(self.file_directory_layout)
        self.directory_listing_layout = QtWidgets.QHBoxLayout()
        self.directory_listing_layout.setAlignment(QtCore.Qt.AlignTop)
        self.directory_groupbox = QtWidgets.QGroupBox("Directory")
        self.directory_groupbox.setLayout(self.directory_listing_layout)
        self.asset_directory_combobox = QtWidgets.QComboBox()
        self.asset_directory_combobox.currentIndexChanged.connect(self.asset_directory_combobox_changed)
        self.asset_directory_combobox.setFixedHeight(25)
        self.directory_listing_layout.addWidget(self.asset_directory_combobox)
        self.file_directory_layout.addWidget(self.directory_groupbox)
        self.file_directory_layout.addSpacing(7)

        # Material Files
        self.material_files_layout = QtWidgets.QHBoxLayout()
        self.material_files_layout.setAlignment(QtCore.Qt.AlignTop)
        self.material_files_groupbox = QtWidgets.QGroupBox("Material Files")
        self.material_files_groupbox.setLayout(self.material_files_layout)
        self.material_files_combobox = QtWidgets.QComboBox()
        self.material_files_items = ['Choose']
        self.material_files_combobox.addItems(self.material_files_items)
        self.material_files_combobox.setFixedHeight(25)
        self.material_files_combobox.setStyleSheet('QComboBox:item {padding-left:30px;}')
        self.material_files_layout.addWidget(self.material_files_combobox)
        self.material_file_open_button = QtWidgets.QPushButton('Open')
        self.material_file_open_button.setFixedSize(40, 27)
        self.material_file_open_button.clicked.connect(self.open_material_clicked)
        self.material_files_layout.addWidget(self.material_file_open_button)
        self.file_directory_layout.addWidget(self.material_files_groupbox)

        # FBX File
        self.maya_fbx_combo_layout = QtWidgets.QHBoxLayout()
        self.panel_layout.addLayout(self.maya_fbx_combo_layout)
        self.fbx_layout = QtWidgets.QVBoxLayout()
        self.fbx_layout.setAlignment(QtCore.Qt.AlignTop)
        self.fbx_groupbox = QtWidgets.QGroupBox("FBX File")
        self.fbx_groupbox.setLayout(self.fbx_layout)
        self.fbx_combobox = QtWidgets.QComboBox()
        self.fbx_combobox.setFixedHeight(25)
        self.fbx_combobox.currentIndexChanged.connect(self.fbx_combobox_changed)
        self.fbx_layout.addWidget(self.fbx_combobox)
        self.maya_fbx_combo_layout.addWidget(self.fbx_groupbox)
        self.maya_fbx_combo_layout.addSpacing(7)

        # Maya File
        self.maya_file_layout = QtWidgets.QHBoxLayout()
        self.maya_file_layout.setAlignment(QtCore.Qt.AlignTop)
        self.maya_file_groupbox = QtWidgets.QGroupBox("Maya File")
        self.maya_file_groupbox.setLayout(self.maya_file_layout)
        self.maya_file_label = QtWidgets.QLabel()
        self.maya_file_label.setStyleSheet('color:blue')
        self.maya_file_layout.addWidget(self.maya_file_label)
        self.maya_open_button = QtWidgets.QPushButton('Open')
        self.maya_open_button.setFixedSize(40, 27)
        self.maya_file_layout.addWidget(self.maya_open_button)
        self.maya_fbx_combo_layout.addWidget(self.maya_file_groupbox)

        # File Textures
        self.file_textures_layout = QtWidgets.QHBoxLayout()
        self.texture_table_layout = QtWidgets.QVBoxLayout()
        self.file_textures_layout.addLayout(self.texture_table_layout)
        self.file_textures_layout.setAlignment(QtCore.Qt.AlignTop)
        self.file_textures_groupbox = QtWidgets.QGroupBox("File Textures")
        self.file_textures_groupbox.setLayout(self.file_textures_layout)

        # File Textures Table
        self.file_textures_table = QtWidgets.QTableWidget()
        self.file_textures_table.setColumnCount(2)
        self.file_textures_table.setAlternatingRowColors(True)
        self.file_textures_table.setSelectionBehavior(QtWidgets.QTableView.SelectRows)
        self.file_textures_table.setHorizontalHeaderLabels(['Texture Type', 'Texture Name'])
        self.file_textures_table.horizontalHeader().setStyleSheet('QHeaderView::section '
                                                                  '{background-color: rgb(220, 220, 220); '
                                                                  'padding-top:7px; padding-left:5px;}')
        vertical_header = self.file_textures_table.verticalHeader()
        vertical_header.setSectionResizeMode(QtWidgets.QHeaderView.Fixed)
        vertical_header.setDefaultSectionSize(40)
        vertical_header.hide()
        files_header = self.file_textures_table.horizontalHeader()
        files_header.setFont(self.small_font)
        files_header.setDefaultAlignment(QtCore.Qt.AlignLeft)
        files_header.setContentsMargins(0, 0, 0, 0)
        files_header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        self.texture_table_layout.addWidget(self.file_textures_table)

        self.textures_button_layout = QtWidgets.QVBoxLayout()
        self.directory_button_layout = QtWidgets.QVBoxLayout()
        self.directory_button_layout.setAlignment(QtCore.Qt.AlignBottom)
        self.textures_button_layout.addLayout(self.directory_button_layout)
        self.texture_directory_button = QtWidgets.QPushButton('Directory')
        self.texture_directory_button.setFixedSize(90, 40)
        self.texture_directory_button.clicked.connect(self.goto_directory_clicked)
        self.directory_button_layout.addWidget(self.texture_directory_button)
        self.textures_button_layout.addSpacing(3)

        self.selected_button_layout = QtWidgets.QVBoxLayout()
        self.selected_button_layout.setAlignment(QtCore.Qt.AlignTop)
        self.textures_button_layout.addLayout(self.selected_button_layout)
        self.texture_open_button = QtWidgets.QPushButton('Open Selected')
        self.texture_open_button.setFixedSize(90, 40)
        self.texture_open_button.clicked.connect(self.texture_open_clicked)
        self.selected_button_layout.addWidget(self.texture_open_button)
        self.file_textures_layout.addLayout(self.textures_button_layout)
        self.panel_layout.addWidget(self.file_textures_groupbox)

        # Run Script
        self.checklist_script_combo_layout = QtWidgets.QHBoxLayout()
        self.panel_layout.addLayout(self.checklist_script_combo_layout)
        self.run_script_layout = QtWidgets.QHBoxLayout()
        self.run_script_groupbox = QtWidgets.QGroupBox("Run Script")
        self.run_script_groupbox.setLayout(self.run_script_layout)
        self.run_script_combobox = QtWidgets.QComboBox()
        self.run_script_items = ['Attach Textures', 'Create ID Maps', 'Create Metallic Map', 'Set Maya Workspace',
                                 'Archive Asset']
        self.run_script_combobox.addItems(self.run_script_items)
        self.run_script_combobox.setFixedHeight(25)
        self.run_script_combobox.setStyleSheet('QComboBox:item {padding-left:30px;}')
        self.run_script_layout.addWidget(self.run_script_combobox)
        self.run_button = QtWidgets.QPushButton('Run')
        self.run_button.clicked.connect(self.run_script_clicked)
        self.run_button.setFixedSize(40, 27)
        self.run_script_layout.addWidget(self.run_button)
        self.checklist_script_combo_layout.addWidget(self.run_script_groupbox)
        self.checklist_script_combo_layout.addSpacing(7)

        # Edit Checklist
        self.checklist_layout = QtWidgets.QHBoxLayout()
        self.checklist_layout.setAlignment(QtCore.Qt.AlignTop)
        self.checklist_groupbox = QtWidgets.QGroupBox("Update")
        self.checklist_groupbox.setLayout(self.checklist_layout)
        self.update_spreadsheet_button = QtWidgets.QPushButton('Spreadsheet')
        self.update_spreadsheet_button.setFixedHeight(27)
        self.update_date_button = QtWidgets.QPushButton('Update All')
        self.update_date_button.setFixedHeight(27)
        self.checklist_layout.addWidget(self.update_spreadsheet_button)
        self.checklist_layout.addWidget(self.update_date_button)
        self.checklist_script_combo_layout.addWidget(self.checklist_groupbox)
        self.checklist_script_combo_layout.addSpacing(10)

        # Navigate Listings
        self.navigate_layout = QtWidgets.QHBoxLayout()
        self.navigate_groupbox = QtWidgets.QGroupBox("Cycle Listings")
        self.navigate_groupbox.setLayout(self.navigate_layout)
        self.navigate_groupbox.setFixedWidth(105)
        self.back_button = QtWidgets.QToolButton()
        self.back_button.setFixedSize(37, 27)
        self.back_button.setArrowType(QtCore.Qt.LeftArrow)
        self.back_button.clicked.connect(self.back_button_clicked)
        self.forward_button = QtWidgets.QToolButton()
        self.forward_button.setFixedSize(37, 27)
        self.forward_button.setArrowType(QtCore.Qt.RightArrow)
        self.forward_button.clicked.connect(self.forward_button_clicked)
        self.navigate_layout.addWidget(self.back_button)
        self.navigate_layout.addWidget(self.forward_button)
        self.checklist_script_combo_layout.addWidget(self.navigate_groupbox)

        # +++++++++++++++++++---->>
        # UTILITY TAB --------------->>
        # +++++++++++++++++++---->>

        self.utility_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.utility_tab_content, 'Utility')
        self.utility_layout = QtWidgets.QVBoxLayout(self.utility_tab_content)
        self.utility_layout.setContentsMargins(10, 10, 10, 10)
        self.utility_layout.setSpacing(0)
        self.utility_color_frame = QtWidgets.QFrame(self.utility_tab_content)
        self.utility_color_frame.setGeometry(0, 0, 5000, 5000)
        self.utility_color_frame.setStyleSheet('background-color:rgb(235, 235, 235);')

        self.utility_buttons_layout = QtWidgets.QVBoxLayout()
        self.utility_buttons_layout.setAlignment(QtCore.Qt.AlignTop)
        self.utility_buttons_groupbox = QtWidgets.QGroupBox("Actions")
        self.utility_buttons_groupbox.setLayout(self.utility_buttons_layout)
        self.utility_layout.addWidget(self.utility_buttons_groupbox)

        self.initialize_window()

    ##############################
    # Initialize #################
    ##############################

    def initialize_window(self):
        """
        This function is ran by default. The main purpose is to gather information from the Shelve database
        on startup to display in the tool's main window in the UI if previously ran.
        :return:
        """
        self.mayapy_path = self.get_mayapy_path()
        self.set_io_directories()
        self.asset_data_window.setText('Ready.')
        self.initialize_qprocess()

        # Get Material Definition Template
        mat_abs_path = Path(__file__).with_name(self.default_material_definition)
        with mat_abs_path.open() as json_file:
            self.template_lumberyard_material = json.load(json_file)

        if os.path.exists(os.path.join(Path.cwd(), 'materialsdb.dat')):
            if self.materials_db:
                self.set_data_window()
            else:
                self.asset_data_window.setText('Ready.')

    def process_directory(self, directory_path):
        """
        This is the main launching point for processing assets and asset information. The loop processes each
        directory with an FBX file contained within it, and orchestrates image and pbr material conversions
        :return:
        """
        _LOGGER.info('Process directory')
        target_path = Path(directory_path)
        directory_audit = lumberyard_data.walk_directories(target_path)
        self.start_load_sequence(directory_audit)

        for key, values in directory_audit.items():
            directory_audit = values
            destination_directory = self.set_destination_directory(values.directoryname)
            base_directory_path = Path(values.directorypath)
            self.transfer_existing_files(base_directory_path, destination_directory)
            fbx_files = values['files']['.fbx']
            for fbx_file in fbx_files:
                self.progress_event_fired('fbx_{}'.format(fbx_file.name))
                mtl_info = lumberyard_data.get_material_info(base_directory_path, f'{fbx_file.stem}.mtl')
                asset_information = self.filter_asset_information(fbx_file, directory_audit, mtl_info)
                self.set_material_information(asset_information, mtl_info, fbx_file, destination_directory)
            self.create_maya_files(base_directory_path, destination_directory, fbx_files)
        self.reset_load_progress()

    def process_single_asset(self, fbx_path):
        """
        Coordinates the processing of a single FBX asset, which includes generation of all material files
        associated with that FBX file, the conversion of attached textures, and a Maya file for preview
        :param fbx_path: Path to the FBX file to be converted.
        :return:
        """
        _LOGGER.info('Process single asset')
        fbx_file = Path(fbx_path)
        base_directory_path = fbx_file.parent
        directory_audit = lumberyard_data.walk_directories(fbx_file)
        self.start_load_sequence(directory_audit)
        destination_directory = self.set_destination_directory(directory_audit[0].directoryname)
        self.progress_event_fired('fbx_{}'.format(fbx_file.name))
        mtl_info = lumberyard_data.get_material_info(fbx_file.parent, f'{fbx_file.stem}.mtl')
        asset_information = self.filter_asset_information(fbx_file, directory_audit[0], mtl_info)
        self.transfer_existing_files(fbx_file.parent, destination_directory, fbx_file.name)
        self.set_material_information(asset_information, mtl_info, fbx_file, destination_directory)
        self.create_maya_files(base_directory_path, destination_directory, [fbx_file])
        self.reset_load_progress()

    def update_db_listing(self, index, asset_information):
        """
        Processed file information is stored in a Shelve database. Due to the way Shelve stores information,
        if listings need to be updated this cannot be achieved directly- a listing needs to be copied, revisions
        made to it, with the original listing deleted and replaced by the revised copy
        :param index: Index of the shelve database
        :param asset_information: The information to include or update
        :return:
        """
        temp_dict = self.materials_db[index].copy()
        del self.materials_db[index]

        fbx_files = temp_dict.fbxfiles
        for key, values in fbx_files.items():
            asset_information.fbxfiles[key] = values
        self.materials_db[index] = asset_information

    def compare_boilerplate_settings(self, property_values, material_template, export_file_path):
        """
        Lumberyard material definitions only include settings that deviate from default settings. When converting .mtl
        files to .material files, the script attempts to find all settings that each material previously held and carry
        them over. When creating .material files, the script compares information found with the default settings of a
        standard pbr material description, and only includes this information to the definition when it finds a need to
        :param property_values:
        :param material_template:
        :param export_file_path: The absolute path to the final material file
        :return:
        """
        updated = False
        updated_property_dictionary = {}
        try:
            for key, value in material_template.items():
                lower_template_key = key.lower()
                _LOGGER.info(f'-----|| Key: {lower_template_key}')
                for k, v in property_values.items():
                    lower_property_key = k.lower()
                    if lower_property_key == lower_template_key:
                        _LOGGER.info(f'////////////////// Target key found: {key}')
                        _LOGGER.info(f'TemplateValue: {value}')
                        _LOGGER.info(f'PropertyValue: {v}')
                        if v != value:
                            _LOGGER.info(f'{key}---->> NEW VALUE: {v}')
                            updated = True
                            target_value = self.format_material_value(v, export_file_path)
                            updated_property_dictionary[key] = target_value
                        else:
                            updated_property_dictionary[key] = value
                        break
        except Exception as e:
            _LOGGER.info(f'Problem in boilerplate: {e}')
        if updated:
            return updated_property_dictionary
        return None

    def format_material_value(self, value, export_file_path):
        """
        In order for the formatting of the information to work properly once converted into a JSON-like material
        definition, information needs to be properly formatted... not all information can be gathered as strings. This
        function reviews the format of the information and makes changes if necessary for the information to carry
        over properly in the definition.
        :param value: The value under review for formatting issues
        :param export_file_path: The absolute path to the final material file
        :return:
        """
        if isinstance(value, list):
            converted_list = []
            for item in value:
                converted_list.append(item)
            return converted_list
        elif isinstance(value, str):
            if os.path.isfile(str(value)):
                target_path = self.get_relative_path_from_material(value, export_file_path)
                return str(target_path)
            elif value in ['true', 'false']:
                return str(value)
            elif value.isnumeric():
                return 0 if value == 0 else float(value)
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

    def export_material_file(self, file_name, material_name, material_values, destination_directory):
        """
        Collects all the information necessary to generate a material definition, and exports the file. Definition
        information is cross-referenced with a StandardPBR material template file, and information that runs counter
        to what is listed in this file is included in the definition.
        :param file_name: The name of the file to be saved as a material definition.
        :param material_name: The name of the material that the definition represents.
        :param material_values: The values for the material to be included in the definition.
        :param destination_directory: Where the .material file is saved once processed.
        :return:
        """
        _LOGGER.info(f'\n_\n******** {material_name} -- EXPORT INFO: {material_values}')
        _LOGGER.info(f'Filename::::::::: {file_name}')
        export_file_path = Path(destination_directory / file_name)
        if not export_file_path.is_file():
            material_description = {}
            material_template = self.template_lumberyard_material.copy()
            material_textures = material_values.textures
            try:
                for key, values in material_template.items():
                    # These values remain constant
                    if key != 'properties':
                        material_description[key] = values
                    # Values driven by textures and numerical values
                    else:
                        modified_items = []
                        material_description[key] = {}
                        for material_property in constants.SUPPORTED_MATERIAL_PROPERTIES:
                            temp_dict = {}
                            texture_key = None
                            if material_property == 'general':
                                pass
                            elif material_property == 'ambientOcclusion':
                                pass
                            elif material_property == 'baseColor':
                                clr = material_values.numericalsettings.Diffuse.split(',')
                                clr = [float(x) for x in clr]
                                factor = clr[-1]
                                temp_dict['color'] = [clr[0], clr[1], clr[2]]
                                if factor < 1:
                                    temp_dict['factor'] = factor
                                texture_key = 'BaseColor'
                            elif material_property == 'emissive':
                                if 'Emittance' in material_values.numericalsettings:
                                    clr = material_values.numericalsettings.Emittance.split(',')
                                    clr = [float(x) for x in clr]
                                    temp_dict['color'] = [clr[0], clr[1], clr[2]]
                                    factor = clr[-1] if len(clr) == 4 else 0.0
                                    emission_intensity = self.get_emission_intensity(factor)
                                    if 'Emissive' in material_textures.keys() or emission_intensity != -5:
                                        temp_dict['enable'] = True
                                        temp_dict['intensity'] = str(emission_intensity)
                                texture_key = 'Emissive'
                            elif material_property == 'metallic':
                                texture_key = 'Metallic'
                            elif material_property == 'roughness':
                                texture_key = 'Roughness'
                            elif material_property == 'specularF0':
                                clr = material_values.numericalsettings.Specular.split(',')
                                if float(clr[0]) != .5:
                                    temp_dict['factor'] = float(clr[0])
                            elif material_property == 'normal':
                                texture_key = 'Normal'
                            elif material_property == 'opacity':
                                texture_key = 'Opacity'
                                alpha = self.get_alpha_test_value(material_values.numericalsettings)
                                if alpha > 0.:
                                    temp_dict['mode'] = 'Cutout'
                                    temp_dict['factor'] = 1. - alpha
                            elif material_property == 'uv':
                                modifications = material_values['modifications']
                                if modifications:
                                    for modification_key in modifications.keys():
                                        _LOGGER.info(f'{material_name.lower()} <+++++++> {modification_key.lower()}')
                                        if material_name.lower() == modification_key.lower():
                                            for mod_key, mod_value in modifications[material_name].items():
                                                _LOGGER.info(f'ModKey: {mod_key}   ModValue: {mod_value}')
                                                temp_dict[mod_key] = mod_value
                            else:
                                _LOGGER.info(f'Unknown Material Property found: {material_property}')

                            # Add Texture Map
                            if texture_key and texture_key in material_textures.keys() and material_textures[texture_key]:
                                if material_property not in ['emissive', 'general'] and 'factor' not in temp_dict.keys():
                                    temp_dict['factor'] = 1.0
                                    temp_dict['useTexture'] = True
                                temp_dict['textureMap'] = os.path.normpath(material_textures[texture_key])

                            # Determine if property should be added based on deviation from default settings
                            _LOGGER.info(f'TempDict: {temp_dict}')
                            if temp_dict:
                                comparison_dictionary = material_template['properties'][material_property]
                                _LOGGER.info(f'ComparisonDictionary: {comparison_dictionary}')
                                custom_settings = self.compare_boilerplate_settings(temp_dict, comparison_dictionary, os.path.abspath(export_file_path))
                                _LOGGER.info(f'CustomSettings: {custom_settings}')
                                _LOGGER.info(f'MaterialProperty: {material_property}')
                                _LOGGER.info(f'Key: {key}')
                                if custom_settings:
                                    material_description[key][material_property] = custom_settings
                                    _LOGGER.info('Custom settings found')

                if self.create_material_files_checkbox.isChecked():
                    if len(material_description) > 4:
                        _LOGGER.info('\n++++++++++++++\n+++++++++++++++\nFinal Material: {}\n++++++++++++++\n'
                                     '+++++++++++++++\n_\n'.format(json.dumps(material_description, indent=4, sort_keys=False)))
                        lumberyard_data.export_lumberyard_material(os.path.abspath(export_file_path), material_description)
                        return os.path.abspath(export_file_path)
            except Exception as e:
                _LOGGER.info(f'###################### Error processing material file [{file_name}]: {e}')
        else:
            return os.path.abspath(export_file_path)
        return None

    def get_emission_intensity(self, mtl_setting):
        """
        This converts the emission intensity setting of an mtl file into an equivalent exposure value for the
        .material format.
        :param mtl_setting: The float value to be converted from the .mtl file
        :return:
        """
        if float(mtl_setting) > 0.0:
            return math.log(mtl_setting * 1000.0 / .125, 2) / 3.0
        else:
            return -5.0

    def filter_asset_information(self, target_path, directory_audit, mtl_info):
        """
        Helper function used for comparisons as well as structuring information for shelve database entries
        :param target_path: Path to the processed directory
        :param directory_audit: List of files found in a processed directory
        :param mtl_info: Material information relating to each FBX asset (texture names, pbr type, and paths)
        :return:
        """
        asset_dictionary = {}
        if target_path.is_file():
            directory_name = directory_audit.directoryname
            directory_path = Path(directory_audit.directorypath)
            directory_files = directory_audit.files
            materials = self.get_material_information(directory_path, mtl_info, f'{target_path.name}.assetinfo')
            fbx_files = {target_path.name: {constants.FBX_MAYA_FILE: '', constants.FBX_MATERIALS: materials}}
            asset_dictionary = {constants.FBX_DIRECTORY_NAME: directory_name, constants.FBX_DIRECTORY_PATH: directory_path,
                                constants.FBX_FILES: fbx_files}
        else:
            pass
        return Box(asset_dictionary)

    ##############################
    # Creation ###################
    ##############################

    def reformat_materials_db(self, to_maya, updates=False):
        """
        This function is ran before and after the interchange of files between Maya, and is needed so that Maya
        (which runs on Python 2.7) is able to access the information gathered from the main script, as opposed to
        trying to shuttle and manage information based back and forth as strings through a subprocess. This function
        will also prompt a listing update when new file information is received from Maya
        :param to_maya: Specifies whether the reformatting is being executed before or after handoff to Maya for
        generating Maya files
        :param updates: Currently this only holds a list of Maya files generated after the return trip completes,
        but can be used to shuttle any information back from Maya as needed.
        :return:
        """
        temp_dictionary = {}
        for k, v in self.materials_db.items():
            temp_dictionary[k] = v
        self.materials_db.clear()

        if updates:
            temp_dictionary = self.update_maya_files(temp_dictionary, updates)

        if to_maya:
            reformatted_database = utilities.convert_box_dict_to_standard(temp_dictionary)
            for key, value in reformatted_database.items():
                self.materials_db[key] = value
        else:
            reformatted_database = utilities.convert_standard_dict_to_box(temp_dictionary)
            for key, value in reformatted_database.items():
                self.materials_db[key] = value

    def update_maya_files(self, temp_dictionary, updates):
        """
        Updates the "mayafiles" entry of each FBX file that has had a Maya file successfully generated for it.
        :param temp_dictionary: Dictionary containing the current shelve database listing relating to the Maya
        files generated.
        :param updates: Dictionary containing the Maya file information
        :return:
        """
        _LOGGER.info('Updating Maya Files ------------------->>')
        for key, value in temp_dictionary.items():
            for k, v in updates.items():
                if k == key:
                    for fbx_file, maya_file_value in v.items():
                        for target_fbx, target_fbx_listing in value[constants.FBX_FILES].items():
                            if fbx_file == target_fbx:
                                _LOGGER.info('Updating Mayafile: {}'.format(maya_file_value['maya_file']))
                                target_fbx_listing[constants.FBX_MAYA_FILE] = maya_file_value['maya_file']
        return temp_dictionary

    def initialize_qprocess(self):
        """
        This sets the QProcess object up and adds all of the aspects that will remain consistent for each directory
        processed.
        :return:
        """
        self.p = QProcess()
        env = [env for env in QtCore.QProcess.systemEnvironment() if not env.startswith('PYTHONPATH=')]
        if self.mayapy_path != None: 
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

    def create_maya_files(self, base_directory, destination_directory, fbx_files):
        """
        Once information is gathered for targeted directories and all the texture files have been converted, this is the
        final processing step run before the material files are generated. The creation of the Maya files is done by
        way of Maya Standalone. There are some reported issues with Stingray textures and Maya that have been formally
        added to the bug fix queue at Autodesk. Until then, there is an additional script to run for each of the Maya
        files generated to attach texture connections (isolate_and_assign.py).
        :param base_directory: The directory that contains the FBX files used for creating the Maya files
        :param destination_directory: Where the Maya files are to be saved
        :param fbx_files: List of FBX files to be converted to Maya ascii files
        :return:
        """
        if self.create_maya_files_checkbox.isChecked():
            self.reformat_materials_db(True)
            db_updates = None
            fbx_list = []
            for file in fbx_files:
                maya_file = Path(os.path.join(destination_directory, str(file.stem) + '.ma'))
                if not maya_file.is_file():
                    fbx_list.append(str(file))
            try:
                script_path = os.path.join(os.getcwd(), 'create_maya_files.py')
                modify_material_names = 'True' if self.clean_material_names_checkbox.isChecked() else 'False'
                fbx_list.append(os.path.normpath(base_directory))
                fbx_list.append(os.path.normpath(destination_directory))
                fbx_list.append(modify_material_names)

                command = [script_path]
                for file in fbx_list:
                    command.append(file)

                self.p.setArguments(command)
                self.p.start()
                self.p.waitForFinished(-1)
            except Exception as e:
                _LOGGER.warning(f'Error creating Maya Files: {e}')

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

    def cleanup(self):
        self.p.closeWriteChannel()
        _LOGGER.info("Maya File Conversion Complete")

    def create_designer_files(self, fbx_file):
        """
        It would be helpful to have the ability to send an FBX file to Substance Designer and have a node tree
        automatically created to assist in further processing of each of the PBR textures. This serves as a stub
        until this functionality can be added.
        :param fbx_file: File for the Designer setup to be based upon
        :return:
        """
        _LOGGER.info(f'Generate Designer File for: {fbx_file}')

    def create_checklist_data(self, directory_index):
        """
        Creates checklists for keeping track of assets as well as containing file paths and other useful information
        :param directory_index: The shelve database listing that the checklist corresponds to
        :return:
        """
        checklist_data = {}
        texture_types = 'basecolor metallic roughness normal emissive scattering opacity'.split(' ')

        if self.create_checklist_checkbox.isChecked():
            target_dictionary = self.materials_db[str(directory_index)]
            try:
                for key, values in target_dictionary[constants.FBX_FILES].items():
                    fbx_name = key
                    destination_directory = self.set_destination_directory(target_dictionary[constants.FBX_DIRECTORY_NAME])

                    for material_name, texture_set in values[constants.FBX_MATERIALS].items():
                        checklist = collections.OrderedDict()
                        checklist['fbxname'] = fbx_name
                        checklist['materialname'] = material_name
                        checklist['dirname'] = target_dictionary[constants.FBX_DIRECTORY_NAME]
                        checklist['dirpath'] = target_dictionary[constants.FBX_DIRECTORY_PATH]
                        checklist['mayafile'] = values[constants.FBX_MAYA_FILE]

                        for texture in texture_types:
                            checklist[texture] = ''
                            try:
                                if 'textures' in texture_set.keys():
                                    for key, value in texture_set['textures'].items():
                                        if key.lower() == texture:
                                            checklist[texture] = str(value)
                                            break
                            except TypeError as e:
                                _LOGGER.info(f'Could not gather image data for index [{directory_index}]: {e}')
                        checklist['status'] = 'Started'
                        checklist['synced'] = 'No'
                        checklist_data[material_name] = {key: values for key, values in checklist.items()}
                self.export_checklist(destination_directory, dict(checklist_data))
            except Exception as e:
                _LOGGER.info(f'Checklist data could not be processed- Exception: {e}')

    def export_checklist(self, target_path, export_data):
        """
        Creates the auditing checklist csv file for keeping logs on progress in the conversion process.
        :param target_path:
        :param export_data:
        :return:
        """
        if len(export_data.items()):
            headers = [value for key, value in export_data.items()][0].keys()
            checklist_path = os.path.join(target_path, 'checklist.csv')
            with open(checklist_path, 'w', newline='') as outfile:
                writer = csv.writer(outfile)
                writer.writerow(headers)
                for key, value in export_data.items():
                    row_info = []
                    for k, v in value.items():
                        row_info.append(v)
                    writer.writerow(row_info)

    def populate_image_table(self, table_items):
        """
        Feeds texture information into the texture table of the UI when inspecting FBX asset information
        :param table_items: The texture list to be displayed- contains PBR texture type and file paths
        :return:
        """
        self.file_textures_table.setRowCount(0)
        for index, listing in enumerate(table_items):
            self.file_textures_table.insertRow(index)
            for column in range(2):
                cell_value = listing[0] if column == 0 else os.path.basename(listing[1])
                item = QtWidgets.QTableWidgetItem(cell_value)
                self.file_textures_table.setItem(index, column, item)

    def transfer_existing_files(self, base_directory, destination_directory, target_prefix=None):
        """
        Some files in the conversion process need to be copied over into the newly converted destination directory
        unchanged. These files include the assetinfo and fbx files. Target prefix is for the single FBX transfer
        setting, and will limit files moved to those starting with this value
        :param base_directory: The directory that the existing files are located in
        :param destination_directory: The directory that the existing files need to be moved to
        :param target_prefix: The file stem used to target individual file transfers
        :return:
        """
        for file in base_directory.iterdir():
            if file.suffix in constants.TRANSFER_EXTENSIONS:
                if target_prefix:
                    if str(file).startswith(target_prefix) or str(file).startswith(target_prefix.lower()):
                        image_conversion.transfer_file(base_directory / file.name, destination_directory / file.name)
                else:
                    image_conversion.transfer_file(base_directory / file.name, destination_directory / file.name)

    ##############################
    # Getters/Setters ############
    ##############################

    def get_material_information(self, search_path, mtl_info, assetinfo_filename):
        """
        Builds a dictionary for each fbx in a processed directory used for creating a .material file. This information
        includes texture sets, 'uv' modifications like tiling and rotation, numerical settings like color or f0 specularity
        settings, and the fbx layer geometry that the materials are assigned to
        :param search_path: The directory used to search for assets related to a listing
        :param mtl_info: Information drawn from the .mtl file relevant for processing
        :param assetinfo_filename: Assetinfo file
        :return:
        """
        fbx_material_dictionary = Box({})
        for key, values in mtl_info.items():
            _LOGGER.info(f'\n_\n+++++++++++++++++++++++++\nMaterial: {key}\n+++++++++++++++++++++++++\n')
            fbx_material_dictionary[key] = {}

            if self.skip_white_texture_materials.isChecked():
                if 'engineassets/textures/white.dds' in (str(path.as_posix()).lower() for path in values.textures.values()):
                    _LOGGER.info('Found engine white texture... skipping')
                    continue

            if values.attributes.Shader in constants.EXPORT_MATERIAL_TYPES:
                legacy_textures = values.textures
                all_textures = self.get_additional_textures(search_path, legacy_textures)
                numerical_settings = self.get_numerical_settings(mtl_info, key)
                uses_alpha = self.get_alpha_test_value(numerical_settings) > 0.
                fbx_material_dictionary[key][constants.FBX_NUMERICAL_SETTINGS] = numerical_settings
                fbx_material_dictionary[key][constants.FBX_TEXTURES] = self.get_texture_set(search_path, all_textures, uses_alpha) if legacy_textures else ''
                fbx_material_dictionary[key][constants.FBX_TEXTURE_MODIFICATIONS] = values.modifications if values.modifications else ''
                fbx_material_dictionary[key][constants.FBX_MATERIAL_FILE] = ''
                fbx_material_dictionary[key][constants.FBX_ASSIGNED_GEO] = lumberyard_data.get_asset_info(search_path, assetinfo_filename, key)
            elif values.attributes.Shader == 'Nodraw':
                _LOGGER.info('Found No-draw shader... skipping')
            else:
                _LOGGER.info(f'Shader {values.attributes.Shader} not supported... skipping')
                pass
        _LOGGER.info(f'\n_____\n_____FBX Material Dictionary:::: {fbx_material_dictionary}\n______\n')
        return fbx_material_dictionary

    def get_additional_textures(self, search_path, texture_set):
        """
        Mtl files only contain 3 main textures in most cases that are associated with a material (*.dds files)- these
        include diffuse, specular, and ddna ('emission') are also often listed. There are other textures that can be
        lurking in these directories such as Opacity or SSS files- this function tries to find all textures that
        comprise a set that may not fall explicitly into that mtl definition- found based on file prefix
        :param search_path: The directory to search for the additional textures
        :param texture_set: The specified textures from the mtl definition used to track down other related textures
        :return:
        """
        all_textures = texture_set.copy()
        if texture_set:
            texture_keys = texture_set.keys()
            for key in texture_keys:
                try:
                    texture_basename = image_conversion.get_texture_basename(texture_set[key])
                    if texture_basename:
                        for target_image in search_path.iterdir():
                            if target_image.stem.lower().startswith(texture_basename):
                                filemask = image_conversion.get_filemask(target_image)
                                for tex_type, possible_masks in constants.FILE_MASKS.items():
                                    if filemask.lower() in possible_masks:
                                        if tex_type not in all_textures.keys():
                                            all_textures[tex_type] = search_path / target_image
                                            break
                except Exception as e:
                    _LOGGER.info(f'Could not find texture basename... skipping: {e}')
        return all_textures

    def get_numerical_settings(self, mtl_info, material):
        """
        In addition to texture file information, there are many settings that don't require files to change material
        appearances. This gathers that information if it can be carried over to .material file definitions
        :param mtl_info:
        :param material:
        :return:
        """
        numerical_settings = {}
        property_list = ['Diffuse', 'Specular', 'Emittance', 'Opacity', 'AlphaTest']
        for material_property in property_list:
            try:
                numerical_settings[material_property] = mtl_info[material].attributes[material_property]
            except Exception as e:
                pass
        return numerical_settings

    def get_alpha_test_value(self, numericalsettings):
        """
        Return the AlphaTest key value found in dictionnary. Default to 0
        :param numericalsettings: The shader numerical settings dictionnary
        :return:
        """
        if 'AlphaTest' in numericalsettings:
            return float(numericalsettings['AlphaTest'])
        else:
            return 0.

    def get_texture_set(self, search_path, mtl_textures, uses_alpha):
        """
        Begins the process of collecting texture sets for each material found in an FBX file. The process begins by
        getting the file "stem" string in order to find textures starting with identical strings
        :param search_path: The directory to search for corresponding files
        :param mtl_textures: The texture file names pulled from the mtl files in the legacy directories
        :param uses_alpha: Bool to state weither diffuse texture should support alpha channel
        :return:
        """
        _LOGGER.info(f'GetTexSet: {search_path}   MTL TEXTURES: {mtl_textures}')
        destination_directory = self.set_destination_directory(search_path.name)
        for key, value in mtl_textures.items():
            for file in search_path.iterdir():
                if file.stem.lower() == value.stem:
                    mtl_textures[key] = search_path / file
                    _LOGGER.info(f'\n///////// MTL TEX: {mtl_textures[key]}')
                    break
        return image_conversion.get_pbr_textures(mtl_textures, Path(destination_directory), Path(self.input_directory), uses_alpha)

    def get_relative_path_from_material(self, texture_file_abs_path, material_abs_path):
        """
        Material definitions use relative paths for file textures- this function takes the full paths of assets and
        converts to the abbreviated relative path format needed for Lumberyard to source the files
        :param texture_file_abs_path: Absolute path to the texture file
        :param material_abs_path: Absolute path to the material file
        :return: relative path from the material to the texture
        """
        material_dir_abs_path = os.path.dirname(material_abs_path)
        relative_path = os.path.relpath(texture_file_abs_path, material_dir_abs_path)
        return relative_path

    def get_separator_bar(self):
        """ Convenience function for UI layout element """
        self.line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        self.line.setLineWidth(3)
        self.line.setFixedHeight(10)
        self.separator_layout.addWidget(self.line)
        return self.separator_layout

    def get_checklist(self, target_path):
        """
        Reads audit checklist and returns the values contained as a list

        :param target_path:
        :return:
        """
        transfer_list = []
        if os.path.exists(target_path):
            with open(target_path, 'r') as f:
                reader = csv.reader(f, skipinitialspace=True, delimiter=',')
                for row in reader:
                    transfer_list.append(row)
        return transfer_list

    def get_current_section_path(self):
        """
        Helps to determine which shelf database listing is open in the asset inspection window.
        :return:
        """
        section_directory = self.asset_directory_combobox.currentText()
        for key, value in self.materials_db.items():
            if value[constants.FBX_DIRECTORY_PATH] == section_directory:
                return value[constants.FBX_DIRECTORY_PATH]

    def get_mayapy_path(self):
        """
        Finds the most current version of Maya for use of its standalone python capabilities.
        :return:
        """
        maya_versions = [int(x.name[-4:]) for x in self.autodesk_directory.iterdir() if x.name.startswith('Maya') and x.name[-1].isdigit()]
        if maya_versions:
            target_version = f'Maya{max(maya_versions)}'
            self.create_maya_files_checkbox.setChecked(True)
            return self.autodesk_directory / target_version / 'bin\mayapy.exe'
        else:
            self.create_maya_files_checkbox.setEnabled(False)
        return None

    def get_section_audit(self, key):
        """
        Gathers basic information for a directory that has been converted.

        :param key: Which key in the shelve database to target for audit information
        :return:
        """
        section_data = self.materials_db[key]
        readout_text = '+++++++++++++++++++\n'
        readout_text += '{}\n'.format(section_data[constants.FBX_DIRECTORY_NAME])
        readout_text += '+++++++++++++++++++\n\n'

        # FBX files //------------------>>
        fbx_files = section_data[constants.FBX_FILES]
        readout_text += f'FBX files: ({len(fbx_files)} total)\n'
        for k, v in fbx_files.items():
            readout_text += f'{k}\n'

        # Maya files //------------------>>
        maya_file_count = 0
        maya_file_list = []
        material_file_list = []
        texture_files = []
        for k, v in fbx_files.items():
            file_path = v[constants.FBX_MAYA_FILE]
            if file_path:
                maya_file_count += 1
                maya_file_list.append(file_path.name)
            if constants.FBX_MATERIALS in v.keys():
                for k, v in v[constants.FBX_MATERIALS].items():
                    for mat_key, mat_val in v.items():
                        if mat_key == constants.FBX_MATERIAL_FILE:
                            if mat_val:
                                material_file_list.append(mat_val)
                        elif mat_key == constants.FBX_TEXTURES and isinstance(mat_val, dict):
                            if mat_val.keys():
                                for texture_key, texture_path in mat_val.items():
                                    try:
                                        texture_path = Path(texture_path)
                                        texture_files.append(texture_path.name)
                                    except Exception as e:
                                        _LOGGER.info(f'Could not add TextureKey/Path in GET_SECTION_AUDIT: {e}')
                        else:
                            pass
        texture_files = list(set(texture_files))

        # Maya files //------------------>>
        readout_text += f'\nMaya files: ({maya_file_count} total)\n'
        for maya_file in maya_file_list:
            readout_text += f'{maya_file}\n'

        # Material files //------------------>>
        readout_text += f'\nMaterial Definitions: ({len(material_file_list)} Total)\n'
        for material_definition in material_file_list:
            # material_file = Path(material_definition)
            readout_text += f'{Path(material_definition).name}\n'

        # Texture files //------------------>>
        pbr_textures = ['BaseColor', 'Normal', 'Metallic', 'Roughness', 'Emissive']
        readout_text += f'\nTexture Files: ({len(texture_files)} Total)\n'
        for texture_file in texture_files:
            readout_text += f'{texture_file}\n'
        readout_text += '\n\n\n'
        return readout_text

    def set_material_information(self, asset_information, mtl_info, fbx_file, destination_directory):
        """
        Sets the material information in shelve database listing based on individual FBX file
        :param asset_information: Newly generated pbr texture information for each material
        :param mtl_info: Information pulled from corresponding mtl file (modifications, textures, numerical info)
        :param fbx_file: Target FBX file
        :param destination_directory: The directory that assets will be saved to in the conversion process
        :return:
        """
        first_key = next(iter(asset_information.fbxfiles))
        for key, values in asset_information.fbxfiles[first_key].materials.items():
            if not values:
                # Unsupported material. See get_material_information()
                continue

            for k, v in mtl_info.items():
                if key is v.attributes.Name:
                    if v.attributes.Shader in constants.EXPORT_MATERIAL_TYPES:
                        material_values = Box(values)
                        material_file = self.export_material_file(f'{fbx_file.stem}_{key}.material'.lower(), key,
                                                                  material_values, destination_directory)
                        values.materialfile = material_file if material_file else ''
        self.set_materials_db(asset_information)

    def set_materials_db(self, asset_information):
        """
        Once information has been successfully collected for all assets involved in an individual asset directory,
        this prompt moves the recorded information on all transactions into the shelve database
        :param asset_information:
        :return:
        """
        target_path = asset_information[constants.FBX_DIRECTORY_PATH]
        if self.materials_db:
            for key, values in self.materials_db.items():
                if constants.FBX_DIRECTORY_PATH in values.keys():
                    if values[constants.FBX_DIRECTORY_PATH] == target_path:
                        self.update_db_listing(key, asset_information)
                        return key
            target_index = str(len(self.materials_db))

            self.materials_db[target_index] = asset_information
            return target_index
        else:
            self.materials_db['0'] = asset_information
            return '0'

    def set_io_directories(self):
        """
        This is constantly running when user interacts with the source and destination fields in the UI, validating that
        the paths are legitimate. In the event that there are malformed or non-existing paths- it disables the processing
        button.
        :return:
        """
        input_directory = self.input_path_field.text()
        output_directory = self.output_path_field.text()
        unlocked = True

        if self.use_directory_radio_button.isChecked():
            if os.path.isdir(input_directory.strip()):
                self.input_directory = Path(input_directory.strip())
            else:
                unlocked = False
        else:
            if os.path.isfile(input_directory.strip()):
                self.input_directory = Path(input_directory.strip())
            else:
                unlocked = False
        if os.path.isdir(output_directory.strip()):
            self.output_directory = Path(output_directory.strip())
        else:
            unlocked = False
        self.process_files_button.setEnabled(unlocked)

    def set_asset_window(self, section_data=None):
        """
        The asset window gives a more detailed readout of information relating to individual asset folders that have
        been processed. The information stored in this area is gathered using this function.
        :param section_data: If there is no section data a brand now listing is created, otherwise the information
        is added to an existing listing
        :return:
        """
        if len(self.materials_db) and section_data is None:
            section_data = self.materials_db['0']

        try:
            self.asset_directory_combobox.blockSignals(True)
            self.asset_directory_combobox.clear()
            directory_items = []
            for key, values in self.materials_db.items():
                directory_items.append(values[constants.FBX_DIRECTORY_NAME])
            self.asset_directory_combobox.addItems(directory_items)
            current_directory = section_data[constants.FBX_DIRECTORY_NAME]
            index = self.asset_directory_combobox.findText(current_directory, QtCore.Qt.MatchFixedString)
            self.asset_directory_combobox.setCurrentIndex(index)
            self.asset_directory_combobox.blockSignals(False)

            # Set FBX files -->
            fbx_dictionary = {}
            fbx_info = section_data[constants.FBX_FILES]
            if fbx_info:
                fbx_dictionary = self.get_fbx_info(fbx_info)

                # Set Material File List -->
                self.material_files_combobox.clear()
                material_list = self.get_material_list(fbx_dictionary)
                self.material_files_combobox.addItems(material_list)

                # # Set FBX List -->
                self.fbx_combobox.blockSignals(True)
                self.fbx_combobox.clear()
                fbx_files = list(fbx_dictionary.keys())
                self.fbx_combobox.addItems(fbx_files)
                self.fbx_combobox.blockSignals(False)

                # Maya Filename and File Textures -->
                success = self.set_texture_list(fbx_dictionary, fbx_files[0])

        except Exception as e:
            _LOGGER.info(f'Exception occured in set asset window: {e}')

    def get_fbx_info(self, fbx_files):
        fbx_dictionary = {}
        for key, values in fbx_files.items():
            temp_dictionary = {}
            if 'mayafile' in values.keys():
                temp_dictionary['mayafile'] = values[constants.FBX_MAYA_FILE]
            if 'materials' in values.keys():
                temp_dictionary['materials'] = values[constants.FBX_MATERIALS]
            if temp_dictionary:
                fbx_dictionary[key] = temp_dictionary
        return fbx_dictionary

    def get_material_list(self, fbx_dictionary):
        material_list = []
        for key, values in fbx_dictionary.items():
            for k, v in values['materials'].items():
                if 'materialfile' in v.keys():
                    if v['materialfile']:
                        file_path = Path(v['materialfile'])
                        material_list.append(file_path.name)
        material_list.insert(0, 'Choose')
        return material_list

    def set_texture_list(self, fbx_dictionary, target_fbx):
        table_texture_list = []
        for key, values in fbx_dictionary.items():
            if key == target_fbx:
                self.maya_file_label.setText(Path(values['mayafile']).name)
                for k, v in values['materials'].items():
                    if 'textures' in v.keys():
                        for texture_type, texture_path in v['textures'].items():
                            table_texture_list.append([texture_type, texture_path])
        if table_texture_list:
            self.populate_image_table(table_texture_list)
            return True
        else:
            self.file_textures_table.setRowCount(0)
            return False

    def set_data_window(self, target_section='All'):
        """
        The data window prints out comprehensive audit information for all files processed
        :param target_section: This specifies individual sections if the user filters by directory in the UI
        :return:
        """
        self.asset_data_window.setText('')
        readout_text = ''

        # Show general information for "All"
        if target_section == 'All':
            count_totals = Box({'materials': 0, 'textures': 0, 'fbxfiles': 0, 'mayafiles': 0})

            for key, values in self.materials_db.items():
                if values:
                    try:
                        values = Box(values)
                        for k, v in values.fbxfiles.items():
                            count_totals.fbxfiles += 1
                            if v[constants.FBX_MAYA_FILE]:
                                count_totals.mayafiles += 1
                            if v[constants.FBX_MATERIALS]:
                                for material_name, material_values in v[constants.FBX_MATERIALS].items():
                                    count_totals.materials += 1
                                    if constants.FBX_TEXTURES in material_values.keys():
                                        for texture_type, texture_path in material_values.textures.items():
                                            count_totals.textures += 1
                    except Exception as e:
                        _LOGGER.info(f'MaterialDB processing failed for [{key}]: Error: {e}')

            readout_text += 'Processed Directories: {}\n'.format(len(self.materials_db))
            readout_text += 'Material Files Processed: {}\n'.format(count_totals['materials'])
            readout_text += 'Texture Files Processed: {}\n'.format(count_totals['textures'])
            readout_text += 'FBX Files Processed: {}\n'.format(count_totals['fbxfiles'])
            readout_text += 'Generated Maya Files: {}\n\n\n'.format(count_totals['mayafiles'])

        if target_section == 'All':
            target_keys = [x for x in self.materials_db.keys()]
        else:
            target_keys = [str(self.directory_combobox.currentIndex())]

        for key in target_keys:
            readout_text += self.get_section_audit(key)

        self.asset_data_window.setText(readout_text)
        self.set_asset_window()
        self.fbx_combobox_changed()
        self.create_checklist_data(key)

    def set_output_directory(self):
        """
        This lays the groundwork for the output directory. This includes transferring some configuration files, as
        well as creating the subdirectory structure to house the output directories
        :return:
        """
        for target_file in self.initialization_files:
            src = os.path.join(os.getcwd(), target_file)
            dst = os.path.join(self.output_directory, target_file)

            if os.path.isfile(src) and not os.path.isfile(dst):
                image_conversion.transfer_file(src, dst)

        subdirectories = os.path.join(self.output_directory, 'Assets', 'Objects')
        if not os.path.exists(subdirectories):
            os.makedirs(subdirectories)

    def set_destination_directory(self, target_directory_name):
        """
        Checks if a the directory specified as the "Output Directory" in the UI exists, and if not it creates it
        :param target_directory_name: The directory path to search for
        :return:
        """
        destination_directory = self.output_directory / 'Assets\Objects' / target_directory_name
        if not destination_directory.is_dir():
            _LOGGER.info(f'Making Destination Directory: {destination_directory}')
            destination_directory.mkdir()
        return destination_directory

    def set_directory_combobox(self, activate):
        """
        Fills the directory combobox with all processed folders present in the shelve database
        :param activate:
        :return:
        """
        if activate:
            self.directories_items = []
            for item in self.materials_db.values():
                self.directories_items.append(item[constants.FBX_DIRECTORY_NAME])

            if self.directories_items:
                combobox_items = [str(x) for x in self.directories_items]
                self.update_combobox(self.directory_combobox, combobox_items)
                self.directory_combobox.setEnabled(True)
        else:
            self.directory_combobox.clear()
            self.directory_combobox.setEnabled(False)

    def set_asset_directory_combobox(self):
        """
        Fills the asset directory combobox with all the directory names that have been processed by the tool
        :return:
        """
        directories_list = [x[constants.FBX_DIRECTORY_NAME] for x in self.materials_db.values()]
        self.update_combobox(self.asset_directory_combobox, directories_list)

    def update_combobox(self, target_combobox, items_list):
        """
        Adds new items to a specified combobox
        :param target_combobox:
        :param items_list:
        :return:
        """
        target_combobox.blockSignals(True)
        target_combobox.clear()
        target_combobox.addItems(items_list)
        target_combobox.blockSignals(False)

    ##############################
    # Load Actions ###############
    ##############################

    def start_load_sequence(self, audit_info):
        """
        This sets the amount of 'load events' for a completed process. Once this number is set, whenever a load
        event is triggered it will move the load bar a percentage of the way across.
        :param audit_info: Load events are based on what this dictionary contains
        :return:
        """
        _LOGGER.info(f'STARTING LOAD SEQUENCE :::::: {audit_info}')
        self.load_events = [0, 0]
        for key, listing in audit_info.items():
            for fbx in listing['files']['.fbx']:
                if self.create_maya_files_checkbox.isChecked():
                    self.load_events[1] += constants.LOAD_WEIGHTS['maya'] + constants.LOAD_WEIGHTS['fbx']
                else:
                    self.load_events[1] += constants.LOAD_WEIGHTS['fbx']
        _LOGGER.info('LoadEventTotal: {}'.format(self.load_events[1]))

    def progress_event_fired(self, progress):
        """
        Load events are weighted by type- each "maya" file has a weight of 4, 'fbx' has a weight of 2, 'material' is a
        weight of 1
        :param progress: This contains the type of event and the name of the generated asset
        :return:
        """
        event_type = progress.split('_')[0]
        progress_weight = constants.LOAD_WEIGHTS[event_type]
        self.load_events[0] += progress_weight
        load_percentage = int((self.load_events[0] / self.load_events[1]) * 100)
        self.progress_bar.setValue(load_percentage)
        self.app.processEvents()

    def maya_processing_complete(self, db_updates):
        self.reformat_materials_db(False, db_updates)

    def reset_load_progress(self):
        self.set_data_window()
        self.load_events = None
        self.progress_bar.setValue(0)
        _LOGGER.info('Process Complete.')

    ##############################
    # Button Actions #############
    ##############################

    def set_directory_button_clicked(self, target):
        """
        Launches a file browser dialog window for specifying file paths

        :param target: Target sets which directory is being set- input or output
        :return:
        """
        if self.use_directory_radio_button.isChecked():
            file_path = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Directory', self.desktop_location,
                                                                   QtWidgets.QFileDialog.ShowDirsOnly)
            if file_path != '':
                self.input_path_field.setText(file_path) if target == 'input' else self.output_path_field.setText(
                    file_path)
        else:
            if target == 'input':
                file_browser = QtWidgets.QFileDialog()
                file_path = file_browser.getOpenFileName(self, 'Set Target File', self.desktop_location,
                                                         'FBX Files (*.fbx)')
                if file_path != '':
                    self.input_path_field.setText(file_path[0])
                    self.single_asset_file = file_path
            else:
                file_path = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Directory', self.desktop_location,
                                                                       QtWidgets.QFileDialog.ShowDirsOnly)
                if file_path != '':
                    self.output_path_field.setText(file_path)
        self.set_io_directories()

    def radio_clicked(self):
        """
        Reconfigures the UI when user toggles between single asset and directory processing.

        :return:
        """
        self.input_path_field.setText('')
        signal_sender = self.sender()
        if signal_sender.text() == 'Use Directory':
            self.input_directory_label.setText('Input Directory')
            self.input_path_field.setText(str(self.input_directory))
            self.file_target_method = 'Directory'
        else:
            self.input_directory_label.setText('Input FBX File')
            self.input_path_field.setText(str(self.single_asset_file))
            self.file_target_method = 'File'

    def process_files_clicked(self):
        """
        Starts the conversion process once a target directory/file and output folder has been chosen in the UI
        :return:
        """
        self.asset_data_window.setText('Conversion Process Started...')
        self.app.processEvents()
        invalid_path = False
        if self.use_directory_radio_button.isChecked():
            directory_path = self.input_path_field.text()
            if os.path.isdir(directory_path):
                self.input_directory = os.path.normpath(directory_path)
                self.set_output_directory()
                self.process_directory(directory_path)
            else:
                invalid_path = 'Please enter a valid directory path.'

        else:
            fbx_path = self.input_path_field.text()
            if os.path.exists(fbx_path) and fbx_path.endswith('.fbx'):
                self.input_directory = os.path.normpath(os.path.dirname(fbx_path))
                self.set_output_directory()
                self.process_single_asset(fbx_path)
            else:
                invalid_path = 'Please enter a valid FBX path.'

        if invalid_path:
            msg = QtWidgets.QMessageBox()
            msg.setIcon(QtWidgets.QMessageBox.Warning)
            msg.setText(invalid_path)
            msg.setWindowTitle('Bad Path')
            msg.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msg.exec_()

    def search_button_clicked(self):
        search_text = self.find_field.text()
        for key, values in self.materials_db.items():
            section_data = values
            for k, v in section_data[constants.FBX_FILES].items():
                if k == search_text:
                    self.inspect_asset(section_data)
                    return

    def display_combobox_changed(self):
        self.find_field.clear()
        if self.display_combobox.currentText() == 'Show All':
            self.selected_directory_index = -1
            self.set_directory_combobox(False)
            self.results_stacked_widget.setCurrentIndex(0)
        elif self.display_combobox.currentText() == 'Show By Directory':
            self.set_directory_combobox(True)
            self.results_stacked_widget.setCurrentIndex(0)
        else:
            self.results_stacked_widget.setCurrentIndex(1)

    def directory_combobox_changed(self):
        target_data = 'All' if self.directory_combobox.currentText() == '' else self.directory_combobox.currentText()
        self.set_data_window(target_data)

    def asset_directory_combobox_changed(self):
        self.set_asset_window(self.materials_db[str(self.asset_directory_combobox.currentIndex())])

    def fbx_combobox_changed(self):
        target_directory = self.asset_directory_combobox.currentText()
        target_key = self.fbx_combobox.currentText()
        _LOGGER.info(f'TargetDirectory: {target_directory}  TargetKey: {target_key}')
        fbx_files = None
        for index, values in self.materials_db.items():
            if values[constants.FBX_DIRECTORY_NAME] == target_directory:
                fbx_files = values[constants.FBX_FILES]
                fbx_dictionary = self.get_fbx_info(fbx_files)
                self.set_texture_list(fbx_dictionary, target_key)
                break

    def goto_directory_clicked(self):
        """
        Launches the directory containing specified file
        :return:
        """
        index = self.file_textures_table.selectionModel().selectedRows()
        texture_type = self.file_textures_table.item(0,0).text()
        current_directory = self.asset_directory_combobox.currentText()
        for key, values in self.materials_db.items():
            if values[constants.FBX_DIRECTORY_NAME] == current_directory:
                fbx_values = values[constants.FBX_FILES].values()
                for val in fbx_values:
                    material_list = val[constants.FBX_MATERIALS]
                    for material_key, material_values in material_list.items():
                        for texture_key, texture_path in material_values[constants.FBX_TEXTURES].items():
                            if texture_type:
                                if os.path.exists(texture_path):
                                    try:
                                        subprocess.Popen(r'explorer /select, "{}"'.format(texture_path))
                                    except Exception as e:
                                        _LOGGER.info(f'Couldnt open image file directory: {e}')
                            return

    def run_script_clicked(self):
        """
        Sends commands from the standalone window to an open Maya session.
        :return:
        """
        pass

    def back_button_clicked(self):
        _LOGGER.info('Back button clicked')

    def forward_button_clicked(self):
        _LOGGER.info('Forward button clicked')

    def open_material_clicked(self):
        if self.material_files_combobox.currentText() != 'Choose':
            base_path = self.get_current_section_path()
            file_path = os.path.join(base_path, self.material_files_combobox.currentText())
            os.startfile(os.path.abspath(file_path))

    def texture_open_clicked(self):
        """
        Launches and/or opens selected texture file in texture table in Photoshop for making edits.
        :return:
        """
        index = self.file_textures_table.selectionModel().selectedRows()
        texture_type = self.file_textures_table.item(index[0].row(), 0).text()
        current_directory = self.asset_directory_combobox.currentText()
        for key, values in self.materials_db.items():
            if values[constants.FBX_DIRECTORY_NAME] == current_directory:
                fbx_values = values[constants.FBX_FILES].values()
                for val in fbx_values:
                    material_list = val[constants.FBX_MATERIALS]
                    for material_key, material_values in material_list.items():
                        for texture_key, texture_path in material_values[constants.FBX_TEXTURES].items():
                            if texture_key == texture_type:
                                if os.path.exists(texture_path):
                                    try:
                                        os.startfile(os.path.abspath(texture_path))
                                    except Exception as e:
                                        _LOGGER.info(f'Couldnt open image: {e}')
                                    return


if __name__ == '__main__':
    app = QApplication(sys.argv)
    converter = LegacyFilesConverter()
    converter.show()
    sys.exit(app.exec_())
