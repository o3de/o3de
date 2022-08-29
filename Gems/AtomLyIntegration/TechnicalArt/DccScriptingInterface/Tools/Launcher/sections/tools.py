# -*- coding: utf-8 -*-
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


from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtWidgets import QMessageBox, QRadioButton
from PySide2.QtCore import Signal, Slot
from Tools.Launcher.sections import section_utilities as utilities
from Tools.Launcher.data import project_constants as constants
from pathlib import Path
from box import Box
from copy import copy
import logging


_LOGGER = logging.getLogger('Launcher.tools')


class Tools(QtWidgets.QWidget):
    def __init__(self, model):
        super(Tools, self).__init__()

        _LOGGER.info('Tools Page added to content layout')
        self.model = model
        self.tools_listings = self.model.tools
        self.markdown = (settings.get('PATH_DCCSI_TOOLS') /
                         'Launcher/markdown/PycharmDebugging/pycharm_debugging.md').as_posix()
        self.desktop_location = (Path.home() / 'Desktop').as_posix()
        self.current_section = None
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(10, 3, 10, 10)
        self.content_frame = QtWidgets.QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)

        self.add_button_group = QtWidgets.QButtonGroup()
        self.add_button_group.idClicked.connect(self.add_button_clicked)

        self.browse_button_group = QtWidgets.QButtonGroup()
        self.browse_button_group.idClicked.connect(self.browse_clicked)

        self.splitter_layout = QtWidgets.QVBoxLayout()
        self.content_layout.addLayout(self.splitter_layout)
        self.page_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        self.page_splitter.setSizes([150, 200, 565, 565])
        self.splitter_layout.addWidget(self.page_splitter)

        # +++++++++++++++++++++--->>>
        # LEFT COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++--->>>

        self.left_column_widget = QtWidgets.QWidget()
        self.left_column_widget.setMaximumWidth(100)
        self.left_column_container = QtWidgets.QVBoxLayout()
        self.left_column_container.setContentsMargins(0, 0, 0, 0)
        self.left_column_widget.setLayout(self.left_column_container)
        self.page_splitter.addWidget(self.left_column_widget)

        # Tool Category Selection Panel ########################################
        self.tool_category_layout = QtWidgets.QVBoxLayout()
        self.tool_category_layout.setAlignment(QtCore.Qt.AlignTop)
        self.tool_category_panel = QtWidgets.QWidget()
        self.tool_category_panel.setLayout(self.tool_category_layout)
        self.tool_category_layout.setContentsMargins(0, 7, 0, 0)
        self.panel_frame = QtWidgets.QFrame(self.tool_category_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(135, 135, 135);')
        self.panel_frame.setGeometry(0, 0, 150, 5000)
        self.left_column_container.addWidget(self.tool_category_panel)

        # --> Tool Category Button Layout
        self.category_button_container = QtWidgets.QVBoxLayout()
        self.category_button_container.setContentsMargins(10, 3, 10, 10)
        self.tool_category_layout.addLayout(self.category_button_container)

        self.section_buttons = {}
        self.section_button = None
        self.active_section_button = None
        for tool_type in self.model.tools:
            self.section_buttons.update({tool_type: self.add_section_button(tool_type)})
            self.category_button_container.addWidget(self.section_buttons[tool_type])

        # +++++++++++++++++++++++--->>>
        # CENTER COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++++--->>>

        self.center_column_widget = QtWidgets.QWidget()
        self.center_column_widget.setFixedWidth(200)
        self.center_column_container = QtWidgets.QStackedLayout()
        self.center_column_container.setContentsMargins(0, 0, 0, 0)
        self.center_column_widget.setLayout(self.center_column_container)
        self.page_splitter.addWidget(self.center_column_widget)

        #####################
        # Get Started Panel #
        #####################

        self.get_started_layout = QtWidgets.QVBoxLayout()
        self.get_started_panel = QtWidgets.QWidget()
        self.get_started_panel.setLayout(self.get_started_layout)
        self.get_started_frame = QtWidgets.QFrame(self.get_started_panel)
        self.get_started_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        self.get_started_frame.setGeometry(0, 0, 200, 5000)
        self.center_column_container.addWidget(self.get_started_panel)
        self.select_category_label = QtWidgets.QLabel('Select Tool Category\nto the left to get started.')
        self.select_category_label.setAlignment(QtCore.Qt.AlignCenter)
        self.get_started_layout.addWidget(self.select_category_label)

        ##################
        # Add Tool Panel #
        ##################

        self.add_tool_layout = QtWidgets.QVBoxLayout()
        self.add_tool_panel = QtWidgets.QWidget()
        self.add_tool_panel.setLayout(self.add_tool_layout)
        self.add_tool_frame = QtWidgets.QFrame(self.add_tool_panel)
        self.add_tool_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        self.add_tool_frame.setGeometry(0, 0, 200, 5000)
        self.center_column_container.addWidget(self.add_tool_panel)

        # --- No tools found message
        self.tool_message_layout = QtWidgets.QVBoxLayout()
        self.add_tool_layout.addSpacing(220)
        self.add_tool_layout.addLayout(self.tool_message_layout)
        self.no_tools_found_label = QtWidgets.QLabel('No Tools found for\nselected section.')
        self.no_tools_found_label.setAlignment(QtCore.Qt.AlignCenter)
        self.tool_message_layout.addWidget(self.no_tools_found_label)

        # ******* Add/Cancel Tool Buttons *******
        self.add_cancel_button_layout = QtWidgets.QStackedLayout()
        self.tool_message_layout.addLayout(self.add_cancel_button_layout)

        # --- Add tool button
        self.add_center_widget = QtWidgets.QWidget()
        self.add_center_container = QtWidgets.QVBoxLayout()
        self.add_center_container.setAlignment(QtCore.Qt.AlignTop)
        self.add_center_widget.setLayout(self.add_center_container)

        self.add_center_layout = QtWidgets.QHBoxLayout()
        self.add_center_layout.setAlignment(QtCore.Qt.AlignCenter)
        self.add_center_container.addLayout(self.add_center_layout)
        self.add_cancel_button_layout.addWidget(self.add_center_widget)
        self.add_tool_listing_button = QtWidgets.QPushButton('Add Tool')
        self.add_tool_listing_button.clicked.connect(self.add_button_clicked)
        self.add_tool_listing_button.setFixedSize(75, 30)
        self.add_tool_listing_button.setObjectName('Primary')
        self.add_center_layout.addWidget(self.add_tool_listing_button)

        # --- Cancel button
        self.cancel_center_widget = QtWidgets.QWidget()
        self.cancel_center_container = QtWidgets.QVBoxLayout()
        self.cancel_center_container.setAlignment(QtCore.Qt.AlignTop)
        self.cancel_center_widget.setLayout(self.cancel_center_container)

        self.cancel_center_layout = QtWidgets.QHBoxLayout()
        self.cancel_center_layout.setAlignment(QtCore.Qt.AlignCenter)
        self.cancel_center_container.addLayout(self.cancel_center_layout)
        self.add_cancel_button_layout.addWidget(self.cancel_center_widget)
        self.cancel_tool_button = QtWidgets.QPushButton('Cancel')
        self.cancel_tool_button.clicked.connect(self.cancel_tool_clicked)
        self.cancel_tool_button.setFixedSize(75, 30)
        self.cancel_tool_button.setObjectName('Secondary')
        self.cancel_center_layout.addWidget(self.cancel_tool_button)

        #######################
        # Tool Listings Panel #
        #######################

        self.tool_listings_layout = QtWidgets.QVBoxLayout()
        self.tool_listings_layout.setAlignment(QtCore.Qt.AlignTop)
        self.tool_listings_panel = QtWidgets.QWidget()
        self.tool_listings_panel.setLayout(self.tool_listings_layout)
        self.tool_listings_layout.setContentsMargins(0, 0, 0, 0)
        self.tool_listings_layout.setSpacing(0)
        self.panel_frame = QtWidgets.QFrame(self.tool_listings_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        self.panel_frame.setGeometry(0, 0, 200, 5000)
        self.center_column_container.addWidget(self.tool_listings_panel)

        # +++++++++++++++++++++++--->>>
        # RIGHT COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++++--->>>

        self.stacked_layout = QtWidgets.QStackedLayout()
        self.stacked_widget = QtWidgets.QWidget()
        self.stacked_widget.setLayout(self.stacked_layout)
        self.page_splitter.addWidget(self.stacked_widget)

        self.markdown_display = QtWidgets.QTextEdit()
        self.stacked_layout.addWidget(self.markdown_display)

        # Add Tool Layer
        self.add_tool_layout = QtWidgets.QVBoxLayout()
        self.add_tool_widget = QtWidgets.QWidget()
        self.add_tool_widget.setLayout(self.add_tool_layout)
        self.stacked_layout.addWidget(self.add_tool_widget)

        # Name
        self.tool_name_label = QtWidgets.QLabel('Name')
        self.add_tool_layout.addWidget(self.tool_name_label)
        self.tool_name_field = QtWidgets.QLineEdit()
        self.tool_name_field.setFixedHeight(25)
        self.add_tool_layout.addWidget(self.tool_name_field)

        # Category
        self.category_placement_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.category_placement_layout)

        self.tool_category_layout = QtWidgets.QVBoxLayout()
        self.category_placement_layout.addLayout(self.tool_category_layout)
        self.tool_category_label = QtWidgets.QLabel('Category')
        self.tool_category_layout.addWidget(self.tool_category_label)
        self.tool_category_combobox = QtWidgets.QComboBox()
        self.tool_category_combobox.setFixedHeight(25)
        self.tool_categories = (
            'Utility',
            'Workflow',
            'Conversion',
            'Add New Category'
        )
        self.tool_category_layout.addWidget(self.tool_category_combobox)
        self.tool_category_combobox.addItems(self.tool_categories)
        self.category_placement_layout.addSpacing(40)

        # Tool window placement
        self.align_window_layout = QtWidgets.QVBoxLayout()
        self.align_window_layout.setAlignment(QtCore.Qt.AlignBottom)
        self.category_placement_layout.addLayout(self.align_window_layout)
        self.tool_window_layout = QtWidgets.QHBoxLayout()
        self.align_window_layout.addLayout(self.tool_window_layout)
        self.tool_ui_label = QtWidgets.QLabel('Tool Window Placement:')
        self.tool_ui_label.setFixedWidth(140)
        self.tool_window_layout.addWidget(self.tool_ui_label)
        self.tool_window_layout.addSpacing(10)
        self.window_radio_layout = QtWidgets.QHBoxLayout()
        self.tool_window_layout.addLayout(self.window_radio_layout)
        self.window_radio_button_group = QtWidgets.QButtonGroup()
        self.embedded_radio_button = QtWidgets.QRadioButton('Embedded')
        self.embedded_radio_button.setFixedWidth(100)
        self.window_radio_layout.addWidget(self.embedded_radio_button)
        self.window_radio_button_group.addButton(self.embedded_radio_button)
        self.embedded_radio_button.setChecked(True)
        self.standalone_radio_button = QtWidgets.QRadioButton('Standalone Window')
        self.standalone_radio_button.setFixedWidth(140)
        self.window_radio_layout.addWidget(self.standalone_radio_button)
        self.window_radio_button_group.addButton(self.standalone_radio_button)

        # Description
        self.tool_description_label = QtWidgets.QLabel('Description')
        self.add_tool_layout.addWidget(self.tool_description_label)
        self.tool_description_field = QtWidgets.QTextEdit()
        self.add_tool_layout.addWidget(self.tool_description_field)

        # Location
        self.tool_location_label = QtWidgets.QLabel('Start File Location')
        self.add_tool_layout.addWidget(self.tool_location_label)
        self.tool_location_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.tool_location_layout)
        self.tool_location_field = QtWidgets.QLineEdit()
        self.tool_location_field.setFixedHeight(25)
        self.tool_location_layout.addWidget(self.tool_location_field)
        self.tool_location_browse = QtWidgets.QPushButton('Browse')
        self.tool_location_browse.setObjectName('Secondary')
        self.browse_button_group.addButton(self.tool_location_browse, 0)
        self.tool_location_browse.setFixedSize(75, 25)
        self.tool_location_layout.addWidget(self.tool_location_browse)

        # Tool Documentation
        self.tool_documentation_location = QtWidgets.QLabel('Documentation File Location')
        self.add_tool_layout.addWidget(self.tool_documentation_location)
        self.tool_documentation_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.tool_documentation_layout)
        self.tool_documentation_field = QtWidgets.QLineEdit()
        self.tool_documentation_field.setFixedHeight(25)
        self.tool_documentation_layout.addWidget(self.tool_documentation_field)
        self.tool_documentation_browse = QtWidgets.QPushButton('Browse')
        self.tool_documentation_browse.setObjectName('Secondary')
        self.browse_button_group.addButton(self.tool_documentation_browse, 1)
        self.tool_documentation_browse.setFixedSize(75, 25)
        self.tool_documentation_layout.addWidget(self.tool_documentation_browse)

        # Markdown Location
        self.tool_markdown_location = QtWidgets.QLabel('Markdown File Location')
        self.add_tool_layout.addWidget(self.tool_markdown_location)
        self.markdown_location_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.markdown_location_layout)
        self.tool_markdown_field = QtWidgets.QLineEdit()
        self.tool_markdown_field.setFixedHeight(25)
        self.markdown_location_layout.addWidget(self.tool_markdown_field)
        self.markdown_create_button = QtWidgets.QPushButton('Create New')
        self.markdown_create_button.setFixedSize(90, 25)
        self.markdown_create_button.setEnabled(False)
        self.markdown_location_layout.addWidget(self.markdown_create_button)
        self.markdown_location_browse = QtWidgets.QPushButton('Browse')
        self.markdown_location_browse.setObjectName('Secondary')
        self.browse_button_group.addButton(self.markdown_location_browse, 2)
        self.markdown_location_browse.setFixedSize(75, 25)
        self.markdown_location_layout.addWidget(self.markdown_location_browse)
        self.add_tool_layout.addSpacing(15)

        # Add/Remove Tool Listing
        self.line = None
        self.separator_layout = self.get_separator_bar()
        self.add_tool_layout.addLayout(self.separator_layout)
        self.add_remove_button_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.add_remove_button_layout)

        # Add tool button
        self.add_tool_button = QtWidgets.QPushButton('Add Tool')
        self.add_tool_button.setObjectName('Primary')
        self.add_tool_button.setFixedHeight(50)
        self.add_tool_button.clicked.connect(self.add_tool_clicked)
        self.add_remove_button_layout.addWidget(self.add_tool_button)

        # Remove Tool Button
        self.remove_tool_button = QtWidgets.QPushButton('Remove Tool')
        self.remove_tool_button.setObjectName('Secondary')
        self.remove_tool_button.setFixedSize(100, 50)
        self.remove_tool_button.clicked.connect(self.remove_tool_clicked)
        self.add_remove_button_layout.addWidget(self.remove_tool_button)

        # ++++++++++++++++++++++++--->>>
        # BOTTOM CONTROL WIDGETS +---->>>
        # ++++++++++++++++++++++++--->>>

        self.bottom_control_widget = QtWidgets.QWidget()
        self.bottom_control_widget.setFixedHeight(100)
        self.bottom_control_layout = QtWidgets.QVBoxLayout()
        self.bottom_control_layout.setContentsMargins(0, 0, 0, 0)
        self.bottom_control_widget.setLayout(self.bottom_control_layout)
        self.panel_frame = QtWidgets.QFrame(self.bottom_control_widget)
        self.panel_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        self.panel_frame.setGeometry(0, 0, 5000, 100)
        self.content_layout.addWidget(self.bottom_control_widget)
        self.open_section()

    def open_section(self):
        """ Initializes Tools Window """
        self.set_list_widget()
        self.set_markdown_window(self.markdown)

    def close_section(self):
        if self.active_section_button:
            self.active_section_button.setEnabled(True)

    def add_section_button(self, section_name):
        _LOGGER.info(f'sectionName: {section_name}')
        self.section_button = QtWidgets.QPushButton(section_name)
        self.section_button.setFont(constants.BOLD_FONT)
        self.section_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.section_button.setFixedHeight(50)
        self.section_button.setObjectName('Primary')
        self.section_button.clicked.connect(self.section_button_clicked)
        return self.section_button

    def reset_tool_form(self):
        self.tool_name_field.setText('')
        self.tool_category_combobox.setCurrentIndex(0)
        self.tool_description_field.setText('')
        self.tool_location_field.setText('')
        self.tool_documentation_field.setText('')
        self.tool_markdown_field.setText('')

    def validate_path(self, target_path, file_extension):
        if not Path(target_path).is_file() or Path(target_path).suffix != file_extension:
            return False
        return True

    def throw_validation_message(self, failed_validation):
        error_fields = []
        error_message = 'Check the following fields and try to submit again:\n\n'
        for item in failed_validation:
            error_message += f'{item[0]}\n'
            error_fields.append(item[1])
        error_message = error_message[:-1]

        button = QMessageBox.critical(self, 'Validation Failed', error_message, buttons=QMessageBox.Ok)
        if button == QMessageBox.Ok:
            for field in error_fields:
                field.clear()

    def validate_tool_information(self):
        tool_settings = {
            'Category': [None, self.tool_category_combobox.currentText()],
            'Description': [None, self.tool_description_field.toPlainText()],
            'Documentation': ['.html', self.tool_documentation_field],
            'Markdown': ['.md', self.tool_markdown_field],
            'Start File': ['.py', self.tool_location_field],
            'Name': ['input', self.tool_name_field]
        }

        failed_validation = []
        for field_key, field_values in tool_settings.items():
            entered_value = field_values[1] if isinstance(field_values[1], str) else field_values[1].text()
            if entered_value:
                file_extension = field_values[0]
                if not file_extension:
                    pass
                elif file_extension == 'input':
                    if not entered_value.replace(' ', '').isalnum() or len(entered_value) == 0:
                        failed_validation.append(['Name', self.tool_name_field])
                        continue
                else:
                    if not self.validate_path(entered_value, file_extension):
                        failed_validation.append([field_key, field_values[1]])
                        continue
                tool_settings[field_key] = entered_value

        # Rename keys for database matching
        existing_keys = copy(list(tool_settings.keys()))
        for key in existing_keys:
            new_key = 'tool' + key.replace(' ', '')
            new_value = tool_settings.get(key)
            tool_settings[new_key] = new_value if not isinstance(new_value, list) else None
            del tool_settings[key]

        if not failed_validation:
            self.model.set_tool(self.get_db_formatting(tool_settings))
            # Add Stacked Layout switch here
        else:
            self.throw_validation_message(failed_validation)

    def get_db_formatting(self, tool_settings):
        _LOGGER.info(f'ToolSettings: {tool_settings}')
        entry_list = []
        tool_settings['toolId'] = self.model.get_tool_id()
        tool_settings['toolPlacement'] = self.window_radio_button_group.checkedButton().text()
        for attribute in self.model.tools_headers:
            _LOGGER.info(f'++++ Attribute: {attribute}')
            target_value = None
            for key, value in tool_settings.items():
                if key == attribute:
                    _LOGGER.info(f'TargetKey: {key} -----> {value}')
                    target_value = value
                    break
            entry_list.append(target_value)

        return entry_list

    def get_tool_button(self, label_text):
        self.tool_button = QtWidgets.QPushButton()
        self.tool_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.tool_button.setText(label_text)
        self.tool_button.setFixedSize(200, 35)
        self.tool_button.setStyleSheet("QPushButton {border: 1px solid black; color: #00b4ef; background-color: "
                                       "qlineargradient(spread:pad, x1:1, y1:0, x2:1, y2:1, stop:0 "
                                       "rgba(100, 100, 100, 255), stop:1 rgba(60, 60, 60, 255));} QPushButton:hover {"
                                       "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #888888, "
                                       "stop:1 #777777); border: 1px solid #FFFFFF;}")
        self.tool_button.setFont(constants.BOLD_FONT)
        self.tool_button.clicked.connect(self.tool_button_clicked)
        return self.tool_button

    def get_separator_bar(self):
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QFrame()
        self.line.setStyleSheet('color: rgb(20, 20, 20);')
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setLineWidth(1)
        self.separator_layout.addWidget(self.line)
        return self.separator_layout

    def set_section(self, button_name, target_button):
        self.close_section()
        target_button.setEnabled(False)
        self.active_section_button = target_button
        self.current_section = button_name
        self.set_tools(button_name)

    def set_tools(self, section_name):
        if self.tool_listings_layout.count():
            for i in reversed(range(self.tool_listings_layout.count())):
                self.tool_listings_layout.itemAt(i).widget().setParent(None)
        if len(self.tools_listings[section_name].values()):
            for i in range(len(self.tools_listings[section_name])):
                btn = self.get_tool_button(self.tools_listings[section_name][i])
                self.tool_listings_layout.addWidget(btn)

    def set_list_widget(self):
        if self.current_section:
            if self.tools_listings[self.current_section]:
                self.center_column_container.setCurrentIndex(2)
            else:
                self.center_column_container.setCurrentIndex(1)
        else:
            self.center_column_container.setCurrentIndex(0)

    def set_markdown_window(self, markdown_file):
        formatted_text = self.model.convert_markdown(markdown_file)
        self.markdown_display.setText(formatted_text)

    # ++++++++++++++++--->>>
    # BUTTON ACTIONS +---->>>
    # ++++++++++++++++--->>>

    def add_button_clicked(self):
        self.add_cancel_button_layout.setCurrentIndex(1)
        self.stacked_layout.setCurrentIndex(1)

    def section_button_clicked(self):
        signal_sender = self.sender()
        button_name = signal_sender.text()
        self.set_section(button_name, self.section_buttons[button_name])
        self.set_list_widget()

    def add_tool_clicked(self):
        self.validate_tool_information()

    def browse_clicked(self, _id):
        browse_info = [
            ['Select Python Start File (.py)', 'Python Files (*.py)', self.tool_location_field],
            ['Select Documentation file (.html)', 'HTML Files (*.html)', self.tool_documentation_field],
            ['Select Markdown Instructions (.md)', 'Markdown Files (*.md)', self.tool_markdown_field]
        ]
        dialog = QtWidgets.QFileDialog()
        dialog.setDirectory(self.desktop_location)
        dialog.setWindowTitle(browse_info[_id][0])
        dialog.setNameFilter(browse_info[_id][1])
        dialog.setFileMode(QtWidgets.QFileDialog.ExistingFile)
        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            browse_info[_id][2].setText(dialog.selectedFiles()[0])

    def remove_tool_clicked(self):
        _LOGGER.info('Remove Tool Button Clicked')

    def tool_button_clicked(self):
        signal_sender = self.sender()
        button_name = signal_sender.text()
        _LOGGER.info(f'CurrentSection: {self.current_section}  SelectedScript: {button_name}')

    def cancel_tool_clicked(self):
        _LOGGER.info('Cancel Tool Button Clicked')
        self.add_cancel_button_layout.setCurrentIndex(0)
        self.stacked_layout.setCurrentIndex(0)

