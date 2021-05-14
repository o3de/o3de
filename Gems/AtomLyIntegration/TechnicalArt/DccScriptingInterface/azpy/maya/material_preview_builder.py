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


#  Maya Library Imports
from PySide2 import QtWidgets, QtCore, QtGui
from maya import OpenMayaUI as omui
from shiboken2 import wrapInstance

#  built-ins
import os

from azpy.maya import materials

mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


class PreviewBuilder(QtWidgets.QWidget):
    def __init__(self, script_data=None):
        super(PreviewBuilder, self).__init__()

        self.setParent(mayaMainWindow)
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(500, 500, 600, 350)
        self.setObjectName('PreviewBuilder')
        self.setWindowTitle('Material Preview Builder (v1.0)')
        self.app = QtWidgets.QApplication.instance()
        self.isTopLevel()

        self.script_data = script_data
        self.database_values = None
        self.start_path = None
        self.processing_mode = 'single_file'
        self.boldFont = QtGui.QFont("Plastique", 8, QtGui.QFont.Bold)

        self.vertical_container = QtWidgets.QVBoxLayout(self)
        self.browser_container = QtWidgets.QVBoxLayout()

        ##################
        # Options Header #
        ##################

        self.options_container = QtWidgets.QHBoxLayout()
        self.drive_label = QtWidgets.QLabel('Operation Type:')
        self.drive_label.setFixedWidth(80)
        self.options_container.addWidget(self.drive_label)
        self.options_container.addSpacing(25)

        self.operation_button_layout = QtWidgets.QHBoxLayout()
        self.radio_button_layout = QtWidgets.QHBoxLayout()
        self.radio_button_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.radio_button_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.radio_button_group = QtWidgets.QButtonGroup()
        self.single_file_radio_button = QtWidgets.QRadioButton('Single File')
        self.single_file_radio_button.setChecked(True)
        self.single_file_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_group.addButton(self.single_file_radio_button)
        self.radio_button_layout.addWidget(self.single_file_radio_button)
        self.radio_button_layout.addSpacing(10)

        self.use_directory_radio_button = QtWidgets.QRadioButton('Directory')
        self.use_directory_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_layout.addWidget(self.use_directory_radio_button)
        self.radio_button_group.addButton(self.use_directory_radio_button)
        self.radio_button_layout.addSpacing(10)

        self.directory_and_subd_radio_button = QtWidgets.QRadioButton('Directory and Subdirectories')
        self.directory_and_subd_radio_button.clicked.connect(self.radio_clicked)
        self.radio_button_layout.addWidget(self.directory_and_subd_radio_button)
        self.radio_button_group.addButton(self.directory_and_subd_radio_button)
        self.operation_button_layout.addLayout(self.radio_button_layout)
        self.options_container.addLayout(self.operation_button_layout)
        self.browser_container.addLayout(self.options_container)
        self.browser_container.addSpacing(3)

        ###########################
        # Directory Browser Panes #
        ###########################

        self.browser_panes_container = QtWidgets.QHBoxLayout()

        # Directory Browser
        self.directory_model = QtWidgets.QFileSystemModel()
        self.directory_model.setFilter(QtCore.QDir.NoDotAndDotDot | QtCore.QDir.NoDot | QtCore.QDir.AllDirs)
        self.directory_view = QtWidgets.QTreeView()
        self.directory_view.setModel(self.directory_model)
        self.directory_root_path = None
        self.directory_view.setHeaderHidden(True)
        self.directory_view.setStyleSheet('QTreeView::item { padding: 2px }')
        self.directory_view.hideColumn(1)
        self.directory_view.hideColumn(2)
        self.directory_view.hideColumn(3)
        self.directory_view.clicked[QtCore.QModelIndex].connect(self.directory_view_clicked)
        self.selection_model = self.directory_view.selectionModel()

        # File Browser
        self.file_model = QtWidgets.QFileSystemModel()
        self.file_model.setFilter(QtCore.QDir.Files)
        self.file_model.setNameFilters(['*.fbx', '*.FBX'])
        self.file_model.setNameFilterDisables(False)
        self.file_view = QtWidgets.QListView()
        self.file_view.setModel(self.file_model)
        self.file_view.clicked[QtCore.QModelIndex].connect(self.file_view_clicked)
        self.file_root_path = 'None'
        self.directory_view_clicked(self.directory_view.currentIndex())

        self.browser_window = QtWidgets.QSplitter()
        self.browser_window.addWidget(self.directory_view)
        self.browser_window.addWidget(self.file_view)
        self.browser_panes_container.addWidget(self.browser_window)
        self.browser_container.addLayout(self.browser_panes_container)
        self.browser_container.addSpacing(4)

        self.browse_groupbox = QtWidgets.QGroupBox()
        self.browse_groupbox.setLayout(self.browser_container)
        self.vertical_container.addWidget(self.browse_groupbox)

        ###########################
        # File Processing Options #
        ###########################

        self.additional_options_container = QtWidgets.QHBoxLayout()
        self.options_groupbox = QtWidgets.QGroupBox()
        self.options_groupbox.setFixedHeight(45)
        self.options_groupbox.setLayout(self.additional_options_container)
        self.vertical_container.addWidget(self.options_groupbox)
        self.additional_options_container.setAlignment(QtCore.Qt.AlignLeft)
        self.additional_options_label = QtWidgets.QLabel('Options:')
        self.additional_options_label.setFont(self.boldFont)
        self.additional_options_container.addWidget(self.additional_options_label)
        self.additional_options_container.addSpacing(25)

        self.clean_material_names = QtWidgets.QCheckBox('Clean Material Names')
        self.additional_options_container.addWidget(self.clean_material_names)
        self.additional_options_container.addSpacing(10)
        self.generate_material_masks = QtWidgets.QCheckBox('Generate Material Masks')
        self.additional_options_container.addWidget(self.generate_material_masks)
        self.additional_options_container.addSpacing(10)
        self.overwrite_existing_files = QtWidgets.QCheckBox('Overwrite Existing')
        self.additional_options_container.addWidget(self.overwrite_existing_files)

        self.process_files_btn = QtWidgets.QPushButton('Create Files')
        self.process_files_btn.setFixedHeight(35)
        self.process_files_btn.clicked.connect(self.create_files_clicked)
        self.vertical_container.addWidget(self.process_files_btn)
        self.initialize()

    def initialize(self):
        """
        Sets up the file browser with the selected FBX from the material conversion scripts
        :return:
        """
        self.database_values = self.script_data
        self.directory_root_path = os.environ['LY_DEV']
        if self.script_data:
            if isinstance(self.script_data, list):
                self.database_values = self.script_data[0]
                self.start_path = self.script_data[1]
                self.directory_root_path = os.path.dirname(self.start_path)
        self.directory_model.setRootPath(self.directory_root_path)
        self.directory_view.setCurrentIndex(self.directory_model.index(self.directory_root_path))
        root_index = self.file_model.setRootPath(self.directory_root_path)
        self.file_view.setRootIndex(root_index)
        if self.start_path:
            self.file_view.setCurrentIndex(self.file_model.index(self.start_path))
            self.file_root_path = self.file_model.filePath(self.file_view.currentIndex())

    def validate_path(self):
        validation = {}
        if self.processing_mode == 'single_file':
            if os.path.isfile(str(self.file_root_path)):
                validation['success'] = True
                validation['target_list'] = [self.file_root_path]
            else:
                validation['error_message'] = 'Operation failed. You must chose an .fbx file\n' \
                                              'in the right browser pane to continue!'
        else:
            mode = '' if self.processing_mode == 'directory' else True
            validation = self.get_directory_paths(mode)
        return validation

    def get_directory_paths(self, subdirectories=False):
        validation = {'target_list': []}
        if not subdirectories:
            validation['target_list'] = [x for x in os.listdir(self.directory_root_path) if x.lower().endswith('.fbx')]
        else:
            for (root, dirs, files) in os.walk(self.directory_root_path, topdown=True):
                for file in files:
                    if file.lower().endswith('.fbx'):
                        validation['target_list'].append(os.path.join(root, file))
        if validation['target_list']:
            validation['success'] = True
        else:
            validation['error_message'] = 'Operation failed. Check that you have the\n' \
                                          'right item selected in the file browser.'
        return validation

    ##############################
    # Button Actions #############
    ##############################

    def directory_view_clicked(self, index):
        self.file_root_path = None
        self.directory_root_path = self.directory_model.filePath(index)
        root_index = self.file_model.setRootPath(self.directory_root_path)
        self.file_view.setRootIndex(root_index)

    def file_view_clicked(self, index):
        self.file_root_path = self.file_model.filePath(index)

    def radio_clicked(self):
        signal_sender = self.sender()
        self.processing_mode = '_'.join(signal_sender.lower().split(' '))

    def create_files_clicked(self):
        validation = self.validate_path()
        if 'success' in validation.keys():
            materials.create_preview_files(validation['target_list'], self.database_values)
        else:
            msg = QtWidgets.QMessageBox()
            msg.setIcon(QtWidgets.QMessageBox.Warning)
            msg.setText(validation['error_message'])
            msg.setWindowTitle('Build Failed')
            msg.setStandardButtons(QtWidgets.QMessageBox.Ok)
            msg.exec_()


def delete_instances():
    for obj in mayaMainWindow.children():
        if str(type(obj)) == "<class 'material_preview_builder.PreviewBuilder'>":
            if obj.__class__.__name__ == "PreviewBuilder":
                obj.setParent(None)
                obj.deleteLater()


def show_ui(script_data=None):
    delete_instances()
    ui = PreviewBuilder(script_data)
    ui.show()
