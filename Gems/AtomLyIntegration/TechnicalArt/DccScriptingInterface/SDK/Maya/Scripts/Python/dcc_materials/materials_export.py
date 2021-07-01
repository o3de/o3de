# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

"""
Usage
=====
Put usage instructions here.

Output
======
Put output information here.

Notes:
In order to run this, you'll need to verify that the "mayapy_path" class attribute corresponds to the location on
your machine. Currently I've just included mapping instructions for Maya StingrayPBS materials, although most of
the needed elements are in place to carry out additional materials inside of Maya pretty quickly moving forward.
I've marked areas that still need refinement (or to be added altogether) with TODO comments

TODO- Add command line access
TODO- Docstrings need work... wanted to get descriptions in but they need to be set for Sphinx
TODO- Add Blender and 3ds Max interoperablity
Links:
https://blender.stackexchange.com/questions/100497/use-blenders-bpy-in-projects-outside-blender
https://knowledge.autodesk.com/support/3ds-max/learn-explore/caas/CloudHelp/cloudhelp/2019/ENU/3DSMax-Batch/files/GUID-0968FF0A-5ADD-454D-B8F6-1983E76A4AF9-htm.html

TODO- Look at dynaconf and wire in a solid means for configuration settings
TODO- This hasn't been "designed"- might be worth it to consider the visual design to ensure the most effective and
    attractive UI

Reading FBX file information (might come in handy later)
-- Materials information can be extracted from ASCII fbx pretty easily, binary is possible but more difficult
-- FBX files could be exported as ASCII files and I could use regex there to extract material information 
-- I couldn't get pyfbx_i42 to work, but purportedly it can extract information from binary files. You may just have
to use the specified python versions

-- Jonny wants me to use pathlib wherever possible for OO pathing, as well as python-box (aka Box) for dict access

# Things to do:
    --> Create the cube demo for Jonny with 4 attached materials
    --> Allow export of JSON file for demo
    --> Need to figure out how to clear pointers for stored data properly
    --> Allow for command line control

"""

from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtWidgets import QApplication
from model import MaterialsModel, TreeNode
import subprocess
import json
import sys
import os
import re


