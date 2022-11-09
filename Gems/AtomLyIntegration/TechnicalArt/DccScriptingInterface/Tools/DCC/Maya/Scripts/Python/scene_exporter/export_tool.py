
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# -------------------------------------------------------------------------
"""! @brief: todo: module docstring"""

# standard imports
from pathlib import Path
import logging as _logging
import os

# o3de, dccsi imports
from DccScriptingInterface.azpy.o3de.renderer.materials import material_utilities
from DccScriptingInterface.azpy.dcc.maya.helpers import maya_materials_conversion as material_utils
from DccScriptingInterface.azpy import general_utils

# maya imports
import maya.cmds as mc
from maya import OpenMayaUI as omui
from shiboken2 import wrapInstance
from PySide2 import QtWidgets, QtCore, QtGui
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
#  global scope
from DccScriptingInterface.Tools.DCC.Maya.Scripts.Python.scene_exporter import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.materials'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.info(f'Initializing: {_MODULENAME}')

from DccScriptingInterface.globals import *

# early attach WingIDE debugger (can refactor to include other IDEs later)
if DCCSI_DEV_MODE:
    import DccScriptingInterface.azpy.test.entry_test
    DccScriptingInterface.azpy.test.entry_test.connect_wing()

from DccScriptingInterface.azpy.shared.ui.help_menu import HelpMenu

HELP_URL = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/Scripts/Python/scene_exporter/readme.md'

maya_mainwindow_ptr = omui.MQtUtil.mainWindow()
maya_mainwindow = wrapInstance(int(maya_mainwindow_ptr), QtWidgets.QWidget)
# -------------------------------------------------------------------------


