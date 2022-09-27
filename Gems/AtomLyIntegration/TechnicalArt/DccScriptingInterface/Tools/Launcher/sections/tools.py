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
from PySide2.QtWidgets import QMessageBox, QFrame, QStyle
from Tools.Launcher.data import project_constants as constants
from pathlib import Path
from box import Box
from copy import copy
from azpy import config_utils
import logging


_LOGGER = logging.getLogger('Launcher.tools')


class Tools(QtWidgets.QWidget):
    def __init__(self, model):
        super(Tools, self).__init__()

        self.model = model
        self.tools_listings = self.model.tools
        self.markdown = (settings.get('PATH_DCCSI_TOOLS') /
                         'Launcher/markdown/PycharmDebugging/pycharm_debugging.md').as_posix()
        self.desktop_location = (Path.home() / 'Desktop').as_posix()
        self.current_section = None
        self.active_tool_button = None
        self.tool_buttons = {}
        self.tool_button = None
        self.content_layout = QtWidgets.QVBoxLayout(self)
        self.content_layout.setContentsMargins(5, 0, 5, 5)
        self.content_frame = QFrame(self)
        self.content_frame.setGeometry(0, 0, 5000, 5000)
        self.section_tabs = QtWidgets.QTabBar()
        self.section_tabs.setShape(QtWidgets.QTabBar.RoundedNorth)
        self.section_tabs.currentChanged.connect(self.section_button_clicked)
        self.section_tabs.setStyleSheet(constants.TAB_STYLESHEET)
        self.section_tabs.setFont(constants.BOLD_FONT_LARGE)
        self.content_layout.addWidget(self.section_tabs)
        self.content_layout.addSpacing(-5)

        # Button Groups
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
        self.get_started_frame = QFrame(self.get_started_panel)
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
        self.add_tool_frame = QFrame(self.add_tool_panel)
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

        self.listings_panel_container = QtWidgets.QVBoxLayout()
        self.listings_panel_container.setContentsMargins(0, 0, 0, 0)
        self.listings_panel_widget = QtWidgets.QWidget()
        self.listings_panel_widget.setLayout(self.listings_panel_container)
        self.center_column_container.addWidget(self.listings_panel_widget)

        # Top Bracket --------------------->>
        self.top_bracket_layout = QtWidgets.QVBoxLayout()
        self.top_bracket_layout.setAlignment(QtCore.Qt.AlignBottom)
        self.top_bracket_layout.setContentsMargins(0, 0, 0, 0)
        self.top_bracket_widget = QtWidgets.QWidget()
        self.top_bracket_widget.setFixedSize(200, 60)
        self.top_bracket_widget.setLayout(self.top_bracket_layout)
        self.top_bracket_frame = QFrame(self.top_bracket_widget)
        self.top_bracket_frame.setGeometry(0, 0, 200, 60)
        self.top_bracket_frame.setStyleSheet("QFrame { background-color: qlineargradient(spread:pad, x1:1, y1:0, x2:1, "
                                             "y2:1, stop:0 rgba(55, 55, 55, 255), stop:1 rgba(15, 15, 15, 255));}")
        self.listings_panel_container.addSpacing(5)
        self.listings_panel_container.addWidget(self.top_bracket_widget)

        # Add Tool Button
        self.add_tool_layout = QtWidgets.QHBoxLayout()
        self.add_tool_layout.setAlignment(QtCore.Qt.AlignRight)
        self.add_tool_button = QtWidgets.QPushButton('+')
        self.add_tool_button.setFont(constants.BOLD_FONT)
        self.add_tool_button.setFixedSize(22, 22)
        self.add_tool_button.setObjectName('Primary')
        self.add_tool_button.clicked.connect(self.add_tool_clicked)
        self.add_tool_layout.addWidget(self.add_tool_button)
        self.add_tool_layout.addSpacing(5)
        self.top_bracket_layout.addLayout(self.add_tool_layout)
        self.top_bracket_layout.addSpacing(5)

        # Tool Listings --------------------->>
        self.tool_listings_layout = QtWidgets.QVBoxLayout()
        self.tool_listings_layout.setAlignment(QtCore.Qt.AlignTop)
        self.tool_listings_panel = QtWidgets.QWidget()
        self.tool_listings_panel.setLayout(self.tool_listings_layout)
        self.tool_listings_layout.setContentsMargins(0, 0, 0, 0)
        self.tool_listings_layout.setSpacing(0)
        self.panel_frame = QFrame(self.tool_listings_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        self.panel_frame.setGeometry(0, 0, 200, 5000)
        self.listings_panel_container.addWidget(self.tool_listings_panel)

        # +++++++++++++++++++++++--->>>
        # CONTENT AREA WIDGETS +---->>>
        # +++++++++++++++++++++++--->>>

        self.stacked_layout = QtWidgets.QStackedLayout()
        self.stacked_widget = QtWidgets.QWidget()
        self.stacked_widget.setLayout(self.stacked_layout)
        self.page_splitter.addWidget(self.stacked_widget)

        ###############
        # SPLASH PAGE #
        ###############

        self.splash_page_layout = QtWidgets.QVBoxLayout()
        self.splash_page_layout.setAlignment(QtCore.Qt.AlignCenter)
        self.splash_page_widget = QtWidgets.QWidget()
        self.splash_page_widget.setLayout(self.splash_page_layout)
        self.panel_frame = QFrame(self.splash_page_widget)
        self.panel_frame.setStyleSheet('background-color: #000000;')
        self.panel_frame.setGeometry(0, 5, 5000, 5000)
        self.stacked_layout.addWidget(self.splash_page_widget)

        # ++++++++++++++++++++++++
        # Splash Tool Info +++++++
        # ++++++++++++++++++++++++

        # Main Content Box
        self.tool_info_layout = QtWidgets.QVBoxLayout()
        self.tool_info_layout.setContentsMargins(0, 0, 0, 0)
        self.tool_info_layout.setAlignment(QtCore.Qt.AlignTop)
        self.tool_info_widget = QtWidgets.QWidget()
        self.tool_info_widget.setLayout(self.tool_info_layout)
        self.tool_info_frame = QFrame(self.tool_info_widget)
        self.tool_info_frame.setStyleSheet('background-color: rgb(50, 50, 50)')
        self.tool_info_frame.setGeometry(0, 0, 1000, 1000)
        self.tool_info_widget.setFixedSize(800, 450)
        self.splash_page_layout.addWidget(self.tool_info_widget)

        # Blue Top Border
        self.blue_border_layout = QtWidgets.QVBoxLayout()
        self.blue_border_layout.setAlignment(QtCore.Qt.AlignBottom)
        self.blue_border_layout.setContentsMargins(7, 0, 0, 7)
        self.blue_border_widget = QtWidgets.QWidget()
        self.blue_border_widget.setFixedSize(800, 105)
        self.blue_border_widget.setLayout(self.blue_border_layout)
        self.blue_border_frame = QFrame(self.blue_border_widget)
        self.blue_border_frame.setGeometry(0, 0, 1000, 1000)
        self.blue_border_frame.setStyleSheet('background-color: rgb(27, 105, 221)')
        self.tool_info_layout.addWidget(self.blue_border_widget)

        # Section Name Label
        self.splash_section_label = QtWidgets.QLabel()
        self.splash_section_label.setFont(constants.BOLD_FONT_SPLASH)
        self.splash_section_label.setAlignment(QtCore.Qt.AlignRight)
        self.splash_section_label.setStyleSheet('color: white; padding-right: 2px;')
        self.blue_border_layout.addWidget(self.splash_section_label)
        self.blue_border_layout.addSpacing(35)

        # Tool Name Label
        self.top_border_label_layout = QtWidgets.QHBoxLayout()
        self.blue_border_layout.addLayout(self.top_border_label_layout)

        self.border_left_layout = QtWidgets.QHBoxLayout()
        self.top_border_label_layout.addLayout(self.border_left_layout)
        self.border_left_layout.addSpacing(10)

        self.section_name_label = QtWidgets.QLabel('Tool Name')
        self.section_name_label.setFont(constants.BOLD_FONT_LARGE)
        self.section_name_label.setStyleSheet('color: white')
        self.border_left_layout.addWidget(self.section_name_label)

        self.border_right_layout = QtWidgets.QHBoxLayout()
        self.border_right_layout.setAlignment(QtCore.Qt.AlignRight)
        self.top_border_label_layout.addLayout(self.border_right_layout)

        self.tool_type_label = QtWidgets.QLabel('Tool Category')
        self.tool_type_label.setFont(constants.BOLD_FONT_LARGE)
        self.tool_type_label.setStyleSheet('color: white')
        self.border_right_layout.addWidget(self.tool_type_label)
        self.border_right_layout.addSpacing(15)
        self.tool_info_layout.addSpacing(5)

        # Tool Name Info Field
        self.listing_one_layout = QtWidgets.QHBoxLayout()
        self.listing_one_layout.setContentsMargins(0, 0, 0, 0)
        self.tool_name_info_widget = QtWidgets.QWidget()
        self.tool_name_info_widget.setLayout(self.listing_one_layout)
        self.tool_name_info_widget.setFixedHeight(30)
        self.tool_name_layout = QtWidgets.QHBoxLayout()
        self.tool_cat_layout = QtWidgets.QHBoxLayout()
        self.tool_cat_layout.setAlignment(QtCore.Qt.AlignRight)
        self.listing_one_layout.addLayout(self.tool_name_layout)
        self.listing_one_layout.addLayout(self.tool_cat_layout)
        self.tool_info_layout.addWidget(self.tool_name_info_widget)

        self.name_label = QtWidgets.QLabel()
        self.name_label.setStyleSheet('color: white; padding-left: 10px;')
        self.name_label.setFont(constants.BOLD_FONT_XTRA_LARGE)
        self.tool_name_layout.addWidget(self.name_label)

        self.tool_cat_label = QtWidgets.QLabel()
        self.tool_cat_label.setStyleSheet('color: white; padding-right: 10px;')
        self.tool_cat_label.setFont(constants.BOLD_FONT_XTRA_LARGE)
        self.tool_cat_layout.addWidget(self.tool_cat_label)

        # Description
        # self.tool_info_layout.addSpacing(30)
        self.description_header_layout = QtWidgets.QHBoxLayout()
        self.description_header_layout.setContentsMargins(0, 0, 0, 0)
        self.description_header_widget = QtWidgets.QWidget()
        self.description_header_widget.setLayout(self.description_header_layout)
        self.description_header_widget.setFixedSize(800, 25)
        self.black_header_frame = QFrame(self.description_header_widget)
        self.black_header_frame.setGeometry(0, 0, 1000, 1000)
        self.black_header_frame.setStyleSheet('background-color: rgb(25, 25, 25);')
        self.description_label = QtWidgets.QLabel('Description')
        self.description_label.setFont(constants.BOLD_FONT_LARGE)
        self.description_label.setStyleSheet('padding-left: 15px; color: white')
        self.description_header_layout.addWidget(self.description_label)
        self.tool_info_layout.addWidget(self.description_header_widget)
        self.tool_info_layout.addSpacing(-5)

        # Description Text
        self.description_text = QtWidgets.QTextEdit()
        self.description_text.setStyleSheet('padding-left: 15px; padding-right: 15px;')
        self.description_text.setReadOnly(True)
        self.tool_info_layout.addWidget(self.description_text)

        # Launch Button
        self.launch_button_container = QtWidgets.QHBoxLayout()
        self.launch_button_container.setContentsMargins(5, 0, 5, 5)
        self.launch_tool_button = QtWidgets.QPushButton('Launch')
        self.launch_tool_button.clicked.connect(self.launch_button_clicked)
        self.launch_tool_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.launch_tool_button.setFixedHeight(50)
        self.launch_button_container.addWidget(self.launch_tool_button)
        self.tool_info_layout.addLayout(self.launch_button_container)
        self.launch_tool_button.setObjectName('Primary')

        ####################
        # MARKDOWN DISPLAY #
        ####################

        self.markdown_display_layout = QtWidgets.QVBoxLayout()
        self.markdown_display_layout.setContentsMargins(0, 5, 0, 0)
        self.markdown_display_widget = QtWidgets.QWidget()
        self.markdown_display_widget.setLayout(self.markdown_display_layout)
        self.markdown_display = QtWidgets.QTextEdit()
        self.markdown_display_layout.addWidget(self.markdown_display)
        self.stacked_layout.addWidget(self.markdown_display_widget)

        #################
        # ADD TOOL FORM #
        #################

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
        self.add_tool_layout.addSpacing(6)
        
        # Python Distribution
        self.distribution_row_controls = QtWidgets.QHBoxLayout()
        self.add_tool_layout.addLayout(self.distribution_row_controls)

        self.python_distribution_layout = QtWidgets.QVBoxLayout()
        self.distribution_row_controls.addLayout(self.python_distribution_layout)
        self.python_distribution_label_layout = QtWidgets.QHBoxLayout()
        self.python_distribution_label_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.python_distribution_layout.addLayout(self.python_distribution_label_layout)

        self.python_distribution_label = QtWidgets.QLabel('Python Distribution')
        self.python_distribution_label_layout.addWidget(self.python_distribution_label)
        self.help_button = QtWidgets.QPushButton()
        self.help_button.setToolTip('Set this if running Python\nTools designed for specific\nDCC Applications')
        self.help_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.help_button.setFixedSize(20, 20)
        self.help_button.setStyleSheet('border: None;')
        pixmapi = QStyle.SP_MessageBoxQuestion
        self.help_icon = self.style().standardIcon(pixmapi)
        self.help_button.setIcon(self.help_icon)
        self.python_distribution_label_layout.addWidget(self.help_button)

        self.python_distribution_combobox = QtWidgets.QComboBox()
        self.python_distribution_combobox.setFixedHeight(25)
        self.python_distributions = (
            'O3DE Python',
            'Maya Python',
            'Blender Python',
            'Houdini Python',
            '3dsMax Python'
        )
        self.python_distribution_layout.addWidget(self.python_distribution_combobox)
        self.python_distribution_combobox.addItems(self.python_distributions)
        self.distribution_row_controls.addSpacing(40)

        # Category
        self.tool_category_layout = QtWidgets.QVBoxLayout()
        self.distribution_row_controls.addLayout(self.tool_category_layout)
        self.tool_category_label = QtWidgets.QLabel('Category')
        self.tool_category_layout.addWidget(self.tool_category_label)
        self.tool_category_combobox = QtWidgets.QComboBox()
        self.tool_category_combobox.setFixedHeight(25)
        self.tool_categories = []
        self.tool_category_layout.addWidget(self.tool_category_combobox)
        self.tool_category_combobox.addItems((
            'Utility',
            'Workflow',
            'Conversion',
            'Add New Category'
        ))
        self.distribution_row_controls.addSpacing(40)

        # Tool window placement
        self.align_window_layout = QtWidgets.QVBoxLayout()
        self.align_window_layout.setAlignment(QtCore.Qt.AlignBottom)
        self.distribution_row_controls.addLayout(self.align_window_layout)
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
        self.add_tool_layout.addSpacing(6)

        # Description
        self.tool_description_label = QtWidgets.QLabel('Description')
        self.add_tool_layout.addWidget(self.tool_description_label)
        self.tool_description_field = QtWidgets.QTextEdit()
        self.add_tool_layout.addWidget(self.tool_description_field)
        self.add_tool_layout.addSpacing(6)

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
        self.add_tool_button.clicked.connect(self.add_tool_info_clicked)
        self.add_remove_button_layout.addWidget(self.add_tool_button)

        # Remove Tool Button
        self.remove_tool_button = QtWidgets.QPushButton('Remove Tool')
        self.remove_tool_button.setObjectName('Secondary')
        self.remove_tool_button.setFixedSize(100, 50)
        self.remove_tool_button.clicked.connect(self.remove_tool_clicked)
        self.add_remove_button_layout.addWidget(self.remove_tool_button)

        # Cancel Button
        self.cancel_button = QtWidgets.QPushButton('Cancel')
        self.cancel_button.setObjectName('Secondary')
        self.cancel_button.setFixedSize(70, 50)
        self.cancel_button.clicked.connect(self.cancel_clicked)
        self.add_remove_button_layout.addWidget(self.cancel_button)

        ##################
        # TOOL CONTAINER #
        ##################

        self.tool_layout = QtWidgets.QHBoxLayout()
        self.tool_layout.setContentsMargins(0, 5, 0, 0)
        self.tool_widget = QtWidgets.QWidget()
        self.tool_widget.setLayout(self.tool_layout)
        self.stacked_layout.addWidget(self.tool_widget)
        tiling_image = (settings.get('PATH_DCCSI_TOOLS') / 'Launcher/images/checker_background.png').as_posix()

        # Left Margin Spacer ----->>
        self.left_tool_frame = QFrame()
        self.left_tool_frame.setFrameStyle(QFrame.StyledPanel)
        self.left_tool_frame.setStyleSheet(f'background-image: url({tiling_image})')
        self.left_container_layout = QtWidgets.QVBoxLayout(self.left_tool_frame)
        self.tool_layout.addWidget(self.left_tool_frame)

        #  Loaded Tool Container ----->>
        self.scroll_content_layout = QtWidgets.QVBoxLayout()
        self.tool_layout.addLayout(self.scroll_content_layout)
        self.scroll_content_layout.addStretch(0)

        self.tool_container_widget = QtWidgets.QWidget()
        self.tool_container_layout = QtWidgets.QHBoxLayout()

        self.scroll_area = QtWidgets.QScrollArea(self.tool_container_widget)
        self.scroll_area.setFrameShape(QFrame.NoFrame)
        self.scroll_area_contents = QtWidgets.QWidget()
        self.scroll_area_contents.setLayout(self.tool_container_layout)
        self.scroll_area.setWidgetResizable(True)
        self.scroll_area.setWidget(self.scroll_area_contents)
        self.scroll_content_layout.addWidget(self.scroll_area)
        # self.scroll_content_layout.addStretch(0)

        # Right Margin Spacer ----->>
        self.right_tool_frame = QFrame()
        self.right_tool_frame.setFrameStyle(QFrame.StyledPanel)
        self.right_tool_frame.setStyleSheet(f'background-image: url({tiling_image})')
        self.right_container_layout = QtWidgets.QVBoxLayout(self.right_tool_frame)
        self.tool_layout.addWidget(self.right_tool_frame)

    def open_section(self):
        """ Initializes Tools Window """
        if not self.tool_categories:
            for tab_name in self.model.get_tools_categories():
                self.tool_categories.append(tab_name)
                self.section_tabs.addTab(tab_name)
        self.set_list_widget()
        self.set_tool_preview()

    def close_section(self):
        self.set_tool_button()

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

        validation_data = []
        for field_key, field_values in tool_settings.items():
            entered_value = field_values[1] if isinstance(field_values[1], str) else field_values[1].text()
            if entered_value:
                file_extension = field_values[0]
                if not file_extension:
                    pass
                elif file_extension == 'input':
                    if not entered_value.replace(' ', '').isalnum() or len(entered_value) == 0:
                        validation_data.append(['Name', self.tool_name_field])
                        continue
                else:
                    if not self.validate_path(entered_value, file_extension):
                        validation_data.append([field_key, field_values[1]])
                        continue
                tool_settings[field_key] = entered_value
        _LOGGER.info(f'Tool settings before filter: {tool_settings}')
        # Rename keys for database matching
        existing_keys = copy(list(tool_settings.keys()))
        for key in existing_keys:
            new_key = 'tool' + key.replace(' ', '')
            new_value = tool_settings.get(key)
            tool_settings[new_key] = new_value if not isinstance(new_value, list) else None
            del tool_settings[key]

        if not validation_data:
            _LOGGER.info(f'Setting Tool [{self.current_section}]: {tool_settings}')
            tool_registered = self.model.set_tool(self.get_db_formatting(tool_settings))
            self.center_column_container.setCurrentIndex(2)
            self.set_tools(self.current_section)
            if tool_registered:
                return True
        else:
            self.throw_validation_message(validation_data)
        return False

    def update_tools_listings(self):
        _LOGGER.info(f'Update... current tool listings: {self.tools_listings}')

    def reset_form(self):
        self.tool_name_field.clear()
        self.tool_category_combobox.setCurrentIndex(0)
        self.embedded_radio_button.setChecked(True)
        self.tool_description_field.clear()
        self.tool_location_field.clear()
        self.tool_documentation_field.clear()
        self.tool_markdown_field.clear()

    def reset_tool_window(self):
        self.active_tool_button = None
        self.set_tool_preview()

    def get_db_formatting(self, tool_settings):
        entry_list = []
        tool_settings['toolSection'] = self.current_section
        tool_settings['toolPlacement'] = self.window_radio_button_group.checkedButton().text()

        for attribute in constants.TOOLS_HEADERS:
            target_value = None
            for key, value in tool_settings.items():
                if key == attribute:
                    target_value = value
                    break
            entry_list.append(target_value)

        return entry_list

    def get_tool_button(self, label_text):
        self.tool_button = QtWidgets.QPushButton()
        self.tool_button.setCursor(QtGui.QCursor(QtCore.Qt.PointingHandCursor))
        self.tool_button.setText(label_text)
        self.tool_button.setFixedSize(200, 35)
        self.tool_button.setStyleSheet(constants.ACTIVE_BUTTON_STYLESHEET)
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

    def set_section(self, button_name):
        self.current_section = button_name
        self.set_splash_page(button_name)
        self.set_tools(button_name)

    def set_tool(self):
        tool_values = self.tools_listings[self.current_section][self.active_tool_button]
        module_path = tool_values['toolStartFile']
        module_name = f"{Path(tool_values['toolStartFile']).parent.name}.{Path(tool_values['toolStartFile']).stem}"
        class_name = ''.join(self.active_tool_button.split(' '))

        _LOGGER.info(f'ModuleName: {module_name}   ModulePath: {module_path}   ClassName: {class_name}')
        target_module = config_utils.load_module_by_path(module_name, module_path)
        if tool_values['toolPlacement'] == 'Embedded':
            self.stacked_layout.setCurrentIndex(3)
            tool_widget = target_module.get_tool()
            self.tool_container_layout.addWidget(tool_widget)

    def set_tools(self, section_name):
        # Clear existing section buttons
        if self.tool_listings_layout.count():
            self.tool_buttons.clear()
            for i in reversed(range(self.tool_listings_layout.count())):
                self.tool_listings_layout.itemAt(i).widget().setParent(None)

        # Add selected section buttons
        if len(self.tools_listings[section_name].values()):
            for key, value in self.tools_listings[section_name].items():
                btn = self.get_tool_button(key)
                self.tool_buttons[key] = btn
                self.tool_listings_layout.addWidget(btn)

    def set_tool_button(self, selected=None):
        if self.active_tool_button:
            self.tool_buttons[self.active_tool_button].setStyleSheet(constants.ACTIVE_BUTTON_STYLESHEET)
        if selected:
            self.active_tool_button = selected
            self.tool_buttons[self.active_tool_button].setStyleSheet(constants.DISABLE_BUTTON_STYLESHEET)
            self.launch_tool_button.setEnabled(True)
            self.set_tool_preview()
        else:
            self.active_tool_button = None
            self.launch_tool_button.setEnabled(False)

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

    def set_splash_page(self, section_name):
        self.splash_section_label.setText(section_name)

    def set_tool_preview(self):
        if self.active_tool_button:
            tool_values = self.tools_listings[self.current_section][self.active_tool_button]
            self.name_label.setText(tool_values['toolName'])
            self.tool_cat_label.setText(tool_values['toolCategory'])
            self.description_text.setPlainText(tool_values['toolDescription'])
        else:
            self.name_label.setText('No tool selected')
            self.tool_cat_label.setText('-')
            self.description_text.setPlainText('Select tool button in left column to access tool listings.')

    # ++++++++++++++++--->>>
    # BUTTON ACTIONS +---->>>
    # ++++++++++++++++--->>>

    def add_button_clicked(self):
        self.add_cancel_button_layout.setCurrentIndex(1)
        self.stacked_layout.setCurrentIndex(2)

    def section_button_clicked(self):
        self.active_tool_button = None
        self.launch_tool_button.setEnabled(False)
        button_name = self.model.get_tools_categories()[self.section_tabs.currentIndex()]
        self.reset_tool_window()
        self.set_section(button_name)
        self.set_list_widget()

    def add_tool_info_clicked(self):
        _LOGGER.info('Add Tool clicked')
        if self.validate_tool_information():
            self.update_tools_listings()
            section_name = self.section_tabs.tabText(self.section_tabs.currentIndex())
            self.set_tools(section_name)
            self.stacked_layout.setCurrentIndex(0)

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
        if button_name != self.active_tool_button:
            self.set_tool_button(button_name)

    def cancel_tool_clicked(self):
        self.add_cancel_button_layout.setCurrentIndex(0)
        self.stacked_layout.setCurrentIndex(0)

    def launch_button_clicked(self):
        _LOGGER.info(f'Launch Clicked... Values: {self.tools_listings[self.current_section][self.active_tool_button]}')
        self.set_tool()

    def add_tool_clicked(self):
        self.stacked_layout.setCurrentIndex(2)

    def cancel_clicked(self):
        _LOGGER.info('Cancel clicked')
        self.reset_form()
        self.stacked_layout.setCurrentIndex(0)