class MaterialsToLumberyard(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(MaterialsToLumberyard, self).__init__(parent)

        self.app = QtWidgets.QApplication.instance()
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 800, 520)
        self.setObjectName('MaterialsToLumberyard')
        self.setWindowTitle('Materials To Lumberyard')
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowMinMaxButtonsHint)
        self.isTopLevel()

        self.desktop_location = os.path.join(os.path.expanduser('~'), 'Desktop')
        self.lumberyard_materials_directory = os.path.join(self.desktop_location, 'LumberyardMaterials')
        self.mayapy_path = os.path.abspath('C:/"Program Files"/Autodesk/Maya2020/bin/mayapy.exe')
        self.bold_font_large = QtGui.QFont('Helvetica', 7, QtGui.QFont.Bold)
        self.medium_font = QtGui.QFont('Helvetica', 7, QtGui.QFont.Normal)

        self.materials_dictionary = {}
        self.material_definitions = {}
        self.material_definitions_nodes = []
        self.processed_materials = []
        self.target_file_list = []
        self.current_scene = None
        self.model = None
        self.total_materials = 0

        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setContentsMargins(0, 0, 0, 0)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.setLayout(self.main_container)
        self.content_layout = QtWidgets.QVBoxLayout()
        self.content_layout.setAlignment(QtCore.Qt.AlignTop)
        self.content_layout.setContentsMargins(10, 3, 10, 5)
        self.main_container.addLayout(self.content_layout)

        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        # ---->> Header Bar
        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        self.header_bar_layout = QtWidgets.QHBoxLayout()
        self.select_files_layout = QtWidgets.QHBoxLayout()
        self.select_files_layout.setAlignment(QtCore.Qt.AlignLeft)
        self.select_files_button = QtWidgets.QPushButton('Select Files')
        self.select_files_button.clicked.connect(self.select_files_button_clicked)
        self.select_files_button.setFixedSize(90, 35)
        self.select_files_layout.addWidget(self.select_files_button)
        self.header_bar_layout.addLayout(self.select_files_layout)
        self.switch_combobox_layout = QtWidgets.QHBoxLayout()
        self.switch_combobox_layout.setAlignment(QtCore.Qt.AlignRight)
        self.switch_layout_combobox = QtWidgets.QComboBox()
        self.switch_layout_combobox.setEnabled(False)
        self.switch_layout_combobox.setFixedSize(250, 30)
        self.combobox_items = ['Source Files', 'DCC Material Values', 'Export Materials']
        self.switch_layout_combobox.setStyleSheet('QComboBox {padding-left:6px;}')
        self.switch_layout_combobox.addItems(self.combobox_items)
        self.header_bar_layout.addWidget(self.switch_layout_combobox)
        self.header_bar_layout.addLayout(self.switch_combobox_layout)
        self.content_layout.addSpacing(5)
        self.content_layout.addLayout(self.header_bar_layout)

        # ++++++++++++++++++++++++++++++++++++++++++++++++#
        # File Source Table / Attributes (Stacked Layout) #
        # ++++++++++++++++++++++++++++++++++++++++++++++++#

        self.content_stacked_layout = QtWidgets.QStackedLayout()
        self.content_layout.addLayout(self.content_stacked_layout)
        self.switch_layout_combobox.currentIndexChanged.connect(self.layout_combobox_changed)

        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        # ---->> Files Table
        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        self.target_files_table = QtWidgets.QTableWidget()
        self.target_files_table.setFocusPolicy(QtCore.Qt.NoFocus)
        self.target_files_table.setColumnCount(2)
        self.target_files_table.setAlternatingRowColors(True)
        self.target_files_table.setHorizontalHeaderLabels(['File List', ''])
        self.target_files_table.horizontalHeader().setStyleSheet('QHeaderView::section {background-color: rgb(220, 220, 220); padding-top:7px; padding-left:5px;}')
        self.target_files_table.verticalHeader().hide()
        files_header = self.target_files_table.horizontalHeader()
        files_header.setFixedHeight(30)
        files_header.setDefaultAlignment(QtCore.Qt.AlignLeft)
        files_header.setContentsMargins(10, 10, 0, 0)
        files_header.setDefaultSectionSize(60)
        files_header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        files_header.setSectionResizeMode(1, QtWidgets.QHeaderView.Fixed)
        self.target_files_table.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        self.content_stacked_layout.addWidget(self.target_files_table)

        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        # ---->> Scene Information Table
        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        self.material_tree_view = QtWidgets.QTreeView()
        self.headers = ['Key', 'Value']
        self.material_tree_view.setStyleSheet('QTreeView::item {height:25px;} QHeaderView::section {background-color: rgb(220, 220, 220); height:30px; padding-left:10px}')

        self.material_tree_view.setFocusPolicy(QtCore.Qt.NoFocus)
        self.material_tree_view.setAlternatingRowColors(True)
        self.material_tree_view.setUniformRowHeights(True)
        self.content_stacked_layout.addWidget(self.material_tree_view)

        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        # ---->> LY Material Definitions
        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        self.material_definitions_widget = QtWidgets.QWidget()
        self.material_definitions_layout = QtWidgets.QHBoxLayout(self.material_definitions_widget)
        self.material_definitions_layout.setSpacing(0)
        self.material_definitions_layout.setContentsMargins(0, 0, 0, 0)
        self.material_definitions_frame = QtWidgets.QFrame(self.material_definitions_widget)
        self.material_definitions_frame.setGeometry(0, 0, 5000, 5000)
        self.material_definitions_frame.setStyleSheet('background-color:rgb(75,75,75);')
        self.material_definitions_scroller = QtWidgets.QScrollArea()
        self.scroller_widget = QtWidgets.QWidget()
        self.scroller_layout = QtWidgets.QVBoxLayout()
        self.scroller_widget.setLayout(self.scroller_layout)
        self.material_definitions_scroller.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOn)
        self.material_definitions_scroller.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.material_definitions_scroller.setWidgetResizable(True)
        self.material_definitions_scroller.setWidget(self.scroller_widget)
        self.material_definitions_layout.addWidget(self.material_definitions_scroller)
        self.content_stacked_layout.addWidget(self.material_definitions_widget)

        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        # ---->> File processing buttons
        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        self.process_files_layout = QtWidgets.QHBoxLayout()
        self.content_layout.addLayout(self.process_files_layout)
        self.process_files_button = QtWidgets.QPushButton('Process Listed Files')
        self.process_files_button.setFixedHeight(50)
        self.process_files_button.clicked.connect(self.process_listed_files_clicked)
        self.process_files_layout.addWidget(self.process_files_button)
        self.reset_button = QtWidgets.QPushButton('Reset')
        self.reset_button.setFixedSize(50, 50)
        self.reset_button.clicked.connect(self.reset_clicked)
        self.reset_button.setEnabled(False)
        self.process_files_layout.addWidget(self.reset_button)

        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
        # ---->> Status bar / Loader
        # >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

        # TODO- Move all processing of files to another thread and display progress with loader

        self.status_bar = QtWidgets.QStatusBar()
        self.status_bar.setStyleSheet('background-color: rgb(220, 220, 220);')
        self.status_bar.setContentsMargins(0, 0, 0, 0)
        self.status_bar.setSizeGripEnabled(False)
        self.message_readout_label = QtWidgets.QLabel('Ready.')
        self.message_readout_label.setStyleSheet('padding-left: 10px')
        self.status_bar.addWidget(self.message_readout_label)

        self.progress_bar = QtWidgets.QProgressBar()
        self.progress_bar_widget = QtWidgets.QWidget()
        self.progress_bar_widget_layout = QtWidgets.QHBoxLayout()
        self.progress_bar_widget_layout.setContentsMargins(0, 0, 0, 0)
        self.progress_bar_widget_layout.setAlignment(QtCore.Qt.AlignRight)
        self.progress_bar_widget.setLayout(self.progress_bar_widget_layout)
        self.status_bar.addPermanentWidget(self.progress_bar_widget)
        self.progress_bar_widget_layout.addWidget(self.progress_bar)
        self.progress_bar.setFixedSize(180, 20)
        self.main_container.addWidget(self.status_bar)

    ############################
    # UI Display Layers ########
    ############################

    def populate_source_files_table(self):
        """
        Adds selected files from the 'Source Files' section of the UI. This creates each item listing in the table
        as well as adds a 'Remove' button that will clear corresponding item from the table. Processed files will
        get color coded, based on whether or not the materials in the file could be successfully processed. Subsequent
        searches will not clear items from the table currently, as each item acts as a register of materials that have
        and have not yet been processed.
        :return:
        """
        self.target_files_table.setRowCount(0)
        for index, entry in enumerate(self.target_file_list):
            entry = entry[1] if type(entry) == list else entry
            self.target_files_table.insertRow(index)
            item = QtWidgets.QTableWidgetItem('  {}'.format(entry))
            self.target_files_table.setRowHeight(index, 45)
            remove_button = QtWidgets.QPushButton('Remove')
            remove_button.setFixedWidth(60)
            remove_button.clicked.connect(self.remove_source_file_clicked)
            self.target_files_table.setItem(index, 0, item)
            self.target_files_table.setCellWidget(index, 1, remove_button)

    def populate_dcc_material_values_tree(self):
        """
        Sets the materials model class to the file attribute tree.
        :return:
        """

        # TODO- Create mechanism for collapsing previously gathered materials, and or pushing them further down the list

        self.material_tree_view.setModel(self.model)
        self.material_tree_view.expandAll()
        self.material_tree_view.resizeColumnToContents(0)

    def populate_export_materials_list(self):
        """
        Once all materials have been analyzed inside of DCC applications, the 'Export Materials' view lists all
        materials presented as their Lumberyard counterparts. Each listing displays a representation of the material
        file based on its corresponding DCC material values and file connections.
        :return:
        """
        self.reset_export_materials_description()
        for count, value in enumerate(self.material_definitions):
            material_definition_node = MaterialNode([value, self.material_definitions[value]], count)
            self.material_definitions_nodes.append(material_definition_node)
            self.scroller_layout.addWidget(material_definition_node)
            self.scroller_layout.addLayout(self.create_separator_line())

    ############################
    # TBD ########
    ############################

    def process_file_list(self):
        """
        The entry point for reading DCC files and extracting values. Files are filtered and separated
        by DCC app (based on file extensions) before processing is done.

        Supported DCC applications:
        Maya (.ma, .mb, .fbx), 3dsMax(.max), Blender(.blend)
        :return:
        """
        files_dict = {'maya': [], 'max': [], 'blender': [], 'na': []}
        for file_location in self.target_file_list:
            file_name = os.path.basename(str(file_location))
            file_extension = os.path.splitext(file_name)[1]
            target_application = self.get_target_application(file_extension)
            if target_application in files_dict.keys():
                files_dict[target_application].append(file_location)

        for key, values in files_dict.items():
            for value in values:
                transfer_info = {'file_location': value, 'success': True}
                try:
                    if key == 'maya' and len(value):
                        self.get_maya_material_values(value)
                    elif key == 'max' and len(value):
                        self.get_max_material_values(value)
                    elif key == 'blender' and len(value):
                        self.get_blender_material_values(value)
                    else:
                        pass
                except Exception as e:
                    # TODO- Allow corrective actions or some display of errors if this fails?
                    transfer_info['success'] = [value, e]
                self.set_transfer_status(transfer_info)

        if self.materials_dictionary:
            # Create Model with extracted values from file list
            self.set_material_model()
            # Setup Lumberyard Material File Values
            self.set_export_materials_description()
            # Update UI Layout
            self.populate_export_materials_list()
            self.switch_layout_combobox.setCurrentIndex(2)
            self.set_ui_buttons()
            self.message_readout_label.setText('Ready.')

    def get_maya_material_values(self, target_files):
        """
        Launches Maya Standalone and processes list of materials for each scene passed to the 'target_files' argument.
        Also sets the environment paths needed for an instance of Maya's Python distribution. After files are processed
        a single dictionary of scene materials is returned, and added to the "materials_dictionary" scene attribute.
        :param target_files: List of files filtered from total list of files requested for processing that have a
        Maya file extension
        :return:
        """

        # TODO- Set load process to a separate thread and wire load progress bar up

        script_path = str(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'maya_materials.py'))
        runtime_env = os.environ.copy()
        runtime_env['MAYA_LOCATION'] = os.path.dirname(self.mayapy_path)
        runtime_env['PYMEL_SKIP_MEL_INIT'] = '1'
        runtime_env['PYTHONPATH'] = os.path.dirname(self.mayapy_path)
        command = f'{self.mayapy_path} "{script_path}" "{target_files}" "{self.total_materials}"'
        p = subprocess.Popen(command, shell=True, env=runtime_env, stdout=subprocess.PIPE)
        output = p.communicate()[0]
        self.set_material_dictionary(json.loads(output))

    def get_max_material_values(self, target_file):
        print('Max Target file: {}'.format(target_file))

    def get_blender_material_values(self, target_file):
        print('Blender Target file: {}'.format(target_file))

    def reset_export_materials_description(self):
        pass

    def reset_all_values(self):
        pass

    def create_separator_line(self):
        """ Convenience function for adding separation line to the UI. """
        layout = QtWidgets.QHBoxLayout()
        line = QtWidgets.QLabel()
        line.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        line.setLineWidth(1)
        line.setFixedHeight(10)
        layout.addWidget(line)
        layout.setContentsMargins(8, 0, 8, 0)
        return layout

    def export_selected_materials(self):
        """
        This will eventually be revised to save material definitions in the proper place in the user's project folder,
        but for now material definitions will be saved to the desktop.
        :return:
        """
        if not os.path.exists(self.lumberyard_materials_directory):
            os.makedirs(self.lumberyard_materials_directory)

        for node in self.material_definitions_nodes:
            if node.material_name_checkbox.isChecked():
                output = os.path.join(self.lumberyard_materials_directory, '{}.material'.format(node.material_name))
                with open(output, 'w', encoding='utf-8') as material_file:
                    json.dump(node.material_info, material_file, ensure_ascii=False, indent=4)

    ############################
    # Getters/Setters ##########
    ############################

    @staticmethod
    def get_target_application(file_extension):
        """
        Searches compatible file extensions and returns one of three Application names- Maya, 3dsMax, or Blender.
        :param file_extension: Passed file extension used to determine DCC Application it originated from.
        :return: Returns the application corresponding to the extension if found- otherwise returns a Boolean None
        """
        app_extensions = {'maya': ['.ma', '.mb', '.fbx'], 'max': ['.max'], 'blender': ['.blend']}
        target_dcc_application = [key for key, values in app_extensions.items() if file_extension in values]
        if target_dcc_application:
            return target_dcc_application[0]
        return None

    @staticmethod
    def get_lumberyard_material_template(shader_type):
        """
        Loads material descriptions from the Lumberyard installation, providing a template to compare and convert DCC
        shaders to Lumberyard material definitions. This is the first step in the comparison. The second step is to
        compare these values with specific mapping instructions for DCC Application and DCC material type to arrive at
        a converted material.
        :param shader_type: The type of Lumberyard shader to pair material attributes to (i.e. PBR Shader)
        :return: File dictionary of the available boilerplate Lumberyard shader settings.
        """
        definitions = os.path.join(os.path.dirname(os.path.abspath(__file__)), '{}.material'.format(shader_type))
        if os.path.exists(definitions):
            with open(definitions) as f:
                return json.load(f)

    @staticmethod
    def get_lumberyard_material_properties(name, material_type, file_connections):
        """ This system will probably need rethinking if DCCs and compatible materials grow """
        material_properties = {}
        if material_type == 'StingrayPBS':
            naming_exceptions = {'color': 'baseColor', 'ao': 'ambientOcclusion'}
            maps = 'color, metallic, roughness, normal, emissive, ao, opacity'.split(', ')
            for m in maps:
                texture_attribute = 'TEX_{}_map'.format(m)
                for tex in file_connections.keys():
                    if tex.find(texture_attribute) != -1:
                        key = m if m not in naming_exceptions else naming_exceptions.get(m)
                        material_properties[key] = {'useTexture': 'true',
                                                    'textureMap': file_connections.get('{}.{}'.format(name, texture_attribute))}
        return material_properties

    @staticmethod
    def get_filename_increment(name):
        """
        Convenience function that assists in ensuring that if any materials are encountered with the same name, an
        underscore and number is appended to it to prevent overwrites.
        :param name: The name of the material. The function searches the string for increment numbers, and either adds
        one to any encountered, or adds an "_1" if passed name is the first duplicate encountered.
        :return: The adjusted name with a unique incremental value.
        """
        last_number = re.compile(r'(?:[^\d]*(\d+)[^\d]*)+')
        number_found = last_number.search(name)
        if number_found:
            next_number = str(int(number_found.group(1)) + 1)
            start, end = number_found.span(1)
            name = name[:max(end - len(next_number), start)] + next_number + name[end:]
        return name

    def set_transfer_status(self, transfer_info):
        """
        Colorizes listings in the 'Source Files' view of the UI after processing to green or red, indicating whether or
        not scene analysis successfully returned compatible materials and their values.
        :param transfer_info: Each file the scripts attempt to process return a receipt of the success or failure of
        the analysis.
        :return:
        """
        # TODO- Include some way to get error information if analysis fails, and potentially offer the means to
        #  effectively repair values as they map to intended Lumberyard shader type

        for row in range(self.target_files_table.rowCount()):
            if self.target_files_table.item(row, 0).text().strip() == transfer_info['file_location']:
                if transfer_info['success']:
                    self.target_files_table.item(row, 0).setBackground(QtGui.QColor(192, 255, 171))
                else:
                    self.target_files_table.item(row, 0).setBackground(QtGui.QColor(255, 177, 171))

    def set_export_materials_description(self):
        root = self.model.rootItem
        for row in range(self.model.rowCount()):
            source_file = self.model.get_attribute_value('SceneName', root.child(row))
            name = self.model.get_attribute_value('MaterialName', root.child(row))
            material_type = self.model.get_attribute_value('MaterialType', root.child(row))
            file_connections = {}
            shader_attributes = {}

            for childIndex in range(root.child(row).childCount()):
                child_item = root.child(row).child(childIndex)
                child_value = child_item.itemData
                if child_item.childCount():
                    target_dict = file_connections if child_value[0] == 'FileConnections' else shader_attributes
                    for subChildIndex in range(child_item.childCount()):
                        sub_child_data = child_item.child(subChildIndex).itemData
                        target_dict[sub_child_data[0]] = sub_child_data[1]
            self.set_pbr_material_description(source_file, name, material_type, file_connections)

    def set_material_dictionary(self, dcc_dictionary):
        """
        Adds all material descriptions pulled from each DCC file analyzed to the "materials_dictionary" class attribute.
        This function runs each time a subprocess is launced to gather DCC application material values.
        :param dcc_dictionary: The dictionary of values for each material analyzed by each specific DCC file list
        return analyzed values
        :return:
        """
        self.total_materials += len(dcc_dictionary)
        self.materials_dictionary.update(dcc_dictionary)

    def set_material_model(self, initialize=True):
        """
        Once all materials have been gathered across a selected file set query, this organizes the values into a
        QT Model Class
        :param initialize: Default is set to boolean True. If a model has already been established in the current
        session, the initialize parameter would be set to false, and the values added to the Model. All changes to
        the model would then be redistributed to other informational views in the UI.
        :return:
        """
        if initialize:
            self.model = MaterialsModel(self.headers, self.materials_dictionary)
        else:
            self.model.update()
        self.materials_dictionary.clear()
        self.populate_dcc_material_values_tree()

    def set_ui_buttons(self):
        """
        Handles UI buttons for each of the three stacked layout views (Source Files, DCC Material Values,
        Export Materials)
        :return:
        """
        display_index = self.content_stacked_layout.currentIndex()
        self.switch_layout_combobox.setEnabled(True)
        # Source Files Layout ----------------------------------->>
        if display_index == 0:
            self.select_files_button.setEnabled(True)
            self.reset_button.setEnabled(True)
            self.process_files_button.setText('Process Listed Files')

        # DCC Material Values Layout ---------------------------->>
        elif display_index == 1:
            self.reset_button.setEnabled(True)
            self.process_files_button.setEnabled(False)
            self.select_files_button.setEnabled(False)

        # Export Materials Layout ------------------------------->>
        else:
            self.select_files_button.setEnabled(False)
            self.process_files_button.setText('Export Selected Materials')
            if self.material_definitions:
                self.process_files_button.setEnabled(True)

    def set_pbr_material_description(self, source_file, name, material_type, file_connections):
        # Build dictionary for material description based on extracted values
        default_settings = self.get_lumberyard_material_template('pbr')
        material = {'sourceFile': source_file,
                    'description': name,
                    'materialType': default_settings.get('materialType'),
                    'parentMaterial': default_settings.get('parentMaterial'),
                    'propertyLayoutVersion': default_settings.get('propertyLayoutVersion'),
                    'properties': self.get_lumberyard_material_properties(name, material_type, file_connections)}
        self.material_definitions[name if name not in self.material_definitions.keys() else
                                  self.get_filename_increment(name)] = material

    ############################
    # Button Actions ###########
    ############################

    def remove_source_file_clicked(self):
        """
        In the Source File view of the UI layout, this will remove the listed file in its respective row. If files
        have not been processed yet, it prevents that file from being analyzed. If the files have already been
        analyzed, this will remove the materials from stored values.
        :return:
        """
        file_index = self.target_files_table.indexAt(self.sender().pos())
        del self.target_file_list[file_index.row()]
        self.populate_files_table()

    def process_listed_files_clicked(self):
        """
        The button serves a dual purpose, depending on the current layout of the window. 'Process listed files'
        initiates the DCC file analysis that extracts material information. In the "Export Materials" layout, this
        button (for now) will export material files corresponding to each analyzed material.
        :return:
        """

        # TODO- Need to decide how the materials are going to be routed. At this stage they will just be saved to the
        #   desktop, but I assume that we want these files to be saved to an associated project folder

        if self.sender().text() == 'Process Listed Files':
            self.message_readout_label.setText('Gathering Material Information...')
            self.app.processEvents()
            self.process_file_list()
        else:
            self.export_selected_materials()

    def select_files_button_clicked(self):
        """
        This dialog allows user to select DCC files to be processed for the materials present for conversion.
        :return:
        """

        # TODO- Eventually it might be worth it to allow files from multiple locations to be selected. Currently
        #  this only allows single/multiple files from a single directory to be selected.

        dialog = QtWidgets.QFileDialog(self, 'Shift-Select Target Files', self.desktop_location)
        dialog.setFileMode(QtWidgets.QFileDialog.ExistingFile)
        dialog.setNameFilter('Compatible Files (*.ma *.mb *.fbx *.max *.blend)')
        dialog.setOption(QtWidgets.QFileDialog.DontUseNativeDialog, True)
        file_view = dialog.findChild(QtWidgets.QListView, 'listView')

        # Workaround for selecting multiple files with File Dialog
        if file_view:
            file_view.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)
        f_tree_view = dialog.findChild(QtWidgets.QTreeView)
        if f_tree_view:
            f_tree_view.setSelectionMode(QtWidgets.QAbstractItemView.MultiSelection)

        if dialog.exec_() == QtWidgets.QDialog.Accepted:
            self.target_file_list = dialog.selectedFiles()
            if self.target_file_list:
                self.populate_source_files_table()
                self.process_files_button.setEnabled(True)

    def layout_combobox_changed(self):
        """
        Handles main window layout combobox index change.
        :return:
        """
        self.content_stacked_layout.setCurrentIndex(self.switch_layout_combobox.currentIndex())
        self.set_ui_buttons()

    def reset_clicked(self):
        """
        Brings the application and all variables back to their initial state.
        :return:
        """
        self.reset_all_values()


