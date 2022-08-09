from PySide2 import QtWidgets, QtCore, QtGui
from pathlib import Path
from azpy.shared.qt_process import QtProcess
import psutil
import logging
import config
import json
import os
from dynaconf import settings


_LOGGER = logging.getLogger('Launcher.setup')

# TODO - Create icons for active and inactive- you also need to figure out how to add numbers to the end to show
# override ranking


class Setup(QtWidgets.QWidget):
    def __init__(self, model):
        super(Setup, self).__init__()

        _LOGGER.info('Setup Page added to content layout')
        self.model = model
        self.content_layout = QtWidgets.QHBoxLayout(self)
        self.content_layout.setContentsMargins(10, 3, 0, 0)
        self.content_frame = QtWidgets.QFrame(self)
        self.bold_font = QtGui.QFont("Helvetica", 7, QtGui.QFont.Bold)

        self.dcc_paths = {}
        self.dynaconf_environment_values = {}
        self.environment_key_structure = {}
        self.logging_profiles = []
        self.environment_list = []
        self.settings_file = Path(settings.get('PATH_DCCSIG')) / 'settings.json'
        self.supported_ides = ['vscode', 'pycharm', 'wing']

        self.add_button_group = QtWidgets.QButtonGroup()
        self.add_button_group.idClicked.connect(self.add_button_clicked)

        # Left Margin Gutter
        # self.left_margin_gutter_widget = QtWidgets.QWidget()
        # self.left_margin_gutter_widget.setFixedWidth(3)
        # self.content_layout.addWidget(self.left_margin_gutter_widget)

        self.page_splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        self.page_splitter.setSizes([200, 540, 540])
        self.content_layout.addWidget(self.page_splitter)

        # +++++++++++++++++++++--->>>
        # LEFT COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++--->>>

        self.left_column_widget = QtWidgets.QWidget()
        self.left_column_widget.setMaximumWidth(300)
        self.left_column_container = QtWidgets.QVBoxLayout()
        self.left_column_container.setContentsMargins(0, 0, 0, 0)
        self.left_column_widget.setLayout(self.left_column_container)
        self.page_splitter.addWidget(self.left_column_widget)

        # Startup Environment Panel ########################################
        self.startup_panel_layout = QtWidgets.QVBoxLayout()
        self.startup_panel_layout.setAlignment(QtCore.Qt.AlignTop)
        self.startup_panel = QtWidgets.QWidget()
        self.startup_panel.setLayout(self.startup_panel_layout)
        self.startup_panel_layout.setContentsMargins(0, 0, 0, 0)
        self.panel_frame = QtWidgets.QFrame(self.startup_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(135, 135, 135);')
        self.panel_frame.setGeometry(0, 0, 300, 5000)
        self.left_column_container.addWidget(self.startup_panel)

        # --> Startup Environment Title Bar
        self.startup_title_layout = QtWidgets.QHBoxLayout()
        self.startup_widget = self.get_title_bar_widget(self.startup_title_layout, 'Startup Environment', 300)
        self.startup_panel_layout.addWidget(self.startup_widget)
        self.add_startup_button = QtWidgets.QPushButton('+')
        self.add_button_group.addButton(self.add_startup_button, 0)
        self.add_startup_button.setFont(self.bold_font)
        self.add_startup_button.setFixedSize(22, 22)
        self.add_startup_button.setObjectName('Primary')
        self.startup_title_layout.addWidget(self.add_startup_button)

        # --> Startup Environment Content
        self.startup_environment_container = QtWidgets.QVBoxLayout()
        self.startup_environment_container.setContentsMargins(10, 3, 10, 10)
        self.startup_panel_layout.addLayout(self.startup_environment_container)

        # DCCsi Environments Table
        self.dccsi_environments_label = QtWidgets.QLabel('DCCSI ENVIRONMENTS')
        self.startup_environment_container.addWidget(self.dccsi_environments_label)

        self.dccsi_environment_listbox = EnvironmentListWidget(self.model, self)
        self.dccsi_environment_listbox.setFocusPolicy(QtCore.Qt.NoFocus)
        self.dccsi_environment_listbox.clicked.connect(self.environment_clicked)
        self.dccsi_environment_listbox.setAlternatingRowColors(True)
        self.dccsi_environment_listbox.setSelectionMode(QtWidgets.QListWidget.NoSelection)
        self.dccsi_environment_listbox.horizontalScrollBar().setEnabled(False)
        self.dccsi_environment_listbox.setStyleSheet('QListWidget::item {padding-left:15px;} '
                                                     'QListWidget {alternate-background-color: rgb(85, 85, 85); '
                                                     'background-color: rgb(105, 105, 105); '
                                                     'color: rgb(220, 220, 220);}')
        self.dccsi_environment_listbox.setFixedWidth(280)
        self.startup_environment_container.addWidget(self.dccsi_environment_listbox)
        self.startup_environment_container.addSpacing(8)

        # Custom Environments Table
        self.custom_environments_label = QtWidgets.QLabel('CUSTOM ENVIRONMENTS')
        self.startup_environment_container.addWidget(self.custom_environments_label)

        self.custom_environment_listbox = EnvironmentListWidget(self.model, self)
        self.custom_environment_listbox.setFocusPolicy(QtCore.Qt.NoFocus)
        self.custom_environment_listbox.clicked.connect(self.environment_clicked)
        self.custom_environment_listbox.setAlternatingRowColors(True)
        self.custom_environment_listbox.setSelectionMode(QtWidgets.QListWidget.NoSelection)
        self.custom_environment_listbox.horizontalScrollBar().setEnabled(False)
        self.custom_environment_listbox.setStyleSheet('QListWidget::item {padding-left:15px;} '
                                                     'QListWidget {alternate-background-color: rgb(85, 85, 85); '
                                                     'background-color: rgb(105, 105, 105); '
                                                     'color: rgb(220, 220, 220);}')
        self.custom_environment_listbox.setFixedWidth(280)
        self.startup_environment_container.addWidget(self.custom_environment_listbox)
        self.startup_environment_container.addLayout(self.get_separator_bar())

        self.create_new_button = QtWidgets.QPushButton('Create New Environment')
        self.create_new_button.setFixedHeight(25)
        self.create_new_button.setObjectName('Secondary')
        self.create_new_button.clicked.connect(self.create_new_environment_clicked)
        self.startup_environment_container.addWidget(self.create_new_button)

        self.edit_mode_button = QtWidgets.QPushButton('Enter Edit Mode')
        self.edit_mode_button.setFixedHeight(25)
        self.edit_mode_button.setObjectName('Secondary')
        self.edit_mode_button.clicked.connect(self.create_new_environment_clicked)
        self.startup_environment_container.addWidget(self.edit_mode_button)

        self.restore_defaults_button = QtWidgets.QPushButton('Restore DCCsi Defaults')
        self.restore_defaults_button.setFixedHeight(25)
        self.restore_defaults_button.setObjectName('Secondary')
        self.restore_defaults_button.clicked.connect(self.create_new_environment_clicked)
        self.startup_environment_container.addWidget(self.restore_defaults_button)

        self.open_environments_button = QtWidgets.QPushButton('Launch Environment')
        self.open_environments_button.setFixedHeight(25)
        self.open_environments_button.setObjectName('Primary')
        self.open_environments_button.clicked.connect(self.set_startup_environment_clicked)
        self.startup_environment_container.addWidget(self.open_environments_button)
        # self.left_column_container.addSpacing(3)

        # Logging Profile Panel ###########################################
        self.logging_panel_layout = QtWidgets.QVBoxLayout()
        self.logging_panel_layout.setAlignment(QtCore.Qt.AlignTop)
        self.logging_panel = QtWidgets.QWidget()
        self.logging_panel.setLayout(self.logging_panel_layout)
        self.logging_panel_layout.setContentsMargins(0, 0, 0, 0)
        self.logging_panel_frame = QtWidgets.QFrame(self.logging_panel)
        self.logging_panel_frame.setStyleSheet('background-color: rgb(135, 135, 135);')
        self.logging_panel_frame.setGeometry(0, 0, 300, 5000)
        self.left_column_container.addWidget(self.logging_panel)

        # --> Logging Profile Title Bar
        self.logging_title_layout = QtWidgets.QHBoxLayout()
        self.logging_widget = self.get_title_bar_widget(self.logging_title_layout, 'Logging', 300)
        self.logging_panel_layout.addWidget(self.logging_widget)

        # --> Logging Profile Content
        self.logging_profile_container = QtWidgets.QVBoxLayout()
        self.logging_profile_container.setContentsMargins(10, 3, 10, 10)
        self.logging_panel_layout.addLayout(self.logging_profile_container)

        self.logging_combo_layout = QtWidgets.QHBoxLayout()
        self.logging_profile_container.addLayout(self.logging_combo_layout)
        self.logging_profile_combobox = QtWidgets.QComboBox()
        self.logging_profile_combobox.setFixedHeight(25)
        self.logging_combo_layout.addWidget(self.logging_profile_combobox)
        self.set_profile_button = QtWidgets.QPushButton('Set')
        self.set_profile_button.setEnabled(False)
        self.set_profile_button.setFixedSize(30, 25)
        self.set_profile_button.setObjectName('Secondary')
        self.logging_combo_layout.addWidget(self.set_profile_button)
        self.logging_profile_container.addLayout(self.get_separator_bar())
        self.logging_levels_layout = QtWidgets.QHBoxLayout()
        self.logging_profile_container.addLayout(self.logging_levels_layout)

        self.log_level_label = QtWidgets.QLabel('Log Level')
        self.log_level_label.setFixedWidth(55)
        self.logging_levels_layout.addWidget(self.log_level_label)
        self.log_levels = (
            '0 - NOTSET',
            '10 - DEBUG',
            '20 - INFO',
            '30 - WARNING',
            '40 - ERROR',
            '50 - CRITICAL'
        )
        self.level_combobox = QtWidgets.QComboBox()
        self.level_combobox.setFixedHeight(25)
        self.level_combobox.addItems(self.log_levels)
        self.level_combobox.setCurrentIndex(2)
        self.logging_levels_layout.addWidget(self.level_combobox)
        self.logging_levels_layout.addSpacing(40)

        self.dev_mode_checkbox = QtWidgets.QCheckBox('Dev Mode')
        self.logging_levels_layout.addWidget(self.dev_mode_checkbox)
        # self.left_column_container.addSpacing(3)

        # IDE Panel #######################################################
        self.ide_panel_layout = QtWidgets.QVBoxLayout()
        self.ide_panel_layout.setAlignment(QtCore.Qt.AlignTop)
        self.ide_panel = QtWidgets.QWidget()
        self.ide_panel.setLayout(self.ide_panel_layout)
        self.ide_panel_layout.setContentsMargins(0, 0, 0, 0)
        self.ide_panel_frame = QtWidgets.QFrame(self.ide_panel)
        self.ide_panel_frame.setStyleSheet('background-color: rgb(135, 135, 135);')
        self.ide_panel_frame.setGeometry(0, 0, 300, 5000)
        self.left_column_container.addWidget(self.ide_panel)

        # --> IDE Panel Content
        self.ide_panel_container = QtWidgets.QVBoxLayout()
        self.ide_panel_container.setContentsMargins(5, 5, 5, 5)
        self.ide_panel_layout.addLayout(self.ide_panel_container)
        self.ide_radio_layout = QtWidgets.QHBoxLayout()
        self.ide_radio_group = QtWidgets.QButtonGroup()
        self.ide_radio_group.idClicked.connect(self.ide_radio_clicked)
        self.ide_groupbox = QtWidgets.QGroupBox()
        self.ide_groupbox.setFixedWidth(290)
        self.ide_groupbox.setStyleSheet('QGroupBox {border: 1px solid rgb(150, 150, 150);} QRadioButton {color:white;}')
        self.ide_groupbox.setLayout(self.ide_radio_layout)
        self.radio_01 = QtWidgets.QRadioButton('VS Code')
        self.ide_radio_group.addButton(self.radio_01, 0)
        self.ide_radio_layout.addWidget(self.radio_01)
        self.ide_radio_layout.addSpacing(8)
        self.radio_01.setChecked(True)
        self.radio_02 = QtWidgets.QRadioButton('PyCharm')
        self.ide_radio_group.addButton(self.radio_02, 1)
        self.ide_radio_layout.addWidget(self.radio_02)
        self.ide_radio_layout.addSpacing(15)
        self.radio_03 = QtWidgets.QRadioButton('Wing')
        self.radio_03.setFixedWidth(55)
        self.ide_radio_group.addButton(self.radio_03, 2)
        self.ide_radio_layout.addWidget(self.radio_03)
        self.ide_panel_container.addWidget(self.ide_groupbox)
        self.left_column_container.addSpacing(10)

        # +++++++++++++++++++++++--->>>
        # CENTER COLUMN WIDGETS +---->>>
        # +++++++++++++++++++++++--->>>

        self.center_column_widget = QtWidgets.QWidget()
        self.center_column_container = QtWidgets.QVBoxLayout()
        self.center_column_container.setContentsMargins(0, 0, 0, 0)
        self.center_column_widget.setLayout(self.center_column_container)
        self.page_splitter.addWidget(self.center_column_widget)

        # Dynaconf Path Settings Panel ########################################
        self.dynaconf_settings_layout = QtWidgets.QVBoxLayout()
        self.dynaconf_settings_layout.setAlignment(QtCore.Qt.AlignTop)
        self.dynaconf_settings_panel = QtWidgets.QWidget()
        self.dynaconf_settings_panel.setLayout(self.dynaconf_settings_layout)
        self.dynaconf_settings_layout.setContentsMargins(0, 0, 0, 0)
        self.panel_frame = QtWidgets.QFrame(self.dynaconf_settings_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(85, 85, 85);')
        self.panel_frame.setGeometry(0, 0, 5000, 5000)
        self.center_column_container.addWidget(self.dynaconf_settings_panel)

        # --> Path Settings Title Bar
        self.path_settings_title_layout = QtWidgets.QHBoxLayout()
        self.path_settings_widget = self.get_title_bar_widget(self.path_settings_title_layout,
                                                              'Dynaconf Environment Values')
        self.dynaconf_settings_layout.addWidget(self.path_settings_widget)
        self.add_startup_button = QtWidgets.QPushButton('+')
        self.add_button_group.addButton(self.add_startup_button, 1)
        self.add_startup_button.setFont(self.bold_font)
        self.add_startup_button.setFixedSize(22, 22)
        self.add_startup_button.setObjectName('Primary')
        self.path_settings_title_layout.addWidget(self.add_startup_button)

        # --> Path Settings Content
        self.dynaconf_settings_container = QtWidgets.QVBoxLayout()
        self.dynaconf_settings_container.setContentsMargins(10, 3, 10, 10)
        self.dynaconf_settings_layout.addLayout(self.dynaconf_settings_container)
        self.dynaconf_settings_table = QtWidgets.QTableWidget()
        self.dynaconf_settings_table.itemChanged.connect(self.dynaconf_setting_changed)
        self.dynaconf_settings_table.setColumnCount(2)
        self.dynaconf_settings_table.setAlternatingRowColors(True)
        self.dynaconf_settings_table.horizontalHeader().setStretchLastSection(True)
        self.dynaconf_settings_table.horizontalHeader().hide()
        self.dynaconf_settings_table.verticalHeader().hide()
        self.dynaconf_settings_container.addWidget(self.dynaconf_settings_table)
        self.center_column_container.addSpacing(10)

        # ++++++++++++++++++++++--->>>
        # RIGHT COLUMN WIDGETS +---->>>
        # ++++++++++++++++++++++--->>>

        self.right_column_widget = QtWidgets.QWidget()
        self.right_column_container = QtWidgets.QVBoxLayout()
        self.right_column_container.setContentsMargins(0, 0, 0, 0)
        self.right_column_widget.setLayout(self.right_column_container)
        self.page_splitter.addWidget(self.right_column_widget)

        # System Paths Panel ##################################################
        self.system_paths_layout = QtWidgets.QVBoxLayout()
        self.system_paths_layout.setAlignment(QtCore.Qt.AlignTop)
        self.system_paths_panel = QtWidgets.QWidget()
        self.system_paths_panel.setLayout(self.system_paths_layout)
        self.system_paths_layout.setContentsMargins(0, 0, 0, 0)
        self.panel_frame = QtWidgets.QFrame(self.system_paths_panel)
        self.panel_frame.setStyleSheet('background-color: rgb(85, 85, 85);')
        self.panel_frame.setGeometry(0, 0, 5000, 5000)
        self.right_column_container.addWidget(self.system_paths_panel)

        # --> System Paths Title Bar
        self.system_paths_title_layout = QtWidgets.QHBoxLayout()
        self.system_paths_widget = self.get_title_bar_widget(self.system_paths_title_layout, 'Windows System Paths')
        self.system_paths_layout.addWidget(self.system_paths_widget)

        # --> System Paths Content
        self.system_paths_container = QtWidgets.QVBoxLayout()
        self.system_paths_container.setContentsMargins(10, 3, 10, 10)
        self.system_paths_layout.addLayout(self.system_paths_container)
        self.system_paths_table = QtWidgets.QTableWidget()
        self.system_paths_table.setColumnCount(2)
        self.system_paths_table.setAlternatingRowColors(True)
        self.system_paths_table.horizontalHeader().setStretchLastSection(True)
        self.system_paths_table.horizontalHeader().hide()
        self.system_paths_table.verticalHeader().hide()
        self.system_paths_container.addWidget(self.system_paths_table)
        self.right_column_container.addSpacing(3)

        # DCC Applications Paths #############################################
        self.dcc_paths_layout = QtWidgets.QVBoxLayout()
        self.dcc_paths_layout.setAlignment(QtCore.Qt.AlignTop)
        self.dcc_paths_panel = QtWidgets.QWidget()
        self.dcc_paths_panel.setLayout(self.dcc_paths_layout)
        self.dcc_paths_layout.setContentsMargins(0, 0, 0, 0)
        self.dcc_paths_frame = QtWidgets.QFrame(self.dcc_paths_panel)
        self.dcc_paths_frame.setStyleSheet('background-color: rgb(85, 85, 85);')
        self.dcc_paths_frame.setGeometry(0, 0, 5000, 5000)
        self.right_column_container.addWidget(self.dcc_paths_panel)

        # --> DCC Applications Title Bar
        self.dcc_application_title_layout = QtWidgets.QHBoxLayout()
        self.dcc_application_widget = self.get_title_bar_widget(self.dcc_application_title_layout,
                                                                'DCC Applications Paths')
        self.dcc_paths_layout.addWidget(self.dcc_application_widget)

        # --> DCC Applications Content
        self.dcc_applications_container = QtWidgets.QVBoxLayout()
        self.dcc_applications_container.setContentsMargins(10, 3, 10, 10)
        self.dcc_paths_layout.addLayout(self.dcc_applications_container)
        self.dcc_paths_table = QtWidgets.QTableWidget()
        self.dcc_paths_table.setColumnCount(2)
        self.dcc_paths_table.setAlternatingRowColors(True)
        self.dcc_paths_table.horizontalHeader().setStretchLastSection(True)
        self.dcc_paths_table.horizontalHeader().hide()
        self.dcc_paths_table.verticalHeader().hide()
        self.dcc_applications_container.addWidget(self.dcc_paths_table)

        self.right_column_container.addSpacing(10)
        self.content_layout.addSpacing(10)
        self.initialize_configuration_data()

    def initialize_configuration_data(self):
        self.set_logging_profile()

        # Gather Dynaconf Information
        with open(self.settings_file) as f:
            data = json.load(f)
            self.environment_list = [key for key in data.keys()]
            for key in self.environment_list:
                self.environment_key_structure[key] = {}
                for env_key, env_value in data[key].items():
                    self.environment_key_structure[key].update({env_key: env_value})

        self.dccsi_environment_listbox.add_items(self.environment_list)
        self.set_ide()
        self.get_dcc_paths()
        self.get_dynaconf_paths()
        self.get_system_paths()

    def get_dcc_paths(self):
        for application in ['maya', 'blender', 'houdini', 'substance']:
            temp_dict = {}
            executable_key = f'{application.upper()}_EXE'
            python_key = f'{application.upper()}_PY'
            for key_name in [executable_key, python_key]:
                temp_dict[key_name] = settings.get(key_name)
            self.dcc_paths[application] = temp_dict
        self.set_dcc_paths()

    def get_dynaconf_paths(self):
        self.dynaconf_environment_values = {}
        startup_env = settings.get('STARTUP_ENVIRONMENT') if len(settings.get('STARTUP_ENVIRONMENT')) else 'default'
        self.dccsi_environment_listbox.set_active_highlight(startup_env)
        active_dynaconf_environments = [startup_env] if startup_env == 'default' else ['default', startup_env]
        for environment in active_dynaconf_environments:
            for setting_key, setting_value in self.environment_key_structure[environment].items():
                self.dynaconf_environment_values[setting_key] = settings.get(setting_key)
        self.set_dynaconf_values()

    def get_ide(self):
        pass

    def get_system_paths(self):
        self.system_paths_table.clear()
        for key, value in os.environ.items():
            count = self.system_paths_table.rowCount()
            self.system_paths_table.insertRow(count)
            for column in range(2):
                target_value = key if column == 0 else value
                item = QtWidgets.QTableWidgetItem(target_value)
                if column == 0:
                    item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                self.system_paths_table.setItem(count, column, item)
        self.system_paths_table.resizeColumnsToContents()

    def get_title_bar_widget(self, target_layout, title, width=None):
        target_layout.setContentsMargins(10, 0, 5, 0)
        title_bar_widget = QtWidgets.QWidget()
        title_bar_widget.setLayout(target_layout)
        title_bar_frame = QtWidgets.QFrame(title_bar_widget)
        title_bar_frame.setStyleSheet('background-color: rgb(50, 50, 50);')
        title_bar_frame.setGeometry(0, 0, 5000, 30)
        title_bar_label = QtWidgets.QLabel(title)
        title_bar_label.setFont(self.bold_font)
        target_layout.addWidget(title_bar_label)
        if width:
            title_bar_widget.setFixedSize(width, 30)
        else:
            title_bar_widget.setFixedHeight(30)
        return title_bar_widget

    def get_separator_bar(self):
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QFrame()
        self.line.setStyleSheet('color: rgb(60, 60, 60);')
        self.line.setFrameShape(QtWidgets.QFrame.HLine)
        self.line.setLineWidth(1)
        self.separator_layout.addWidget(self.line)
        return self.separator_layout


    def get_logging_profiles(self):
        self.logging_profiles = ['default']
        # TODO - Add main specifics of logging profile

    def get_dynaconf_environment(self):
        return self.dccsi_environment_listbox.active_environment

    def get_dcc_environment(self, target):
        dcc_environment = {}
        for key, value in settings.items():
            if key in self.environment_key_structure[target].keys():
                dcc_environment[key] = value

        return dcc_environment

    def set_dcc_paths(self):
        self.dcc_paths_table.clear()
        self.dcc_paths_table.setRowCount(0)

        for key, values in self.dcc_paths.items():
            for app_key, app_path in values.items():
                count = self.dcc_paths_table.rowCount()
                self.dcc_paths_table.insertRow(count)
                for column in range(2):
                    target_value = app_key if column == 0 else app_path
                    item = QtWidgets.QTableWidgetItem(target_value)
                    if column == 0:
                        item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                    self.dcc_paths_table.setItem(count, column, item)
        self.dcc_paths_table.resizeColumnsToContents()

    def set_logging_profile(self):
        self.get_logging_profiles()
        self.logging_profile_combobox.addItems(self.logging_profiles)

    def set_dynaconf_values(self):
        self.dynaconf_settings_table.blockSignals(True)
        self.dynaconf_settings_table.clear()
        self.dynaconf_settings_table.setRowCount(0)
        for key, value in self.dynaconf_environment_values.items():
            count = self.dynaconf_settings_table.rowCount()
            self.dynaconf_settings_table.insertRow(count)
            for column in range(2):
                target_value = key if column == 0 else value
                item = QtWidgets.QTableWidgetItem(str(target_value))
                if column == 0:
                    item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEditable)
                self.dynaconf_settings_table.setItem(count, column, item)
        self.dynaconf_settings_table.resizeColumnsToContents()
        self.dynaconf_settings_table.blockSignals(False)

    def set_dynaconf_environment(self):
        target_environment = self.dccsi_environment_listbox.currentItem().text()
        dcc_application = f'{target_environment}_EXE'.upper()
        python_interpreter = dcc_application.replace('EXE', 'PY')
        _LOGGER.info(f'Launching [{target_environment}] environment...')

        # Launch DCC Application (if needed)
        if dcc_application in settings:
            self.launch_dcc_application(target_environment, settings.get(dcc_application))

        # Launch IDE
        self.launch_ide()

    def set_ide(self):
        dynaconf_ide = settings.get('DCCSI_GDEBUGGER')
        selection_index = 0 if not dynaconf_ide else self.supported_ides.index(dynaconf_ide.lower())
        for index, target_button in enumerate(self.ide_radio_group.buttons()):
            if index == selection_index:
                target_button.setChecked(True)
        _LOGGER.info(self.ide_radio_group.checkedButton().text())

    def open_section(self):
        pass

    def close_section(self):
        pass

    def launch_dcc_application(self, environment, application_path):
        if Path(application_path).name not in (i.name() for i in psutil.process_iter()):
            _LOGGER.info(f'[{Path(application_path).name}] is not an active process... launching')
            envars = self.get_dcc_environment(environment)
            app = QtProcess(application_path, 'dummy.py', **envars)
            app.start_process()
        else:
            _LOGGER.info(f'Application is open [{environment}].... skipping')

    def launch_ide(self):
        selected = [x for x in self.supported_ides if self.ide_radio_group.checkedButton().text().lower().startswith(x)]
        application_path = settings.get(f'{selected[0].upper()}_EXE')
        if Path(application_path).name not in (i.name() for i in psutil.process_iter()):
            _LOGGER.info(f'[{Path(application_path).name}] is not an active process... launching')
            envars = self.get_ide_profile()
            # app = QtProcess(application_path, 'dummy.py', **envars)
            # app.start_process()
        _LOGGER.info(f'SelectedIDE: {selected}')

    # ++++++++++++++++--->>>
    # BUTTON ACTIONS +---->>>
    # ++++++++++++++++--->>>

    def add_button_clicked(self, id_):
        _LOGGER.info(f'Button clicked: {id_}')

    def ide_radio_clicked(self, id_):
        _LOGGER.info(f'Radio clicked: {id_}')

    def environment_clicked(self):
        item = self.dccsi_environment_listbox.currentItem()
        target_environment = item.text()
        self.dccsi_environment_listbox.set_active_highlight(target_environment)
        settings.set('STARTUP_ENVIRONMENT', target_environment)
        config.initialize_settings(target_environment)
        self.get_dynaconf_paths()

    def dynaconf_setting_changed(self, item):
        target_environment = self.get_dynaconf_environment()
        envar_name = self.dynaconf_settings_table.item(item.row(), 0).text()
        settings.from_env(target_environment)[envar_name] = item.text()

    def set_startup_environment_clicked(self):
        self.set_dynaconf_environment()

    def create_new_environment_clicked(self):
        _LOGGER.info('Create New Environment Clicked')


class EnvironmentListWidget(QtWidgets.QListWidget):
    def __init__(self, model, parent=None):
        super(EnvironmentListWidget, self).__init__(parent)

        self.model = model
        self.environment_list = None
        self.parent = parent
        self.active_environment = 'default'
        self.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self.active_icon = QtGui.QIcon(f'{self.model.launcher_base_path}/images/o3de_icon.png')
        self.inactive_icon = QtGui.QIcon(f'{self.model.launcher_base_path}/images/o3de_icon_disabled.png')

    def set_active_highlight(self, target):
        for index, environment in enumerate(self.environment_list):
            if environment == target:
                self.item(index).setIcon(self.active_icon)
                self.item(index).setBackground(QtGui.QColor('#4b8def'))
                self.active_environment = environment
            else:
                self.item(index).setIcon(self.inactive_icon)
                if index % 2 == 0:
                    self.item(index).setBackground(QtGui.QColor('#555555'))
                else:
                    self.item(index).setBackground(QtGui.QColor('#696969'))

    def add_items(self, item_list):
        self.environment_list = item_list
        for index, env in enumerate(self.environment_list):
            item = QtWidgets.QListWidgetItem(env)
            item.setIcon(self.inactive_icon)
            item.setSizeHint(QtCore.QSize(250, 25))
            self.addItem(item)
        self.setCurrentIndex(QtCore.QModelIndex())

    def clear_items(self):
        self.clear()

