"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------

"""
Usage
=====
Put usage instructions here.

Output
======
Put output information here.

Relevant Links:
https://pythonhosted.org/an_example_pypi_project/sphinx.html
https://github.com/ideasman42/pyfbx_i42
https://pypi.org/project/py-fbx/
https://www.quora.com/How-do-I-execute-Maya-script-without-lauching-Maya
https://stackoverflow.com/questions/27437733/use-external-python-script-to-open-maya-and-run-another-script-inside-maya
 
Notes:
-- Materials information can be extracted from ASCII fbx pretty easily, binary is possible but more difficult
-- FBX files could be exported as ASCII files and I could use regex there to extract material information 
-- I couldn't get pyfbx_i42 to work, but purportedly it can extract information from binary files. You may just have
to use the specified python versions

-- Jonny wants me to use pathlib wherever possible for OO pathing, as well as python-box (aka Box) for dict access

# Things to do:
    --> Create the cube demo for Jonny with 4 attached materials
    --> Create mapping widget for stacked layout
    --> Allow export of JSON file for demo
    --> Allow custom field entries for description, etc.
    --> Need to figure out how to clear pointers for stored data properly
"""

from PySide2 import QtWidgets, QtCore, QtGui
from PySide2.QtCore import Signal, Slot, QThread, QAbstractItemModel, QModelIndex, QObject
from maya import OpenMayaUI as omui
from maya.standalone import initialize
from shiboken2 import wrapInstance
import pymel.core as pm
# import sphinx
# import azpy
import json
import os
import re


mayaMainWindowPtr = omui.MQtUtil.mainWindow()
mayaMainWindow = wrapInstance(long(mayaMainWindowPtr), QtWidgets.QWidget)