class MaterialNode(QtWidgets.QWidget):
    def __init__(self, material_info, current_position, parent=None):
        super(MaterialNode, self).__init__(parent)

        self.material_name = material_info[0]
        self.material_info = material_info[1]
        self.current_position = current_position
        self.property_settings = {}

        self.small_font = QtGui.QFont("Helvetica", 7, QtGui.QFont.Bold)
        self.bold_font = QtGui.QFont("Helvetica", 8, QtGui.QFont.Bold)
        self.main_layout = QtWidgets.QVBoxLayout()
        self.main_layout.setContentsMargins(0, 0, 0, 0)
        self.setLayout(self.main_layout)

        self.background_frame = QtWidgets.QFrame(self)
        self.background_frame.setGeometry(0, 0, 5000, 5000)
        self.background_frame.setStyleSheet('background-color:rgb(220, 220, 220);')

        # ########################
        #  Title Bar
        # ########################

        self.title_bar_widget = QtWidgets.QWidget()
        self.title_bar_layout = QtWidgets.QHBoxLayout(self.title_bar_widget)
        self.title_bar_layout.setContentsMargins(10, 0, 10, 0)
        self.title_bar_layout.setAlignment(QtCore.Qt.AlignTop)
        self.title_bar_frame = QtWidgets.QFrame(self.title_bar_widget)
        self.title_bar_frame.setGeometry(0, 0, 5000, 40)
        self.title_bar_frame.setStyleSheet('background-color:rgb(193,154,255);')
        self.main_layout.addWidget(self.title_bar_widget)
        self.material_name_checkbox = QtWidgets.QCheckBox(self.material_name)
        self.material_name_checkbox.setFixedHeight(35)
        self.material_name_checkbox.setStyleSheet('spacing:10px; color:white')
        self.material_name_checkbox.setFont(self.bold_font)
        self.material_name_checkbox.setChecked(True)
        self.title_bar_layout.addWidget(self.material_name_checkbox)

        self.material_file_layout = QtWidgets.QHBoxLayout()
        self.material_file_layout.setAlignment(QtCore.Qt.AlignRight)
        self.source_file = QtWidgets.QLabel(os.path.basename(self.material_info['sourceFile']))
        self.source_file.setStyleSheet('color:white;')
        self.source_file.setFont(self.small_font)
        self.material_file_layout.addWidget(self.source_file)
        self.material_file_layout.addSpacing(10)

        self.edit_button = QtWidgets.QPushButton('Edit')
        self.edit_button.clicked.connect(self.edit_button_clicked)
        self.edit_button.setFixedWidth(55)
        self.material_file_layout.addWidget(self.edit_button)
        self.title_bar_layout.addLayout(self.material_file_layout)

        self.information_layout = QtWidgets.QHBoxLayout()
        self.information_layout.setContentsMargins(10, 0, 10, 10)
        self.main_layout.addLayout(self.information_layout)

        # ########################
        #  Details layout
        # ########################

        self.details_layout = QtWidgets.QVBoxLayout()
        self.details_layout.setAlignment(QtCore.Qt.AlignTop)
        self.details_groupbox = QtWidgets.QGroupBox("Details")
        self.details_groupbox.setFixedWidth(200)
        self.details_groupbox.setStyleSheet("QGroupBox {font:bold; border: 1px solid silver; "
                                            "margin-top: 6px;} QGroupBox::title { color: rgb(150, 150, 150); "
                                            "subcontrol-position: top left;}")
        self.details_layout.addSpacing(15)
        self.material_type_label = QtWidgets.QLabel('Material Type')
        self.material_type_label.setStyleSheet('padding-left: 6px; color: white; background-color:rgb(175, 175, 175);')
        self.material_type_label.setFixedHeight(25)
        self.material_type_label.setFont(self.bold_font)
        self.details_layout.addWidget(self.material_type_label)

        self.material_type_combobox = QtWidgets.QComboBox()
        self.material_type_combobox.setFixedHeight(30)
        self.material_type_combobox.setStyleSheet('QCombobox QAbstractItemView { padding-left: 15px; }')
        material_type_items = ['  Standard PBR']
        self.material_type_combobox.addItems(material_type_items)
        self.details_layout.addWidget(self.material_type_combobox)
        self.details_layout.addSpacing(10)

        self.description_label = QtWidgets.QLabel('Description')
        self.description_label.setStyleSheet('padding-left: 6px; color: white; background-color:rgb(175, 175, 175);')
        self.description_label.setFixedHeight(25)
        self.description_label.setFont(self.bold_font)
        self.details_layout.addWidget(self.description_label)

        self.description_box = QtWidgets.QTextEdit('This space is reserved for additional information.')
        self.details_layout.addWidget(self.description_box)
        self.information_layout.addWidget(self.details_groupbox)
        self.details_groupbox.setLayout(self.details_layout)

        # ########################
        #  Properties layout
        # ########################

        self.properties_layout = QtWidgets.QVBoxLayout()
        self.properties_layout.setAlignment(QtCore.Qt.AlignTop)
        self.properties_groupbox = QtWidgets.QGroupBox("Properties")
        self.properties_groupbox.setFixedWidth(150)
        self.properties_groupbox.setStyleSheet("QGroupBox {font:bold; border: 1px solid silver; "
                                               "margin-top: 6px;} QGroupBox::title { color: rgb(150, 150, 150); "
                                               "subcontrol-position: top left;}")
        self.properties_list_widget = QtWidgets.QListWidget()
        self.material_properties = ['ambientOcclusion', 'baseColor', 'emissive', 'metallic', 'roughness', 'specularF0',
                                    'normal', 'opacity']
        self.properties_list_widget.addItems(self.material_properties)
        self.properties_list_widget.itemSelectionChanged.connect(self.property_selection_changed)
        self.properties_layout.addSpacing(15)
        self.properties_layout.addWidget(self.properties_list_widget)
        self.information_layout.addWidget(self.properties_groupbox)
        self.properties_groupbox.setLayout(self.properties_layout)

        # ########################
        #  Attributes layout
        # ########################

        self.attributes_layout = QtWidgets.QVBoxLayout()
        self.attributes_layout.setAlignment(QtCore.Qt.AlignTop)
        self.attributes_groupbox = QtWidgets.QGroupBox("Attributes")
        self.attributes_groupbox.setStyleSheet("QGroupBox {font:bold; border: 1px solid silver; "
                                               "margin-top: 6px;} QGroupBox::title { color: rgb(150, 150, 150); "
                                               "subcontrol-position: top left;}")
        self.information_layout.addWidget(self.attributes_groupbox)
        self.attributes_layout.addSpacing(15)
        self.attributes_table = QtWidgets.QTableWidget()
        self.attributes_table.setFocusPolicy(QtCore.Qt.NoFocus)
        self.attributes_table.setColumnCount(2)
        self.attributes_table.setAlternatingRowColors(True)
        self.attributes_table.setHorizontalHeaderLabels(['Attribute', 'Value'])
        attributes_table_header = self.attributes_table.horizontalHeader()
        attributes_table_header.setStyleSheet('QHeaderView::section {background-color: rgb(220, 220, 220);}')
        attributes_table_header.setDefaultAlignment(QtCore.Qt.AlignLeft)
        attributes_table_header.setContentsMargins(10, 10, 0, 0)
        attributes_table_header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        attributes_table_header.setSectionResizeMode(1, QtWidgets.QHeaderView.Stretch)
        self.attributes_layout.addWidget(self.attributes_table)
        self.attributes_groupbox.setLayout(self.attributes_layout)

        self.initialize_display_values()

    def initialize_display_values(self):
        """
        Initializes all of the widget item information for material based on the DCC application info the class has
        been passed.
        :return:
        """
        for material_property in self.material_properties:
            if material_property in self.material_info.get('properties'):
                self.property_settings[material_property] = self.material_info['properties'].get(material_property)
                current_row = self.material_properties.index(material_property)
                current_item = self.properties_list_widget.takeItem(current_row)
                self.properties_list_widget.insertItem(0, current_item)
            else:
                self.property_settings[material_property] = 'inactive'
                current_row = self.material_properties.index(material_property)
                item = self.properties_list_widget.item(current_row)
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsEnabled)
                item.setFlags(item.flags() & ~QtCore.Qt.ItemIsSelectable)

        self.properties_list_widget.setCurrentRow(0)
        self.set_attributes_table(self.get_selected_property())

    def set_attributes_table(self, selected_property):
        """
        Displays the key, value pairs for the item selected in the Properties list widget
        :param selected_property: The item in the Properties list widget that is currently selected. Only active
        values are displayed.
        :return:
        """
        self.attributes_table.setRowCount(0)
        row_count = 0
        for key, value in self.property_settings[selected_property].items():
            self.attributes_table.insertRow(row_count)
            key_item = QtWidgets.QTableWidgetItem(key)
            self.attributes_table.setItem(row_count, 0, key_item)
            value_item = QtWidgets.QTableWidgetItem(value)
            self.attributes_table.setItem(row_count, 1, value_item)
            row_count += 1

    def get_selected_property(self):
        """
        Convenience function to get current value selected in the Properties list widget.
        :return:
        """
        return self.properties_list_widget.currentItem().text()

    def update_model(self):
        """
        Not sure if this will go away, but if desired, I could make attribute values able to be revised after
        materials have been scraped from the DCC materials
        :return:
        """
        pass

    def edit_button_clicked(self):
        """
        This is in place in the event that we want to allow material revisions for properties to be made after
        DCC processing step has already been executed. The idea would basically be to surface an editable
        table where values can be added, removed or changed within the final material definition.
        :return:
        """
        print('Edit button clicked')

    def property_selection_changed(self):
        """
        Fired when index of list view selected property selection has changed.
        :return:
        """
        self.set_attributes_table(self.get_selected_property())


if __name__ == '__main__':
    app = QApplication(sys.argv)
    materials_to_lumberyard = MaterialsToLumberyard()
    materials_to_lumberyard.show()
    sys.exit(app.exec_())