# ---- First Class --------------------------------------------------------
class SceneExporter(QtWidgets.QWidget):
    def __init__(self, operation, parent=None):
        super().__init__(parent)

        self.setParent(maya_mainwindow)
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(200, 200, 390, 600)
        self.setObjectName('MayaSceneExporter')
        self.setWindowTitle('O3DE Scene Exporter')
        self.isTopLevel()
        self.operation = operation
        self.scope = 0
        self.output_location = None
        self.process_dictionary = None
        self.source = None
        self.desktop_location = os.path.join(os.environ['USERPROFILE'], 'Desktop')
        self.bold_font = QtGui.QFont("Plastique", 8, QtGui.QFont.Bold)

        self.main_container = QtWidgets.QVBoxLayout()
        self.setLayout(self.main_container)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.main_container.setContentsMargins(10, 40, 10, 10)

        # Task Section
        self.test_label = QtWidgets.QLabel('Task')
        self.test_label.setFont(self.bold_font)
        self.main_container.addWidget(self.test_label)
        self.main_container.addSpacing(5)

        # Setup Help Menu
        self.menuBar = QtWidgets.QMenuBar(self) # HelpMenu wants menuBar
        self.help_menu = HelpMenu(self, 'Tool Help...', HELP_URL)

        self.task_button_container = QtWidgets.QHBoxLayout()
        self.task_button_container.setAlignment(QtCore.Qt.AlignLeft)
        self.main_container.addLayout(self.task_button_container)
        self.task_button_group = QtWidgets.QButtonGroup()
        self.task_button_group.idClicked.connect(self.task_radio_clicked)
        self.task_radio_one = QtWidgets.QRadioButton('Query Material Attributes')
        self.task_radio_one.setChecked(True)
        self.task_button_container.addWidget(self.task_radio_one)
        self.task_button_container.addSpacing(30)
        self.task_button_group.addButton(self.task_radio_one, 0)
        self.task_radio_two = QtWidgets.QRadioButton('Convert to O3DE Materials')
        self.task_button_container.addWidget(self.task_radio_two)
        self.task_button_group.addButton(self.task_radio_two, 1)
        self.main_container.addSpacing(5)
        self.main_container.addLayout(self.create_spacer_line())

        # Export Settings
        self.main_container.addSpacing(5)
        self.object_settings_label = QtWidgets.QLabel('Export Settings')
        self.object_settings_label.setFont(self.bold_font)
        self.main_container.addWidget(self.object_settings_label)
        self.main_container.addSpacing(5)

        # --> Row One - Export Settings
        self.settings_rowOne_layout = QtWidgets.QHBoxLayout()
        self.settings_rowOne_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.main_container.addLayout(self.settings_rowOne_layout)
        self.z_up_checkbox = QtWidgets.QCheckBox('Force Z-up Export')
        self.z_up_checkbox.setChecked(True)
        self.settings_rowOne_layout.addWidget(self.z_up_checkbox)
        self.settings_rowOne_layout.addSpacing(67)
        self.preserve_transforms_checkbox = QtWidgets.QCheckBox('Preserve Transform Values')
        self.preserve_transforms_checkbox.setChecked(True)
        self.settings_rowOne_layout.addWidget(self.preserve_transforms_checkbox)
        self.main_container.addSpacing(5)

        # --> Row Two - Export Settings
        self.settings_rowTwo_layout = QtWidgets.QHBoxLayout()
        self.settings_rowTwo_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.main_container.addLayout(self.settings_rowTwo_layout)
        self.preserve_grouped_checkbox = QtWidgets.QCheckBox('Preserve Grouped Objects')
        self.settings_rowTwo_layout.addWidget(self.preserve_grouped_checkbox)
        self.settings_rowTwo_layout.addSpacing(25)
        self.triangulated_export_checkbox = QtWidgets.QCheckBox('Triangulate Exported Objects')
        self.settings_rowTwo_layout.addWidget(self.triangulated_export_checkbox)
        self.main_container.addSpacing(5)
        self.main_container.addLayout(self.create_spacer_line())

        # Scope Section
        self.main_container.addSpacing(5)
        self.scope_label = QtWidgets.QLabel('Scope')
        self.scope_label.setFont(self.bold_font)
        self.main_container.addWidget(self.scope_label)
        self.main_container.addSpacing(5)
        self.scope_button_container = QtWidgets.QHBoxLayout()
        self.scope_button_container.setAlignment(QtCore.Qt.AlignLeft)
        self.main_container.addLayout(self.scope_button_container)
        self.main_container.addSpacing(5)
        self.scope_button_group = QtWidgets.QButtonGroup()
        self.scope_button_group.idClicked.connect(self.scope_radio_clicked)
        self.scope_settings = ['Selected', 'By Name', 'Scene', 'Directory']
        self.scope_values = [None] * len(self.scope_settings)
        self.scope_radio_one = QtWidgets.QRadioButton(self.scope_settings[0])
        self.scope_radio_one.setChecked(True)
        self.scope_button_container.addWidget(self.scope_radio_one)
        self.scope_button_container.addSpacing(30)
        self.scope_button_group.addButton(self.scope_radio_one, 0)
        self.scope_radio_two = QtWidgets.QRadioButton(self.scope_settings[1])
        self.scope_button_container.addWidget(self.scope_radio_two)
        self.scope_button_container.addSpacing(30)
        self.scope_button_group.addButton(self.scope_radio_two, 1)
        self.scope_radio_three = QtWidgets.QRadioButton(self.scope_settings[2])
        self.scope_button_container.addWidget(self.scope_radio_three)
        self.scope_button_container.addSpacing(30)
        self.scope_button_group.addButton(self.scope_radio_three, 2)
        self.scope_radio_four = QtWidgets.QRadioButton(self.scope_settings[3])
        self.scope_button_container.addWidget(self.scope_radio_four)
        self.scope_button_group.addButton(self.scope_radio_four, 3)
        self.scope_target_layout = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.scope_target_layout)
        self.scope_line_edit = QtWidgets.QLineEdit()
        self.scope_line_edit.setFixedHeight(25)
        self.scope_target_layout.addWidget(self.scope_line_edit)
        self.scope_set_button = QtWidgets.QPushButton('Set')
        self.scope_set_button.clicked.connect(self.set_scope_clicked)
        self.scope_target_layout.addWidget(self.scope_set_button)
        self.scope_set_button.setFixedSize(30, 25)
        self.main_container.addSpacing(5)
        self.main_container.addLayout(self.create_spacer_line())

        # Export Path
        self.main_container.addSpacing(5)
        self.export_path_label = QtWidgets.QLabel('Export Path')
        self.export_path_label.setFont(self.bold_font)
        self.main_container.addWidget(self.export_path_label)
        self.main_container.addSpacing(5)

        self.export_path_layout = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.export_path_layout)
        self.export_line_edit = QtWidgets.QLineEdit()
        self.export_line_edit.setFixedHeight(25)
        self.export_path_layout.addWidget(self.export_line_edit)
        self.export_set_button = QtWidgets.QPushButton('Set')
        self.export_set_button.clicked.connect(self.set_output_location)
        self.export_path_layout.addWidget(self.export_set_button)
        self.export_set_button.setFixedSize(30, 25)
        self.main_container.addSpacing(5)
        self.main_container.addLayout(self.create_spacer_line())

        # Output Window
        self.main_container.addSpacing(5)
        self.output_label = QtWidgets.QLabel('Output')
        self.output_label.setFont(self.bold_font)
        self.main_container.addWidget(self.output_label)
        self.main_container.addSpacing(5)
        self.output_file_combobox = QtWidgets.QComboBox()
        self.output_file_combobox.currentIndexChanged.connect(self.output_combobox_changed)
        self.output_file_combobox.setFixedHeight(25)
        self.main_container.addWidget(self.output_file_combobox)
        self.output_window = QtWidgets.QTextEdit()
        self.main_container.addWidget(self.output_window)

        # Process Materials Button
        self.process_materials_button = QtWidgets.QPushButton('Process Materials')
        self.process_materials_button.clicked.connect(self.process_materials_clicked)
        self.process_materials_button.setFixedHeight(50)
        self.main_container.addWidget(self.process_materials_button)
        self.set_window()

    def process_materials(self):
        _LOGGER.info('ProcessMaterialsClicked')
        if self.validate_task():
            # Include export options
            export_options = []
            for checkbox in [self.z_up_checkbox, self.preserve_grouped_checkbox, self.preserve_transforms_checkbox, self.triangulated_export_checkbox]:
                if checkbox.isChecked():
                    export_options.append(checkbox.text())

            self.process_dictionary = material_utilities.process_materials('Maya', self.operation, '_'.join(
                self.scope_settings[self.scope].split(' ')).lower(), export_options, self.output_location, self.source)
            self.set_output_combobox()
            self.set_output_window()

    def enable_scope_buttons(self, txt, btn):
        self.scope_line_edit.setEnabled(txt)
        self.scope_set_button.setEnabled(btn)

    def validate_task(self):
        self.source = self.get_source()
        _LOGGER.info(f'Source: {self.source}  OutputLocation: {self.output_location}')
        if self.source:
            if self.operation == 'convert':
                _LOGGER.info(f'Operation: {self.operation}')
                if not self.output_location:
                    self.set_error_window('export_path', 'missing')
                    return False
                elif not os.path.isdir(self.output_location):
                    self.set_error_window('export_path', 'bad_path')
                    return False
            return True
        return False

    # +++++++++++++++++++++++++-->
    # Getters/Setters +++++++++--->
    # +++++++++++++++++++++++++-->

    def set_error_window(self, error_location, error_type):
        pass

    def set_window(self):
        if self.operation == 'convert':
            self.task_radio_two.setChecked(True)
        self.set_scope_widget()
        self.show()

    def set_scope_widget(self):
        if self.scope == 0:
            self.enable_scope_buttons(False, False)
        elif self.scope == 1:
            self.enable_scope_buttons(True, False)
        else:
            self.enable_scope_buttons(True, True)

        scope_value = self.scope_values[self.scope]
        scope_listing = scope_value if scope_value else ''
        self.scope_line_edit.setText(scope_listing)

    def set_output_combobox(self):
        self.output_file_combobox.clear()
        self.output_file_combobox.blockSignals(True)
        output_keys = []
        for key, values in self.process_dictionary.items():
            for k, v in values.items():
                output_keys.append(k)
        self.output_file_combobox.addItems(output_keys)
        self.output_file_combobox.blockSignals(False)

    def set_output_window(self):
        self.output_window.clear()
        self.output_window.setText(self.get_formatted_output())

        # Increase Line Spacing for better readability
        block_format = QtGui.QTextBlockFormat()
        block_format.setLineHeight(150, QtGui.QTextBlockFormat.ProportionalHeight)
        txt_cursor = self.output_window.textCursor()
        txt_cursor.clearSelection()
        txt_cursor.select(QtGui.QTextCursor.Document)
        txt_cursor.mergeBlockFormat(block_format)

    def set_output_location(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Target Directory', self.desktop_location)
        if directory:
            self.export_line_edit.setText(directory)
            self.set_export_path()

    def set_scope_location(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(self, 'Select Target Directory', self.desktop_location)
        if directory:
            self.scope_values[self.scope] = directory
            self.scope_line_edit.setText(directory)

    def get_source(self):
        source_list = []
        scope_value = self.get_scope_value()
        target_object = self.scope_line_edit.text()
        if scope_value == 'Selected':
            if material_utils.get_selected_objects():
                for item in material_utils.get_selected_objects():
                    source_list.append(item)
        elif scope_value == 'By Name':
            if material_utils.get_object_by_name(target_object):
                source_list.append(target_object)
        elif scope_value == 'Scene':
            if self.scope_line_edit.text() == '':
                source_list = material_utils.get_scene_objects()
            else:
                pass
                # source_list.append(current_scene)
        elif scope_value == 'Directory':
            if os.path.isdir(target_object):
                target_files = ['.ma', '.mb']
                maya_files = self.get_files_by_type(target_object, target_files)
                for file in maya_files:
                    source_list.append(file)
        if source_list:
            return source_list
        return None

    def get_output_location(self):
        if self.export_line_edit.text:
            _LOGGER.info(f'Output location: {self.export_line_edit.text}')

    def get_scope_value(self):
        return self.scope_settings[self.scope]

    def get_object_settings(self, object_name):
        for key, values in self.process_dictionary.items():
            for k, v in values.items():
                if object_name in k:
                    return {key: {k: v}}
        return {}

    def get_formatted_output(self):
        current_selection = self.output_file_combobox.currentText()
        output_dict = self.get_object_settings(current_selection)
        output_string = ''
        for target_file, object_list in output_dict.items():
            output_string += f'\nPROCESSED FILE NAME:\n{target_file}\n'
            try:
                for object_name, object_properties in object_list.items():
                    output_string += self.get_formatted_object_name(object_name)
                    for material_name, material_properties in object_properties.items():
                        _LOGGER.info(f'MaterialNameProcessing: {material_name}')
                        output_string += '\n\n+---------\n'
                        output_string += '+------------------\n+---------------------------------------------'
                        output_string += f'\nMATERIAL NAME: {material_name}\n'
                        output_string += f"MATERIAL TYPE: {material_properties['material_type']}\n"
                        output_string += f'\nASSIGNED TEXTURES:\n'
                        for texture_key, texture_values in material_properties['textures'].items():
                            if texture_values:
                                for k, v in texture_values.items():
                                    if k == 'path':
                                        output_string += f'--> {texture_key} ::: {v}\n'
                        output_string += f'\nSETTINGS:\n'
                        for property_name, property_value in material_properties['settings'].items():
                            output_string += f'{property_name} ::: {property_value}\n'
                        output_string += '+---------------------------------------------\n+------------------'
                        output_string += '\n+---------'
            except AttributeError as e:
                _LOGGER.info(f'Error/Exception in get_formatted_output: {type(e)}   ::  {e}')
            output_string += '\n\n'
        return output_string

    @staticmethod
    def get_formatted_object_name(object_name):
        separator = '#' * (len(object_name) + 6)
        formatted_string = f'\n{separator}\nMESH: {object_name}\n{separator}'
        return formatted_string

    # +++++++++++++++++++++++++-->
    # Button Actions ++++++++++--->
    # +++++++++++++++++++++++++-->

    def task_radio_clicked(self):
        self.operation = 'query' if self.task_button_group.checkedId() == 0 else 'convert'

    def scope_radio_clicked(self, id_):
        self.scope = id_
        self.set_scope_widget()

    def set_scope_clicked(self):
        self.set_scope_location()

    def set_export_path(self):
        target_path = self.export_line_edit.text()
        self.output_location = target_path if Path(target_path).is_dir() else None
        _LOGGER.info(f'-----> {self.output_location}')

    def process_materials_clicked(self):
        self.process_materials()

    def output_combobox_changed(self):
        self.set_output_window()

    # +++++++++++++++++++++++++-->
    # Static Methods ++++++++++--->
    # +++++++++++++++++++++++++-->

    @staticmethod
    def create_spacer_line():
        """! Convenience function for adding separation line to the UI. """
        layout = QtWidgets.QHBoxLayout()
        line = QtWidgets.QLabel()
        line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        line.setLineWidth(1)
        line.setFixedHeight(10)
        layout.addWidget(line)
        layout.setContentsMargins(8, 0, 8, 0)
        return layout

    @staticmethod
    def get_files_by_type(base_directory: Path, file_types: list) -> list:
        file_list = []
        for (root, dirs, files) in os.walk(base_directory, topdown=True):
            root = Path(root)
            for file in files:
                if os.path.splitext(file)[-1] in file_types:
                    file_list.append(root / file)
        return file_list
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def delete_instances():
    """to do: docstring"""
    from DccScriptingInterface.Tools.DCC.Maya.Scripts.Python.export import _PACKAGENAME

    for obj in maya_mainwindow.children():
        if str(type(obj)) == f"<class '{_PACKAGENAME}.SceneExporter'>":
            if obj.__class__.__name__ == "SceneExporter":
                obj.setParent(None)
                obj.deleteLater()


def show_ui(operation):
    """to do: docstring"""
    delete_instances()
    ui = SceneExporter(operation, maya_mainwindow)
    ui.show()
# -------------------------------------------------------------------------
