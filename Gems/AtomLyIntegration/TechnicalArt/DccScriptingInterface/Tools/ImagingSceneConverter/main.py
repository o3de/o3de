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
import os
import socket
import time
import logging as _logging

# Third Party
from PySide2 import QtWidgets, QtCore, QtGui
from maya import OpenMayaUI as omui
from shiboken2 import wrapInstance
from PIL import Image
import pymel.core as pm
import maya.mel as mel

import materials
reload(materials)

# --------------------------------------------------------------------------
# -- Global Definitions --
_MODULENAME = 'Tools.ImagingSceneConverter.main'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOCAL_HOST = socket.gethostbyname(socket.gethostname())
# -------------------------------------------------------------------------


# ++++++++++++++++++++++++++++++++++++----->>
############################################
# Requirements:
# - Install PIL (Pillow) for texture packing
############################################
# ++++++++++++++++++++++++++++++++++++----->>


mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


class ImagingSceneConverter(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(ImagingSceneConverter, self).__init__(parent)

        self.app = QtWidgets.QApplication.instance()
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 400, 600)
        self.setObjectName('ImagingSceneConverter')
        self.setWindowTitle('Imaging Scene Converter')
        self.isTopLevel()
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowMinMaxButtonsHint)
        self.scene_material_info = None
        self.bold_font = QtGui.QFont("Helvetica", 8, QtGui.QFont.Bold)
        self.geo_tables = []
        self.files_tables = []

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
        self.target_directory_groupbox = QtWidgets.QGroupBox("Target Scene")
        self.target_directory_groupbox.setLayout(self.target_directory_layout)
        self.content_layout.addWidget(self.target_directory_groupbox)

        # ---- Input Directory
        self.input_field_layout = QtWidgets.QHBoxLayout()
        self.input_path_field = QtWidgets.QLineEdit(self.get_current_scene())
        self.input_path_field.setFixedHeight(25)
        self.input_field_layout.addWidget(self.input_path_field)
        self.set_input_button = QtWidgets.QPushButton('Set')
        self.set_input_button.clicked.connect(self.set_directory_button_clicked)
        self.set_input_button.setFixedSize(40, 25)
        self.input_field_layout.addWidget(self.set_input_button)
        self.target_directory_layout.addLayout(self.input_field_layout)

        ####################################
        # Stacked Layout ###################
        ####################################

        self.window_tabs = QtWidgets.QTabWidget()
        self.content_layout.addWidget(self.window_tabs)
        self.material_combobox_items = []

        # ++++++++++++++++++-->
        # Existing Tab -------->>
        # ++++++++++++++++++-->

        self.existing_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.existing_tab_content, 'Existing')
        self.existing_layout = QtWidgets.QVBoxLayout(self.existing_tab_content)
        self.existing_layout.setAlignment(QtCore.Qt.AlignTop)
        self.existing_layout.setContentsMargins(10, 10, 10, 10)
        # self.existing_layout.addLayout(self.create_spacer_line())

        # Existing Material Combobox
        self.existing_layout.addSpacing(10)
        self.existing_material_groupbox = QtWidgets.QGroupBox()
        self.existing_material_layout = QtWidgets.QHBoxLayout()
        self.existing_material_groupbox.setLayout(self.existing_material_layout)
        self.existing_material_label = QtWidgets.QLabel('Material')
        self.existing_material_label.setStyleSheet('padding-left: 1px;')
        self.existing_material_label.setFont(self.bold_font)
        self.existing_material_label.setFixedWidth(65)
        self.existing_material_layout.addWidget(self.existing_material_label)
        self.existing_material_combobox = QtWidgets.QComboBox()
        self.existing_material_combobox.currentIndexChanged.connect(self.material_combobox_changed)
        self.existing_material_combobox.setFixedHeight(25)
        self.existing_material_layout.addWidget(self.existing_material_combobox)
        self.existing_layout.addWidget(self.existing_material_groupbox)
        self.existing_layout.addLayout(self.create_spacer_line())

        # Existing File Textures Table
        self.existing_textures_label = QtWidgets.QLabel('File Textures')
        self.existing_textures_label.setStyleSheet('padding-left: 1px;')
        self.existing_textures_label.setFont(self.bold_font)
        self.existing_layout.addWidget(self.existing_textures_label)
        self.existing_layout.addSpacing(2)
        self.existing_textures_table = QtWidgets.QTableWidget()
        self.existing_textures_table.verticalHeader().hide()
        self.existing_textures_table.setColumnCount(2)
        self.existing_textures_table.setAlternatingRowColors(True)
        self.existing_textures_table.setSelectionBehavior(QtWidgets.QTableView.SelectRows)
        self.existing_textures_table.setHorizontalHeaderLabels(['Texture Name', 'Texture Type'])
        existing_files_header = self.existing_textures_table.horizontalHeader()
        existing_files_header.setStyleSheet('QHeaderView::section {padding-top:7px; padding-left:5px;}')
        existing_files_header.setDefaultAlignment(QtCore.Qt.AlignLeft)
        existing_files_header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        existing_files_header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.existing_layout.addWidget(self.existing_textures_table)
        self.existing_layout.addLayout(self.create_spacer_line())

        # Assigned Geo Table
        self.existing_geometry_label = QtWidgets.QLabel('Assigned Geometry')
        self.existing_geometry_label.setStyleSheet('padding-left: 1px;')
        self.existing_geometry_label.setFont(self.bold_font)
        self.existing_layout.addWidget(self.existing_geometry_label)
        self.existing_layout.addSpacing(2)
        self.existing_geometry_table = QtWidgets.QTableWidget()
        self.existing_geometry_table.setColumnCount(1)
        self.existing_geometry_table.setAlternatingRowColors(True)
        self.existing_geometry_table.setSelectionBehavior(QtWidgets.QTableView.SelectRows)
        self.existing_geometry_table.horizontalHeader().hide()
        self.existing_geometry_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.existing_geometry_table.verticalHeader().hide()

        self.existing_layout.addWidget(self.existing_geometry_table)

        # ++++++++++++++++++-->
        # Target Tab ---------->>
        # ++++++++++++++++++-->

        self.target_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.target_tab_content, 'Target')
        self.target_layout = QtWidgets.QVBoxLayout(self.target_tab_content)
        self.target_layout.setAlignment(QtCore.Qt.AlignTop)
        self.target_layout.setContentsMargins(10, 10, 10, 10)

        # Existing Material Combobox
        self.target_layout.addSpacing(10)
        self.target_material_groupbox = QtWidgets.QGroupBox()
        self.target_material_layout = QtWidgets.QHBoxLayout()
        self.target_material_groupbox.setLayout(self.target_material_layout)
        self.target_material_label = QtWidgets.QLabel('Material')
        self.target_material_label.setStyleSheet('padding-left: 1px;')
        self.target_material_label.setFont(self.bold_font)
        self.target_material_label.setFixedWidth(65)
        self.target_material_layout.addWidget(self.target_material_label)
        self.target_material_combobox = QtWidgets.QComboBox()
        self.target_material_combobox.currentIndexChanged.connect(self.material_combobox_changed)
        self.target_material_combobox.setFixedHeight(25)
        self.target_material_layout.addWidget(self.target_material_combobox)
        self.target_layout.addWidget(self.target_material_groupbox)
        self.target_layout.addLayout(self.create_spacer_line())

        # Target File Textures Table
        self.target_textures_label = QtWidgets.QLabel('File Textures')
        self.target_textures_label.setStyleSheet('padding-left: 1px;')
        self.target_textures_label.setFont(self.bold_font)
        self.target_layout.addWidget(self.target_textures_label)
        self.target_layout.addSpacing(2)
        self.target_textures_table = QtWidgets.QTableWidget()
        self.target_textures_table.verticalHeader().hide()
        self.target_textures_table.setColumnCount(2)
        self.target_textures_table.setAlternatingRowColors(True)
        self.target_textures_table.setSelectionBehavior(QtWidgets.QTableView.SelectRows)
        self.target_textures_table.setHorizontalHeaderLabels(['Texture Name', 'Texture Type'])
        files_header = self.target_textures_table.horizontalHeader()
        files_header.setStyleSheet('QHeaderView::section {padding-top:7px; padding-left:5px;}')
        files_header.setDefaultAlignment(QtCore.Qt.AlignLeft)
        files_header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        files_header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.target_layout.addWidget(self.target_textures_table)
        self.target_layout.addLayout(self.create_spacer_line())

        # Assigned Geo Table
        self.target_geometry_label = QtWidgets.QLabel('Assigned Geometry')
        self.target_geometry_label.setStyleSheet('padding-left: 1px;')
        self.target_geometry_label.setFont(self.bold_font)
        self.target_layout.addWidget(self.target_geometry_label)
        self.target_layout.addSpacing(2)
        self.target_geometry_table = QtWidgets.QTableWidget()

        self.target_geometry_table.setColumnCount(1)
        self.target_geometry_table.setAlternatingRowColors(True)
        self.target_geometry_table.setSelectionBehavior(QtWidgets.QTableView.SelectRows)
        self.target_geometry_table.horizontalHeader().hide()
        self.target_geometry_table.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.target_geometry_table.verticalHeader().hide()
        self.target_layout.addWidget(self.target_geometry_table)

        # ++++++++++++++++++-->
        # Utilities Tab ------->>
        # ++++++++++++++++++-->

        self.utilities_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.utilities_tab_content, 'Utilities')
        self.utilities_layout = QtWidgets.QVBoxLayout(self.utilities_tab_content)
        self.utilities_layout.setAlignment(QtCore.Qt.AlignTop)
        self.utilities_layout.setContentsMargins(10, 10, 10, 10)

        ####################################
        # Execute Button ###################
        ####################################

        self.convert_scene_button = QtWidgets.QPushButton('Convert Scene')
        self.convert_scene_button.clicked.connect(self.convert_scene_clicked)
        self.content_layout.addWidget(self.convert_scene_button)
        self.convert_scene_button.setFixedHeight(40)
        self.content_layout.addSpacing(10)
        self.initialize()

    def initialize(self):
        self.scene_material_info = materials.get_material_info()
        _LOGGER.info('Scene Material Information: {}'.format(self.scene_material_info, indent=4, sort_keys=False))
        self.geo_tables = [self.existing_geometry_table, self.target_geometry_table]
        self.files_tables = [self.existing_textures_table, self.target_textures_table]
        self.set_material_combobox_items()
        self.update_ui(True)

    def update_ui(self, init=False):
        _LOGGER.info('\n+++++++++++++++++++')
        self.set_files_table(init)
        self.set_geo_table(init)
        _LOGGER.info('\n+++++++++++++++++++\n\n')

    def set_material_combobox_items(self):
        self.material_combobox_items = self.scene_material_info.keys()
        _LOGGER.info(self.material_combobox_items)
        material_comboboxes = [self.existing_material_combobox, self.target_material_combobox]
        for cb in material_comboboxes:
            cb.blockSignals(True)
            cb.clear()
            cb.addItems([str(x) for x in self.material_combobox_items])
            cb.blockSignals(False)

    def set_files_table(self, init=False):
        _LOGGER.info('\nSetting Files Table: Index: {}'.format(self.window_tabs.currentIndex()))
        target_tables = self.files_tables if init is True else [self.files_tables[self.window_tabs.currentIndex()]]
        for table in target_tables:
            table.setRowCount(0)
            target_material = self.get_target_material(self.files_tables.index(table))
            textures_list = self.scene_material_info[target_material]['textures']
            if textures_list:
                for row, texture_file in enumerate(textures_list):
                    _LOGGER.info('Texture: Row[{}] -->> {}'.format(row, texture_file))
                    table.insertRow(row)
                    item = QtWidgets.QTableWidgetItem(str(texture_file))
                    table.setItem(row, 0, item)

    def set_geo_table(self, init=False):
        _LOGGER.info('\nSetting Geo Table')
        target_tables = self.geo_tables if init is True else [self.geo_tables[self.window_tabs.currentIndex()]]
        for table in target_tables:
            table.setRowCount(0)
            target_material = self.get_target_material(self.geo_tables.index(table))
            _LOGGER.info('Table: {}     TargetMaterial: {}'.format(table, target_material))
            geo_list = self.scene_material_info[target_material]['assigned'][0]
            for row, mesh in enumerate(geo_list):
                _LOGGER.info('Geo: Row[{}] -->> {}'.format(row, mesh))
                mesh = str(mesh).split('|')[-1] if '|' in mesh else str(mesh)
                table.insertRow(row)
                item = QtWidgets.QTableWidgetItem(mesh)
                table.setItem(row, 0, item)

    def get_target_material(self, target_tab):
        material_name = self.existing_material_combobox.currentText() if target_tab == 0 else \
            self.target_material_combobox.currentText()
        return material_name

    def get_texture_type(self):
        pass

    @staticmethod
    def get_current_scene():
        return pm.sceneName()

    @staticmethod
    def create_spacer_line():
        """ Convenience function for adding separation line to the UI. """
        layout = QtWidgets.QHBoxLayout()
        line = QtWidgets.QLabel()
        line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        line.setLineWidth(1)
        line.setFixedHeight(10)
        layout.addWidget(line)
        layout.setContentsMargins(0, 0, 0, 0)
        return layout

    def set_directory_button_clicked(self):
        _LOGGER.info('Set Directory clicked')

    def material_combobox_changed(self):
        self.update_ui('existing') if self.window_tabs.currentIndex() == 0 else self.update_ui('target')

    def convert_scene_clicked(self):



def delete_instances():
    for obj in mayaMainWindow.children():
        if str(type(obj)) == "<class 'ImagingSceneConverter.main.ImagingSceneConverter'>":
            if obj.__class__.__name__ == "ImagingSceneConverter":
                obj.setParent(None)
                obj.deleteLater()


def show_ui():
    _LOGGER.info('Firing ImagingSceneConverter...')
    delete_instances()
    ui = ImagingSceneConverter(mayaMainWindow)
    ui.show()