class MayaToLumberyard(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(MayaToLumberyard, self).__init__(parent)

        self.app = QtWidgets.QApplication.instance()
        self.setParent(mayaMainWindow)
        self.setWindowFlags(QtCore.Qt.Window)
        self.setGeometry(50, 50, 600, 500)
        self.setObjectName('MaterialsToLumberyard')
        self.setWindowTitle('Maya To Lumberyard')
        self.isTopLevel()
        self.setWindowFlags(self.windowFlags() & ~QtCore.Qt.WindowMinMaxButtonsHint)

        self.desktop_location = os.path.join(os.path.expanduser('~'), 'Desktop')
        self.bold_font_large = QtGui.QFont('Plastique', 7, QtGui.QFont.Bold)
        self.medium_font = QtGui.QFont('Plastique', 7, QtGui.QFont.Normal)
        self.target_file_list = []
        self.materials_dict = {}
        self.material_definitions = {}
        self.processed_materials = []
        self.current_scene = pm.sceneName()
        self.model = None
        self.total_transfer_materials = 1

        self.main_container = QtWidgets.QVBoxLayout(self)
        self.main_container.setAlignment(QtCore.Qt.AlignTop)
        self.setLayout(self.main_container)

        # Header Bar ------>
        self.header_bar_layout = QtWidgets.QHBoxLayout()
        self.select_files_button = QtWidgets.QPushButton('Select Files')
        self.select_files_button.clicked.connect(self.choose_files_clicked)
        self.select_files_button.setFixedSize(80, 35)
        self.header_bar_layout.addWidget(self.select_files_button)
        self.header_bar_layout.addSpacing(15)
        self.use_current_file_checkbox = QtWidgets.QCheckBox('Use Current File')
        self.use_current_file_checkbox.setFont(self.bold_font_large)
        self.use_current_file_checkbox.clicked.connect(self.use_current_file_clicked)
        self.header_bar_layout.addWidget(self.use_current_file_checkbox)
        self.header_bar_layout.addSpacing(100)
        self.switch_layout_combobox = QtWidgets.QComboBox()
        self.switch_layout_combobox.setEnabled(False)
        self.switch_layout_combobox.setFixedSize(250, 30)
        self.combobox_items = ['Target Files', 'Extracted Values', 'Material Tree']
        self.switch_layout_combobox.setStyleSheet('QComboBox {padding-left:6px;}')
        self.switch_layout_combobox.addItems(self.combobox_items)
        self.header_bar_layout.addWidget(self.switch_layout_combobox)
        self.header_bar_layout.addSpacing(4)
        self.main_container.addSpacing(5)
        self.main_container.addLayout(self.header_bar_layout)

        # Separation Line ------>
        self.separatorLayout1 = QtWidgets.QHBoxLayout()
        self.line1 = QtWidgets.QLabel()
        self.line1.setFrameStyle(QtWidgets.QFrame.HLine | QtWidgets.QFrame.Sunken)
        self.line1.setLineWidth(1)
        self.line1.setFixedHeight(10)
        self.separatorLayout1.addWidget(self.line1)
        self.main_container.addLayout(self.separatorLayout1)

        # ++++++++++++++++++++++++++++++++++++++++++++++++#
        # File Source Table / Attributes (Stacked Layout) #
        # ++++++++++++++++++++++++++++++++++++++++++++++++#
        self.content_stacked_layout = QtWidgets.QStackedLayout()
        self.main_container.addLayout(self.content_stacked_layout)
        self.switch_layout_combobox.currentIndexChanged.connect(self.layout_combobox_changed)

        # --- Files Table
        self.target_files_table = QtWidgets.QTableWidget()
        self.target_files_table.setFocusPolicy(QtCore.Qt.NoFocus)
        self.target_files_table.setColumnCount(2)
        self.target_files_table.setAlternatingRowColors(True)
        self.target_files_table.setHorizontalHeaderLabels(['File List', ''])
        self.target_files_table.horizontalHeader().setStyleSheet('QHeaderView::section {padding-top:9px; padding-left:10px;}')
        self.target_files_table.verticalHeader().hide()
        files_header = self.target_files_table.horizontalHeader()
        files_header.setFixedHeight(30)
        files_header.setDefaultAlignment(QtCore.Qt.AlignLeft)
        files_header.setContentsMargins(10, 10, 0, 0)
        files_header.setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        files_header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeToContents)
        self.target_files_table.setSelectionMode(QtWidgets.QAbstractItemView.NoSelection)
        self.content_stacked_layout.addWidget(self.target_files_table)

        # --- Scene Information Table
        self.material_tree_view = QtWidgets.QTreeView()
        self.headers = ['Key', 'Value']
        self.material_tree_view.setStyleSheet('QTreeView::item {height:25px;} QHeaderView::section {height:30px; padding-left:10px}')

        self.material_tree_view.setFocusPolicy(QtCore.Qt.NoFocus)
        self.material_tree_view.setAlternatingRowColors(True)
        self.material_tree_view.setUniformRowHeights(True)
        self.content_stacked_layout.addWidget(self.material_tree_view)

        # --- LY Material Definitions
        self.material_definitions_widget = QtWidgets.QWidget()
        self.material_definitions_layout = QtWidgets.QHBoxLayout(self.material_definitions_widget)
        self.material_definitions_layout.setSpacing(0)
        self.material_definitions_layout.setContentsMargins(0, 0, 0, 0)
        self.material_definitions_frame = QtWidgets.QFrame(self.material_definitions_widget)
        self.material_definitions_frame.setGeometry(0, 0, 5000, 5000)
        self.material_definitions_frame.setStyleSheet('background-color:rgb(150,150,150);')

        self.title_bar_widget = QtWidgets.QWidget()
        self.title_bar_layout = QtWidgets.QHBoxLayout(self.title_bar_widget)
        self.title_bar_layout.setContentsMargins(17, 17, 17, 0)
        self.title_bar_layout.setAlignment(QtCore.Qt.AlignTop)
        self.title_bar_frame = QtWidgets.QFrame(self.title_bar_widget)
        self.title_bar_frame.setGeometry(0, 0, 5000, 60)
        self.title_bar_frame.setStyleSheet('background-color:rgb(102,69,153);')
        self.material_definitions_layout.addWidget(self.title_bar_widget)

        self.material_name = QtWidgets.QCheckBox('StingrayPBS2')
        self.material_name.setStyleSheet('spacing:10px; color:white')
        self.material_name.setFont(self.bold_font_large)
        self.material_name.setChecked(True)
        self.title_bar_layout.addWidget(self.material_name)

        # Forward/Back Buttons -------------------------->>
        self.previous_next_button_layout = QtWidgets.QHBoxLayout()
        self.item_count = QtWidgets.QLabel('1 of 10')
        self.previous_next_button_layout.addWidget(self.item_count)
        self.previous_next_button_layout.addSpacing(10)
        self.previous_next_button_layout.setContentsMargins(0, 0, 0, 0)
        self.previous_next_button_layout.setAlignment(QtCore.Qt.AlignRight)
        self.previous_button = QtWidgets.QToolButton()
        self.previous_button.setArrowType(QtCore.Qt.LeftArrow)
        self.previous_button.setStyleSheet('background-color:rgb(110,110,110);')
        self.previous_button.clicked.connect(self.previous_button_clicked)
        self.previous_next_button_layout.addWidget(self.previous_button)
        self.next_button = QtWidgets.QToolButton()
        self.next_button.setArrowType(QtCore.Qt.RightArrow)
        self.next_button.setStyleSheet('background-color:rgb(110,110,110);')
        self.next_button.clicked.connect(self.next_button_clicked)
        self.previous_next_button_layout.addWidget(self.next_button)
        self.title_bar_layout.addLayout(self.previous_next_button_layout)
        self.content_stacked_layout.addWidget(self.material_definitions_widget)

        # File processing buttons ------>
        self.process_files_layout = QtWidgets.QHBoxLayout()
        self.main_container.addLayout(self.process_files_layout)
        self.process_files_button = QtWidgets.QPushButton('Process Listed Files')
        self.process_files_button.setFixedHeight(50)
        self.process_files_button.clicked.connect(self.process_files_clicked)
        self.process_files_layout.addWidget(self.process_files_button)
        self.reset_button = QtWidgets.QPushButton('Reset')
        self.reset_button.setFixedSize(50, 50)
        self.reset_button.clicked.connect(self.reset_clicked)
        self.reset_button.setEnabled(False)
        self.process_files_layout.addWidget(self.reset_button)
        self.initialize_window()

    def initialize_window(self):
        if self.current_scene:
            self.target_file_list = [self.current_scene]
            self.use_current_file_checkbox.setChecked(True)
            self.populate_files_table()

    def populate_files_table(self):
        self.target_files_table.setRowCount(0)
        for index, entry in enumerate(self.target_file_list):
            entry = entry[1] if type(entry) == list else entry
            self.target_files_table.insertRow(index)
            item = QtWidgets.QTableWidgetItem('  {}'.format(entry))
            self.target_files_table.setRowHeight(index, 45)
            remove_button = QtWidgets.QPushButton('  Remove  ')
            remove_button.setStyleSheet('border-width:0px; background-color:rgb(100,100,100);')
            remove_button.clicked.connect(self.remove_file_clicked)
            self.target_files_table.setItem(index, 0, item)
            self.target_files_table.setCellWidget(index, 1, remove_button)

    def process_file_list(self):
        file_processing_errors = []
        for maya_file_location in self.target_file_list:
            try:
                if maya_file_location != self.current_scene:
                    pm.openFile(maya_file_location, force=True)
                    self.current_scene = maya_file_location
                self.get_scene_materials_description()
            except Exception as e:
                file_processing_errors.append([maya_file_location, e])

        # Create Model with extracted values from file list
        self.set_material_model()

        # Setup Lumberyard Material File Values
        self.map_materials()
        print ('MaterialDefinitions:'.format(self.material_definitions))
        print(json.dumps(self.material_definitions, sort_keys=True, indent=4))

        # Update UI Layout
        self.set_material_view()
        self.switch_layout_combobox.setCurrentIndex(2)
        self.set_ui_buttons()

    def map_materials(self):
        root = self.model.rootItem
        for row in range(self.model.rowCount()):
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
            self.set_pbr_material_description(name, material_type, file_connections)

    def reset_all_values(self):
        pass
        # Need to figure out how to clear pointers for stored data properly
        # self.target_files_table.setRowCount(0)
        # self.model.beginResetModel()
        # self.model.qDeleteAll(mResults)
        # self.model.endResetModel()
        # self.initialize_window()

    ############################
    # Getters/Setters ##########
    ############################

    @staticmethod
    def get_materials(target_mesh):
        shading_group = pm.listConnections(pm.PyNode(target_mesh), type='shadingEngine')
        materials = pm.ls(pm.listConnections(shading_group), materials=1)
        return list(set(materials))

    @staticmethod
    def get_shader(material_name):
        connections = pm.listConnections(material_name, type='shadingEngine')[0]
        shader_name = '{}.surfaceShader'.format(connections)
        shader = pm.listConnections(shader_name)[0]
        return shader

    @staticmethod
    def get_shader_information(shader):
        shader_file_connections = {}
        for node in pm.listConnections(shader, type='file', c=True):
            shader_file_connections[str(node[0])] = str(pm.getAttr(node[1].fileTextureName))
        shader_attributes = {}
        for shader_attribute in pm.listAttr(shader, s=True, iu=True):
            try:
                shader_attributes[str(shader_attribute)] = pm.getAttr('{}.{}'.format(shader, shader_attribute))
            except pm.MayaAttributeError as e:
                print ('MayaAttributeError: {}'.format(e))

        return shader_file_connections, shader_attributes

    @staticmethod
    def get_shader_properties(name, material_type, file_connections):
        """ This system will probably need rethinking if DCCs and compatible materials grow """
        attr_list = {}
        if material_type == 'StingrayPBS':
            naming_exceptions = {'color': 'baseColor', 'ao': 'ambientOcclusion'}
            maps = 'color, metallic, roughness, normal, emissive, ao, opacity'.split(', ')
            for m in maps:
                texture_attribute = 'TEX_{}_map'.format(m)
                for tex in file_connections.keys():
                    if tex.find(texture_attribute) != -1:
                        key = m if m not in naming_exceptions else naming_exceptions.get(m)
                        attr_list[key] = {'useTexture': 'true',
                                          'textureMap': file_connections.get('{}.{}'.format(name, texture_attribute))}
        return attr_list

    @staticmethod
    def get_increment(name):
        last_number = re.compile(r'(?:[^\d]*(\d+)[^\d]*)+')
        number_found = last_number.search(name)
        if number_found:
            next_number = str(int(number_found.group(1)) + 1)
            start, end = number_found.span(1)
            name = name[:max(end - len(next_number), start)] + next_number + name[end:]
        return name

    @staticmethod
    def get_material_template(shader_type):
        definitions = os.path.join(os.path.dirname(os.path.abspath(__file__)), '{}.material'.format(shader_type))
        if os.path.exists(definitions):
            with open(definitions) as f:
                return json.load(f)

    def get_scene_materials_description(self):
        scene_geo = pm.ls(v=True, geometry=True)
        for target_mesh in scene_geo:
            material_list = self.get_materials(target_mesh)
            for material_name in material_list:
                material_type = pm.nodeType(material_name, api=True)
                material_listed = [x for x in self.materials_dict if self.materials_dict[x]['MaterialName'] == material_name]

                if not material_listed:
                    self.set_material_dict(str(material_name), str(material_type), target_mesh)
                else:
                    mesh_list = self.materials_dict[material_name].get('AppliedMesh')
                    if not isinstance(mesh_list, list):
                        self.materials_dict[material_name]['AppliedMesh'] = [mesh_list, target_mesh]
                    else:
                        mesh_list.append(target_mesh)

    def set_material_dict(self, material_name, material_type, material_mesh):
        shader = self.get_shader(material_name)
        shader_file_connections, shader_attributes = self.get_shader_information(shader)
        material_dict = {'MaterialName': material_name, 'MaterialType': material_type, 'AppliedMesh': material_mesh,
                         'FileConnections': shader_file_connections, 'SceneName': str(self.current_scene),
                         'MaterialAttributes': shader_attributes}
        material_name = 'Material_{}'.format(self.total_transfer_materials)
        self.materials_dict[material_name] = material_dict
        self.total_transfer_materials += 1

    def set_material_model(self):
        self.model = MaterialsModel(self.headers, self.materials_dict)

    def set_material_view(self):
        self.material_tree_view.setModel(self.model)
        self.material_tree_view.expandAll()
        self.material_tree_view.resizeColumnToContents(0)

    def set_pbr_material_description(self, name, material_type, file_connections):
        # Build dictionary for material description based on extracted values
        default_settings = self.get_material_template('pbr')
        material = {'description': name,
                    'materialType': default_settings.get('materialType'),
                    'parentMaterial': default_settings.get('parentMaterial'),
                    'materialTypeVersion': default_settings.get('materialTypeVersion'),
                    'properties': self.get_shader_properties(name, material_type, file_connections)}
        self.material_definitions[name if name not in self.material_definitions.keys() else self.get_increment(name)] = material

    def set_ui_buttons(self):
        display_index = self.content_stacked_layout.currentIndex()
        self.switch_layout_combobox.setEnabled(True)
        # Target Files
        if display_index == 0:
            self.use_current_file_checkbox.setEnabled(True)
            self.select_files_button.setEnabled(True)
            self.reset_button.setEnabled(True)
            self.process_files_button.setText('Process Listed Files')

        # Extracted Values
        elif display_index == 1:
            self.reset_button.setEnabled(True)
            self.process_files_button.setEnabled(False)
            self.use_current_file_checkbox.setEnabled(False)
            self.select_files_button.setEnabled(False)

        # Material Tree
        else:
            self.use_current_file_checkbox.setEnabled(False)
            self.select_files_button.setEnabled(False)
            self.process_files_button.setText('Export Selected Materials')
            if self.material_definitions:
                self.process_files_button.setEnabled(True)

    ############################
    # Button Actions ###########
    ############################

    def use_current_file_clicked(self):
        self.current_scene = pm.sceneName()
        if self.use_current_file_checkbox.isChecked():
            self.target_file_list.insert(0, self.current_scene)
            self.target_file_list = list(set(self.target_file_list))
        else:
            if self.current_scene in self.target_file_list:
                del self.target_file_list[self.target_file_list.index(self.current_scene)]
        self.populate_files_table()

    def remove_file_clicked(self):
        file_index = self.target_files_table.indexAt(self.sender().pos())
        target_file = self.target_file_list[file_index.row()]
        if target_file == pm.sceneName():
            self.use_current_file_checkbox.setChecked(False)
        del self.target_file_list[file_index.row()]
        self.populate_files_table()

    def process_files_clicked(self):
        self.process_file_list()

    def choose_files_clicked(self):
        dialog = QtWidgets.QFileDialog(self, 'Shift-Select Target Files', self.desktop_location)
        dialog.setFileMode(QtWidgets.QFileDialog.ExistingFile)
        dialog.setNameFilter('Maya Files (*.ma *.mb *.fbx)')
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
                self.populate_files_table()
                self.process_files_button.setEnabled(True)

    def layout_combobox_changed(self):
        self.content_stacked_layout.setCurrentIndex(self.switch_layout_combobox.currentIndex())
        self.set_ui_buttons()

    def reset_clicked(self):
        self.reset_all_values()

    def previous_button_clicked(self):
        print ('Previous button clicked')

    def next_button_clicked(self):
        print ('Next button clicked')


