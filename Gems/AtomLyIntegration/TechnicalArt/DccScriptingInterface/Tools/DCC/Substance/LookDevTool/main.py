import config
from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtWidgets import QApplication, QMessageBox
from PySide2.QtCore import Signal, Slot, QThread, QProcess, QProcessEnvironment
from pathlib import Path
import constants
import logging
import json
import sys
import os
import substance_materials
from pysbs import sbsenum
from azpy.shared.qt_process import QtProcess


_LOGGER = logging.getLogger('Tools.DCC.Substance.LookDevTool.main')
_LOGGER.info('LookDevTool Launch')


# ********************************
# *** Create PySide2 interface ***
# ********************************
#
# -> Set Substance Automation Toolkit Location (if not found, but hardcode it for tool)
# -> File Browser
# -> Baking options (Bake all possible maps)
# ---- AO
# ---- Normal
# ---- Direction
# ---- Curvature
# ---- Position
#
# ->
#
# -> Export Atom Material checkbox
# -> Process File button
#
# **********************
# *** DCC conversion ***
# **********************

"""
# ***  (Start with Maya and expand if there's time)
# -> Determine file type, connect to DCC api
# -> Parse file objects and materials
# -> Output list of scene objects that contains mesh name, material name(s), and connected textures
# --- Put this information into a scrollable table with individual components for each found object
# --- Include a button for each object found that will process texture baking and sbs creation
# --- Include a button that activates after bake and launches
# ++++++ (Radio Buttons- After process complete:) -Launch Substance  -Do Nothing

"""

# TODO - Swap Mesh with FBX in SBS File
# TODO - Export Substance Material as StandardPBR Atom Material