class MaterialsModel(QAbstractItemModel):
    def __init__(self, headers, data, parent=None):
        super(MaterialsModel, self).__init__(parent)

        self.rootItem = TreeNode(headers)
        self.parents = [self.rootItem]
        self.indentations = [0]
        self.create_data(data)

    def create_data(self, data, indent=-1):
        if type(data) == dict:
            indent += 1
            position = 4 * indent
            for key, value in data.iteritems():
                if position > self.indentations[-1]:
                    if self.parents[-1].childCount() > 0:
                        self.parents.append(self.parents[-1].child(self.parents[-1].childCount() - 1))
                        self.indentations.append(position)
                else:
                    while position < self.indentations[-1] and len(self.parents) > 0:
                        self.parents.pop()
                        self.indentations.pop()
                parent = self.parents[-1]
                parent.insertChildren(parent.childCount(), 1, parent.columnCount())
                parent.child(parent.childCount() - 1).setData(0, key)
                value_string = str(value) if type(value) != dict else str('')
                parent.child(parent.childCount() - 1).setData(1, value_string)
                try:
                    self.create_data(value, indent)
                except RuntimeError:
                    pass

    @staticmethod
    def get_attribute_value(search_string, search_column):
        for childIndex in range(search_column.childCount()):
            child_item = search_column.child(childIndex)
            child_value = child_item.itemData
            if child_value[0] == search_string:
                return child_value[1]
        return None

    def index(self, row, column, index=QModelIndex()):
        """ Returns the index of the item in the model specified by the given row, column and parent index """
        if not self.hasIndex(row, column, index):
            return QModelIndex()
        if not index.isValid():
            item = self.rootItem
        else:
            item = index.internalPointer()

        child = item.child(row)
        if child:
            return self.createIndex(row, column, child)
        return QModelIndex()

    def parent(self, index):
        """
        Returns the parent of the model item with the given index If the item has no parent,
        an invalid QModelIndex is returned
        """
        if not index.isValid():
            return QModelIndex()
        item = index.internalPointer()
        if not item:
            return QModelIndex()

        parent = item.parentItem
        if parent == self.rootItem:
            return QModelIndex()
        else:
            return self.createIndex(parent.childNumber(), 0, parent)

    def rowCount(self, index=QModelIndex()):
        """
        Returns the number of rows under the given parent. When the parent is valid it means that
        rowCount is returning the number of children of parent
        """
        if index.isValid():
            parent = index.internalPointer()
        else:
            parent = self.rootItem
        return parent.childCount()

    def columnCount(self, index=QModelIndex()):
        """ Returns the number of columns for the children of the given parent """
        return self.rootItem.columnCount()

    def data(self, index, role=QtCore.Qt.DisplayRole):
        """ Returns the data stored under the given role for the item referred to by the index """
        if index.isValid() and role == QtCore.Qt.DisplayRole:
            return index.internalPointer().data(index.column())
        elif not index.isValid():
            return self.rootItem.data(index.column())

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        """ Returns the data for the given role and section in the header with the specified orientation """
        if orientation == QtCore.Qt.Horizontal and role == QtCore.Qt.DisplayRole:
            return self.rootItem.data(section)


class TreeNode(object):
    def __init__(self, data, parent=None):
        self.parentItem = parent
        self.itemData = data
        self.children = []

    def child(self, row):
        return self.children[row]

    def childCount(self):
        return len(self.children)

    def childNumber(self):
        if self.parentItem is not None:
            return self.parentItem.children.index(self)

    def columnCount(self):
        return len(self.itemData)

    def data(self, column):
        return self.itemData[column]

    def insertChildren(self, position, count, columns):
        if position < 0 or position > len(self.children):
            return False
        for row in range(count):
            data = [v for v in range(columns)]
            item = TreeNode(data, self)
            self.children.insert(position, item)

    def parent(self):
        return self.parentItem

    def setData(self, column, value):
        if column < 0 or column >= len(self.itemData):
            return False
        self.itemData[column] = value


def delete_instances():
    for obj in mayaMainWindow.children():
        if str(type(obj)) == "<class 'DCC_Materials.maya_materials_export.MayaToLumberyard'>":
            if obj.__class__.__name__ == "MayaToLumberyard":
                obj.setParent(None)
                obj.deleteLater()


def show_ui():
    delete_instances()
    ui = MayaToLumberyard(mayaMainWindow)
    ui.show()