class ConvertSBS(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(ConvertSBS, self).__init__(parent)

        self.app = QtWidgets.QApplication.instance()
        self.setWindowTitle("SBS Asset Generator")
        self.setObjectName('sbsConversion')
        self.setGeometry(50, 50, 410, 545)
        self.setFixedSize(410, 545)
        self.isTopLevel()

        self.conversion_format = 'File'
        self.separator_layout = ''
        self.line = ''
        self.p = None
        self.bold_font = QtGui.QFont("Plastique", 8, QtGui.QFont.Bold)
        self.export_document_filter = 'Designer/Image Files (*.sbs *.png *.jpg *.tif *.tga)'
        self.create_document_filter = 'Maya Files (*.ma *.mb)'
        self.processed_material_information = {}

        self.main_container = QtWidgets.QVBoxLayout(self)
        self.window_tabs = QtWidgets.QTabWidget()
        self.main_container.addWidget(self.window_tabs)

        # Testing file/directory locations:
        self.path_fields = []
        self.mayapy_location = settings.get('MAYA_PY')
        self.output_location = None
        self.script_path = Path(__file__).parent / 'maya_materials.py'
        self.current_paths = {}

        # -- Button Groups --
        self.browse_button_group = QtWidgets.QButtonGroup()
        self.dcc_radio_group = QtWidgets.QButtonGroup()
        self.existing_radio_group = QtWidgets.QButtonGroup()

        # ++++++++++++++++++++++-->>
        # DCC Tab ++++++++++++++--->>
        # ++++++++++++++++++++++-->>

        self.create_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.create_tab_content, 'DCC')
        self.dcc_tab_layout = QtWidgets.QVBoxLayout()
        self.dcc_tab_layout.setAlignment(QtCore.Qt.AlignTop)
        self.create_tab_content.setLayout(self.dcc_tab_layout)

        # Input Directory
        self.dcc_tab_layout.addSpacing(5)
        self.dcc_textfield = QtWidgets.QTextEdit()
        self.dcc_tab_layout.addWidget(self.dcc_textfield)
        self.dcc_textfield.setReadOnly(True)
        self.dcc_textfield.setStyleSheet('border: none;')
        self.dcc_textfield.setFixedHeight(90)
        self.dcc_textfield.setText('This tab of the tool helps with the creation of a O3DE Substance library asset '
                                   'from existing Maya (.ma,.mb) files. Browse to file or folder and click "Process DCC'
                                   ' Files" button below to convert all objects contained in files to SBS files with '
                                   'required asset library formatting (ready for upload).')

        block_formatting = QtGui.QTextBlockFormat()
        block_formatting.setLineHeight(125, QtGui.QTextBlockFormat.ProportionalHeight)
        dcc_cursor = self.dcc_textfield.textCursor()
        dcc_cursor.clearSelection()
        dcc_cursor.select(QtGui.QTextCursor.Document)
        dcc_cursor.mergeBlockFormat(block_formatting)

        self.create_input_directory_label = QtWidgets.QLabel('Input File')
        self.create_input_directory_label.setFont(self.bold_font)
        self.dcc_tab_layout.addWidget(self.create_input_directory_label)
        self.create_input_directory_layout = QtWidgets.QHBoxLayout()
        self.dcc_tab_layout.addLayout(self.create_input_directory_layout)

        self.create_input_directory_field = QtWidgets.QLineEdit()
        self.create_input_directory_field.setObjectName('dcc_input_field')
        self.create_input_directory_field.setFixedHeight(25)
        self.create_input_directory_field.setStyleSheet('padding-left: 5px;')
        self.path_fields.append(self.create_input_directory_field)
        self.create_input_directory_layout.addWidget(self.create_input_directory_field)
        self.create_input_directory_browse = QtWidgets.QPushButton('Set')
        self.browse_button_group.addButton(self.create_input_directory_browse, 0)
        self.browse_button_group.idClicked.connect(self.browse_button_clicked)
        self.create_input_directory_browse.setFixedSize(30, 25)
        self.create_input_directory_layout.addWidget(self.create_input_directory_browse)

        self.create_input_radio_buttons = QtWidgets.QHBoxLayout()
        self.create_input_radio_buttons.setAlignment(QtCore.Qt.AlignLeft)
        self.dcc_tab_layout.addLayout(self.create_input_radio_buttons)
        self.create_file_radio_btn = QtWidgets.QRadioButton('File')
        self.create_file_radio_btn.clicked.connect(self.radio_button_format_conversion)
        self.create_file_radio_btn.setChecked(True)
        self.dcc_radio_group.addButton(self.create_file_radio_btn, 0)
        self.create_input_radio_buttons.addWidget(self.create_file_radio_btn)
        self.create_input_radio_buttons.addSpacing(8)
        self.create_directory_radio_btn = QtWidgets.QRadioButton('Directory')
        self.create_directory_radio_btn.clicked.connect(self.radio_button_format_conversion)
        self.dcc_radio_group.addButton(self.create_directory_radio_btn, 1)
        self.create_input_radio_buttons.addWidget(self.create_directory_radio_btn)

        # Separator Bar
        self.add_separator_bar(self.dcc_tab_layout)

        # Options
        self.options_layout = QtWidgets.QVBoxLayout()
        self.dcc_tab_layout.addLayout(self.options_layout)
        self.target_materials_label = QtWidgets.QLabel('Application/Material Types')
        self.target_materials_label.setFont(self.bold_font)
        self.options_layout.addWidget(self.target_materials_label)
        self.application_combobox = QtWidgets.QComboBox()
        self.combobox_items = ['Maya']
        self.application_combobox.addItems(self.combobox_items)
        self.options_layout.addWidget(self.application_combobox)
        self.options_layout.addSpacing(4)
        self.arnold_material_checkbox = QtWidgets.QCheckBox('Arnold')
        self.arnold_material_checkbox.setChecked(True)
        self.options_layout.addWidget(self.arnold_material_checkbox)
        self.stingray_material_checkbox = QtWidgets.QCheckBox('StingrayPBS')
        self.stingray_material_checkbox.setChecked(True)
        self.options_layout.addWidget(self.stingray_material_checkbox)
        self.dcc_tab_layout.addStretch()

        # Create Substance Files
        self.create_substance_files_button = QtWidgets.QPushButton('Process DCC Files')
        self.create_substance_files_button.clicked.connect(self.create_substance_files)
        self.create_substance_files_button.setFixedHeight(40)
        self.dcc_tab_layout.addWidget(self.create_substance_files_button)

        # +++++++++++++++++++++++++++-->>
        # Existing Tab ++++++++++++++--->>
        # +++++++++++++++++++++++++++-->>

        self.existing_tab_content = QtWidgets.QWidget()
        self.window_tabs.addTab(self.existing_tab_content, 'Existing')
        self.existing_tab_layout = QtWidgets.QVBoxLayout()
        self.existing_tab_content.setLayout(self.existing_tab_layout)

        # Input Directory
        self.existing_tab_layout.addSpacing(5)
        self.input_textfield = QtWidgets.QTextEdit()
        self.input_textfield.setObjectName('existing_input_field')
        self.input_textfield.setReadOnly(True)
        self.input_textfield.setStyleSheet('border: none;')
        self.input_textfield.setFixedHeight(80)
        self.input_textfield.setText('This tab of the tool helps with the creation of a O3DE Substance Library '
                                     'Asset from existing SBS files and/or texture sets. By selecting the "file" radio '
                                     'button, first choose SBS file (if present) or a single texture from a texture set '
                                     'to generate a new sbs file and asset directory for upload.')
        block_formatting = QtGui.QTextBlockFormat()
        block_formatting.setLineHeight(125, QtGui.QTextBlockFormat.ProportionalHeight)
        cursor = self.input_textfield.textCursor()
        cursor.clearSelection()
        cursor.select(QtGui.QTextCursor.Document)
        cursor.mergeBlockFormat(block_formatting)
        self.existing_tab_layout.addWidget(self.input_textfield)
        self.existing_tab_layout.addSpacing(10)

        self.input_directory_label = QtWidgets.QLabel('Input File')
        self.input_directory_label.setFont(self.bold_font)
        self.existing_tab_layout.addWidget(self.input_directory_label)
        self.input_directory_layout = QtWidgets.QHBoxLayout()
        self.existing_tab_layout.addLayout(self.input_directory_layout)
        self.input_directory_field = QtWidgets.QLineEdit()
        self.input_directory_field.setObjectName('existing_input_field')
        self.input_directory_field.setFixedHeight(25)
        self.input_directory_field.setStyleSheet('padding-left: 5px;')
        self.path_fields.append(self.input_directory_field)
        self.input_directory_layout.addWidget(self.input_directory_field)
        self.input_directory_browse = QtWidgets.QPushButton('Set')
        self.browse_button_group.addButton(self.input_directory_browse, 1)
        self.input_directory_browse.setFixedSize(30, 25)
        self.input_directory_layout.addWidget(self.input_directory_browse)
        self.input_radio_buttons = QtWidgets.QHBoxLayout()
        self.input_radio_buttons.setAlignment(QtCore.Qt.AlignLeft)
        self.existing_tab_layout.addLayout(self.input_radio_buttons)
        self.file_radio_btn = QtWidgets.QRadioButton('File')
        self.file_radio_btn.clicked.connect(self.radio_button_format_conversion)
        self.file_radio_btn.setChecked(True)
        self.existing_radio_group.addButton(self.file_radio_btn, 0)
        self.input_radio_buttons.addWidget(self.file_radio_btn)
        self.input_radio_buttons.addSpacing(8)
        self.directory_radio_btn = QtWidgets.QRadioButton('Directory')
        self.directory_radio_btn.clicked.connect(self.radio_button_format_conversion)
        self.existing_radio_group.addButton(self.directory_radio_btn, 1)
        self.input_radio_buttons.addWidget(self.directory_radio_btn)
        self.existing_tab_layout.addStretch()

        # Convert buttons
        self.execute_button = QtWidgets.QPushButton('Prepare SBS Files')
        self.execute_button.setFixedHeight(40)
        self.execute_button.clicked.connect(self.execute_clicked)
        self.existing_tab_layout.addWidget(self.execute_button)

        # +++++++++++++++++++++++++-->>
        # Settings Tab ++++++++++++--->>
        # +++++++++++++++++++++++++-->>

        self.settings_tab_layout = QtWidgets.QWidget()
        self.window_tabs.addTab(self.settings_tab_layout, 'Settings')
        self.output_tab_layout = QtWidgets.QVBoxLayout()
        self.output_tab_layout.addSpacing(5)
        self.output_tab_layout.setAlignment(QtCore.Qt.AlignTop)
        self.settings_tab_layout.setLayout(self.output_tab_layout)

        # Texture Set
        self.texture_set_label = QtWidgets.QLabel('Output Textures')
        self.texture_set_label.setFont(self.bold_font)
        self.output_tab_layout.addWidget(self.texture_set_label)
        self.output_tab_layout.addSpacing(4)

        self.textures_container = QtWidgets.QHBoxLayout()
        self.output_tab_layout.addLayout(self.textures_container)

        # Left Column - Selected Textures
        self.left_textures_container = QtWidgets.QVBoxLayout()
        self.textures_container.addLayout(self.left_textures_container)
        self.base_color_checkbox = QtWidgets.QCheckBox('Base Color')
        self.base_color_checkbox.setChecked(True)
        self.left_textures_container.addWidget(self.base_color_checkbox)
        self.metallic_checkbox = QtWidgets.QCheckBox('Metallic')
        self.metallic_checkbox.setChecked(True)
        self.left_textures_container.addWidget(self.metallic_checkbox)
        self.roughness_checkbox = QtWidgets.QCheckBox('Roughness')
        self.roughness_checkbox.setChecked(True)
        self.left_textures_container.addWidget(self.roughness_checkbox)
        self.normal_checkbox = QtWidgets.QCheckBox('Normal')
        self.normal_checkbox.setChecked(True)
        self.left_textures_container.addWidget(self.normal_checkbox)

        # Right Column - Selected Textures
        self.right_textures_container = QtWidgets.QVBoxLayout()
        self.textures_container.addLayout(self.right_textures_container)
        self.emissive_checkbox = QtWidgets.QCheckBox('Emissive')
        self.emissive_checkbox.setChecked(True)
        self.right_textures_container.addWidget(self.emissive_checkbox)
        self.ao_checkbox = QtWidgets.QCheckBox('Ambient Occlusion')
        self.ao_checkbox.setChecked(True)
        self.right_textures_container.addWidget(self.ao_checkbox)
        self.parallax_checkbox = QtWidgets.QCheckBox('Parallax')
        self.parallax_checkbox.setChecked(True)
        self.right_textures_container.addWidget(self.parallax_checkbox)
        self.sss_checkbox = QtWidgets.QCheckBox('Subsurface Scattering')
        self.sss_checkbox.setChecked(True)
        self.right_textures_container.addWidget(self.sss_checkbox)

        # Separator Bar
        self.add_separator_bar(self.output_tab_layout)

        # File Output
        self.output_layout = QtWidgets.QHBoxLayout()
        self.output_layout.setAlignment(QtCore.Qt.AlignTop)
        self.output_tab_layout.addLayout(self.output_layout)

        # File Output
        self.file_output_layout = QtWidgets.QVBoxLayout()
        self.output_layout.addLayout(self.file_output_layout)
        self.file_output_label = QtWidgets.QLabel('File Output')
        self.file_output_label.setFont(self.bold_font)
        self.file_output_layout.addWidget(self.file_output_label)
        self.file_output_layout.addSpacing(4)
        self.textures_checkbox = QtWidgets.QCheckBox('Textures')
        self.textures_checkbox.setChecked(True)
        self.file_output_layout.addWidget(self.textures_checkbox)
        self.sbs_file_checkbox = QtWidgets.QCheckBox('.sbs')
        self.sbs_file_checkbox.setChecked(True)
        self.file_output_layout.addWidget(self.sbs_file_checkbox)

        # Output Settings
        self.output_settings_layout = QtWidgets.QVBoxLayout()
        self.output_settings_layout.setAlignment(QtCore.Qt.AlignTop)
        self.output_layout.addLayout(self.output_settings_layout)
        self.output_size_label = QtWidgets.QLabel('Output Dimensions')
        self.output_size_label.setFont(self.bold_font)
        self.output_settings_layout.addWidget(self.output_size_label)
        self.output_size_combobox = QtWidgets.QComboBox()
        self.output_size_combobox.addItems(['512', '1024', '2048', '4096', '8192'])
        self.output_size_combobox.setCurrentIndex(3)
        self.output_settings_layout.addWidget(self.output_size_combobox)

        # Separator Bar
        self.add_separator_bar(self.output_tab_layout)

        # O3DE Directory
        self.o3de_directory_label = QtWidgets.QLabel('O3DE Directory')
        self.o3de_directory_label.setFont(self.bold_font)
        self.output_tab_layout.addWidget(self.o3de_directory_label)
        self.o3de_directory_layout = QtWidgets.QHBoxLayout()
        self.o3de_directory_field = QtWidgets.QLineEdit()
        self.o3de_directory_field.setObjectName('o3de_directory_field')
        self.path_fields.append(self.o3de_directory_field)
        self.o3de_directory_field.setFixedHeight(25)
        self.o3de_directory_field.setStyleSheet('padding-left: 5px;')
        self.o3de_directory_layout.addWidget(self.o3de_directory_field)
        self.o3de_directory_browse = QtWidgets.QPushButton('Set')
        self.browse_button_group.addButton(self.o3de_directory_browse, 4)
        self.o3de_directory_browse.setFixedSize(30, 25)
        self.o3de_directory_layout.addWidget(self.o3de_directory_browse)
        self.output_tab_layout.addLayout(self.o3de_directory_layout)
        self.output_tab_layout.addSpacing(5)

        # Output Directory
        self.output_directory_label = QtWidgets.QLabel('Output Directory')
        self.output_directory_label.setFont(self.bold_font)
        self.output_tab_layout.addWidget(self.output_directory_label)
        self.output_directory_layout = QtWidgets.QHBoxLayout()
        self.output_tab_layout.addLayout(self.output_directory_layout)
        self.output_directory_field = QtWidgets.QLineEdit()
        self.output_directory_field.setObjectName('output_directory')
        self.path_fields.append(self.output_directory_field)
        self.output_directory_field.setFixedHeight(25)
        self.output_directory_field.setStyleSheet('padding-left: 5px;')
        self.output_directory_layout.addWidget(self.output_directory_field)
        self.output_directory_browse = QtWidgets.QPushButton('Set')
        self.browse_button_group.addButton(self.output_directory_browse, 5)
        self.output_directory_browse.setFixedSize(30, 25)
        self.output_directory_layout.addWidget(self.output_directory_browse)
        self.get_tool_settings()

    def add_separator_bar(self, target_layout):
        self.separator_layout = QtWidgets.QHBoxLayout()
        self.line = QtWidgets.QLabel()
        self.line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        self.line.setLineWidth(1)
        self.line.setFixedHeight(10)
        self.separator_layout.addWidget(self.line)
        target_layout.addLayout(self.separator_layout)

    def radio_button_format_conversion(self):
        label, field = (self.create_input_directory_label, self.create_input_directory_field) if \
            self.window_tabs.currentIndex() == 0 else (self.input_directory_label, self.input_directory_field)

        if label.text() == 'Input File':
            label.setText('Input Directory')
            self.conversion_format = 'Directory'
            field_text = self.convert_list_to_string(1) if self.window_tabs.currentIndex() == 0 else \
                self.convert_list_to_string(3)
        else:
            label.setText('Input File')
            self.conversion_format = 'File'
            field_text = self.convert_list_to_string(0) if self.window_tabs.currentIndex() == 0 else \
                self.convert_list_to_string(2)
        field.setText(field_text)

    def browse_for_path(self, browse_filter):
        if 'directory' in browse_filter:
            dialog = QtWidgets.QFileDialog(self, 'Shift-Select Target Directories', 'E://')
            dialog.setFileMode(QtWidgets.QFileDialog.DirectoryOnly)
            dialog.setOption(QtWidgets.QFileDialog.DontUseNativeDialog, True)
            file_view = dialog.findChild(QtWidgets.QListView, 'listView')

            # Workaround for selecting multiple directories with File Dialog
            if file_view:
                file_view.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)
            f_tree_view = dialog.findChild(QtWidgets.QTreeView)
            if f_tree_view:
                f_tree_view.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)

            if dialog.exec_() == QtWidgets.QDialog.Accepted:
                return dialog.selectedFiles()
        else:
            file_list, _ = QtWidgets.QFileDialog.getOpenFileNames(self, 'Shift-Select Target Files', 'E://',
                                                                        filter=str(browse_filter))
            if file_list:
                return file_list

    def validate_path(self, entered_string):
        if os.path.isfile(entered_string):
            return entered_string
        return None

    def convert_list_to_string(self, id_):
        target_id = self.get_field_by_id(id_)
        try:
            if isinstance(self.current_paths[target_id], list):
                path_string = ''
                for path in self.current_paths[id_]:
                    path_string += '{}; '.format(path)
                return path_string[:-2]
        except KeyError:
            return self.current_paths[target_id]

    def enable_window_tabs(self, activate=True):
        if not activate:
            self.window_tabs.setCurrentIndex(2)
        self.window_tabs.setTabEnabled(0, activate)
        self.window_tabs.setTabEnabled(1, activate)

    def check_required_fields(self):
        validated = True
        required_fields = [self.o3de_directory_field, self.output_directory_field]
        for target_field in required_fields:
            if not os.path.isdir(target_field.text()):
                target_field.setStyleSheet('border: 1px solid red')
                validated = False
            else:
                target_field.setStyleSheet('')
        _LOGGER.info('ValidationStatus: {}'.format(validated))
        return validated

    def create_sbs_files(self, processed_material_information):
        output_setting = self.get_sbs_output_setting()
        substance_materials.create_designer_files(output_setting, processed_material_information)

    def get_browse_filter(self, current_tab, button_id):
        if current_tab == 2 or button_id % 2 != 0:
            browse_filter = 'directory'
        else:
            if current_tab == 0:
                browse_filter = self.create_document_filter if self.dcc_radio_group.checkedId() == 0 else 'directory'
            else:
                browse_filter = self.export_document_filter if self.existing_radio_group.checkedId() == 0 else 'directory'
        return browse_filter

    def get_files_from_directory(self, target_directory, target_extensions):
        directory_audit = []
        for (root, dirs, files) in os.walk(target_directory, topdown=True):
            for file in files:
                if os.path.splitext(file)[1] in target_extensions:
                    directory_audit.append(os.path.join(root, file))
        return directory_audit

    def get_conversion_files(self):
        path_index = self.get_path_index()
        target_extensions = ['.ma', '.mb'] if path_index < 2 else ['.sbs']
        target_key = self.get_field_by_id(path_index)
        if self.current_paths[target_key]:
            if os.path.isdir(self.current_paths[target_key]):
                return self.get_files_from_directory(self.current_paths[target_key], target_extensions)
            else:
                conversion_files = [x.strip() for x in self.current_paths[target_key].split(';')]
                return [x for x in conversion_files if self.validate_path(x)]
        return None

    def get_path_index(self):
        if self.window_tabs.currentIndex() == 0:
            path_index = self.dcc_radio_group.checkedId()
        else:
            path_index = self.existing_radio_group.checkedId() + len(self.dcc_radio_group.buttons())
        return path_index

    def get_process_environment(self):
        env = [env for env in QtCore.QProcess.systemEnvironment() if not env.startswith('PYTHONPATH=')]
        env.append(f'MAYA_LOCATION={Path(self.mayapy_location).parent.as_posix()}')
        env.append(f'PYTHONPATH={Path(self.mayapy_location).parent.as_posix()}') # Double check these are used
        return env

    def get_sbs_output_setting(self):
        output_setting = self.output_size_combobox.currentText()
        if output_setting == '512':
            return sbsenum.OutputSizeEnum.SIZE_512
        elif output_setting == '1024':
            return sbsenum.OutputSizeEnum.SIZE_1024
        elif output_setting == '2048':
            return sbsenum.OutputSizeEnum.SIZE_2048
        elif output_setting == '4096':
            return sbsenum.OutputSizeEnum.SIZE_4096
        elif output_setting == '8192':
            return sbsenum.OutputSizeEnum.SIZE_8192
        return None

    def get_clean_path(self, path_string):
        return path_string.replace('\\', '/')

    def get_texture_set(self, texture_file):
        base_directory = os.path.dirname(texture_file)
        file_name = os.path.basename(texture_file)
        file_stem = '_'.join(file_name.split('_')[:-1])
        base_file_name = '_'.join(file_name.split('_')[:-1])
        texture_set = {}
        for file in os.listdir(base_directory):
            if file.startswith(file_stem):
                file_name = file.split('.')[0]
                file_type = file_name.split('_')[-1]
                for key, values in constants.PBR_TEXTURE_KEYS.items():
                    if file_type.lower() in values:
                        texture_set[file_type] = os.path.join(base_directory, file)
        return {base_file_name: texture_set}

    def get_field_by_id(self, id_):
        for key, values in constants.default_settings.items():
            if values['id'] == id_:
                return key

    def get_tool_settings(self):
        for key, value in constants.default_settings.items():
            self.current_paths[key] = value['path']
            for target_field in self.path_fields:
                target_object = target_field.objectName()
                if target_object == key or str('_'.join(target_object.split('_')[:-1]) + '_file') == key:
                    target_field.setText(value['path'])

        if not self.check_required_fields():
            self.enable_window_tabs(False)

    def get_sbs_assets(self, asset_type, asset_directory, asset_name):
        _LOGGER.info('AssetType: {}  AssetDirectory: {}  AssetName: {}'.format(asset_type, asset_directory, asset_name))
        if asset_type == 'substance':
            for target_asset in asset_name:
                target_path = os.path.join(asset_directory, target_asset)
                _LOGGER.info('TargetPath: {}'.format(target_path))
                substance_materials.get_sbs_values(self.get_clean_path(target_path))

    def get_selected_radio_button(self):
        pass

    # +++++++++++++++++++++++++++-->
    # DCC Standalone Connection +--->>
    # +++++++++++++++++++++++++++-->

    def start_maya_session(self, target_files):
        """
        This starts the exchange between the standalone QT UI and Maya Standalone to process FBX files.
        The information sent to Maya is the FBX file for processing, as well as the base directory and
        relative destination paths. There is a pipe that can be used to shuttle processed information
        back to the script, but at this stage it is not being used.
        :param target_fbx:
        :return:
        """
        if target_files and self.mayapy_location:
            try:
                envars = self.get_process_environment()
                envars.append(f'SCRIPT={self.script_path.as_posix()}')
                envars.append(f'OUTPUT_DATA={self.output_location}')
                maya = QtProcess(self.mayapy_location, target_files, envars)
                maya.process_info.connect(self.get_process_info)
                maya.start_process(detached=False)
            except Exception as e:
                _LOGGER.warning('Error creating Maya Files: {}'.format(e))
                return None
        else:
            msg = QMessageBox()
            msg.setIcon(QMessageBox.Critical)
            msg.setText("You must have Maya installed to use this script!")
            msg.setWindowTitle("Maya Not Found")
            msg.setStandardButtons(QMessageBox.Ok)

    # +++++++++++++++++++++++++-->
    # Button Actions ++++++++++--->
    # +++++++++++++++++++++++++-->

    def browse_button_clicked(self, id_):
        browse_filter = self.get_browse_filter(self.window_tabs.currentIndex(), id_)
        target_path = self.browse_for_path(browse_filter)
        if target_path:
            self.current_paths[id_] = target_path[1] if browse_filter != 'directory' else target_path[0]
            self.path_fields[id_].setText(self.current_paths[id_])

            if self.window_tabs.currentIndex() == 2:
                if self.check_required_fields():
                    self.enable_window_tabs(True)

    def create_substance_files(self):
        self.output_location = self.output_directory_field.text()
        self.start_maya_session(self.get_conversion_files())

    def execute_clicked(self):
        target_id = self.existing_radio_group.checkedId()
        field_text = self.input_directory_field.text()
        asset_list = field_text.split(';')
        asset_directory = os.path.dirname(asset_list[0]) if os.path.isfile(asset_list[0]) else asset_list[0]
        asset_name = []

        if target_id == 0:
            file_extension = asset_list[0].split('.')[-1]
            asset_type = 'substance' if file_extension == 'sbs' else 'image'
            for asset in asset_list:
                asset_name.append(os.path.basename(asset))
        else:
            asset_type = 'directory'
            asset_name = [x for x in asset_list] if len(asset_list) > 1 else ''

        self.get_sbs_assets(asset_type, asset_directory, asset_name)

    # +++++++++++++++++++++++++-->
    # Signals/Slots +++++++++++--->
    # +++++++++++++++++++++++++-->

    @Slot(dict)
    def get_process_info(self, process_info):
        _LOGGER.info(f'Returned Process Information: {process_info}')
        self.create_sbs_files(process_info)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    conversion = ConvertSBS()
    conversion.show()
    sys.exit(app.exec_())
