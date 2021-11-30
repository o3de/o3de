# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
from __future__ import unicode_literals
# from builtins import str
# -- This line is 75 characters -------------------------------------------
# built in's
import os
import sys
import site
import logging as _logging
from collections import OrderedDict
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# normal scripts we would do the following 'from dynaconf import settings'
# for GUI apps that need PySide2 we init in the following manner instead
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
settings = _config.get_config_settings(setup_ly_pyside=True)

from pathlib import Path
from pathlib import PurePath

# Lumberyard extensions
#import azpy
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# set up global space, logging etc.
_DCCSI_GDEBUG = env_bool(ENVAR_DCCSI_GDEBUG, settings.DCCSI_GDEBUG)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, settings.DCCSI_GDEBUG)

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

_MODULENAME = 'DCCsi.SDK.substance.builder.sb_gui_main'

_log_level = _logging.INFO
if _DCCSI_GDEBUG:
    _log_level = _logging.DEBUG

_LOGGER = azpy.initialize_logger(name=_MODULENAME,
                                 log_to_file=True,
                                 default_log_level=_log_level)

_LOGGER.debug('Starting up:  {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# early attach WingIDE debugger (can refactor to include other IDEs later)
if _DCCSI_DEV_MODE:
    from azpy.test.entry_test import connect_wing
    foo = connect_wing()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# now we should be cooking with gas (and can use the dnynamic settings)
# this block is really for standalone
from dynaconf import settings
import config
_LOGGER.debug('config.py is: {}'.format(config))

# initialize the Lumberyard Qt / PySide2
config.init_o3de_pyside(settings.O3DE_DEV)  # for standalone
settings.setenv()  # for standalone

# log debug info about Qt/PySide2
_LOGGER.debug('QTFORPYTHON_PATH: {}'.format(settings.QTFORPYTHON_PATH))
_LOGGER.debug('O3DE_BIN_PATH: {}'.format(settings.O3DE_BIN_PATH))
_LOGGER.debug('QT_PLUGIN_PATH: {}'.format(settings.QT_PLUGIN_PATH))
_LOGGER.debug('QT_QPA_PLATFORM_PLUGIN_PATH: {}'.format(settings.QT_QPA_PLATFORM_PLUGIN_PATH))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# Qt and Pyside2 imports block
import PySide2
_LOGGER.debug('PySide2 is:'.format(PySide2.__file__))

import PySide2.QtCore
from PySide2 import QtCore, QtWidgets
# from PySide2.QtCore import QTimer
from PySide2.QtCore import QProcess, Signal, Slot, QTextCodec
from PySide2.QtGui import QTextCursor, QColor
from PySide2.QtWidgets import QApplication, QPlainTextEdit
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# substance automation toolkit (aka pysbs)
# To Do: move this into some kind of substance specific configuration
# SDK\substance should probably have it's own .env and settings.py (dynaconf)
# Note: pysbs has it's own installer and it wants to install itself directly
# into a python distribution (assumes vanilla external)
# we don't want to modify lumberyards core distribution
# and we can't ship pusbs anyway, so I used pip install --target
# and installed it into a folder within the substance local installation
# so a data driven configuration needs to allow this to be easily set
# outside of the DCCsi
from azpy.constants import PATH_SAT_INSTALL_PATH
_PYSBS_DIR_PATH = Path(PATH_SAT_INSTALL_PATH).resolve()
site.addsitedir(str(_PYSBS_DIR_PATH))  # 'install' is the folder I created

import pysbs
import pysbs.batchtools as pysbs_batch
import pysbs.context as pysbs_context

# local modules
import sbsar_utils
from atom_material import AtomMaterial
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# To Do: still should manage via dynaconf (dynamic config and settings)
from azpy.constants import ENVAR_O3DE_DEV
_O3DE_DEV = Path(os.getenv(ENVAR_O3DE_DEV, None)).resolve()

from azpy.constants import ENVAR_O3DE_PROJECT
_O3DE_PROJECT = os.getenv(ENVAR_O3DE_PROJECT, None)

from azpy.constants import ENVAR_O3DE_PROJECT_PATH
_O3DE_PROJECT_PATH = Path(os.getenv(ENVAR_O3DE_PROJECT_PATH, None)).resolve()

from azpy.constants import ENVAR_DCCSI_SDK_PATH
_DCCSI_SDK_PATH = Path(os.getenv(ENVAR_DCCSI_SDK_PATH, None)).resolve()

# build some reuseable path parts
_PROJECT_ASSET_PATH = Path(_O3DE_PROJECT_PATH).resolve()
_PROJECT_ASSETS_PATH = Path(_O3DE_PROJECT_PATH, 'Materials').resolve()

# To Do: figure out a proper way to deal with Lumberyard game projects
_GEM_MATPLAY_PATH = Path(_O3DE_DEV, 'Gems', 'AtomContent', 'AtomMaterialPlayground').resolve()
_GEM_ROYALTYFREE = Path(_O3DE_DEV, 'Gems', 'AtomContent', 'RoyaltyFreeAssets').resolve()
_GEM_SUBSOURCELIBRARY = Path(_O3DE_DEV, 'Gems', 'AtomContent', 'SubstanceSourceLibrary').resolve()
_SUB_LIBRARY_PATH = Path(_GEM_SUBSOURCELIBRARY, 'Assets', 'SubstanceSource', 'Library').resolve()
# ^ This hard codes a bunch of known asset gems, again bad
# To Do: figure out a proper way to scrap the gem registry from project

# path to watcher script
_WATCHER_SCRIPT_PATH = Path(_DCCSI_SDK_PATH, 'substance', 'builder', 'watchdog', '__init__.py').resolve()

_TEX_RNDR_PATH = Path(_O3DE_PROJECT_PATH, 'Materials', 'Substance').resolve()
_MAT_OUTPUT_PATH = Path(_O3DE_PROJECT_PATH, 'Materials', 'Substance').resolve()
_SBSAR_COOK_PATH = Path(_O3DE_PROJECT_PATH, 'Materials', 'Substance').resolve()
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# sys.path.insert(0, os.path.abspath('../ui/'))
# import main_win
# sys.path.insert(0, os.path.abspath('..'))
# from watchdog import *
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class Window(QtWidgets.QDialog):
    def __init__(self, parent=None, project_path=None, default_material_path=None):
        super(Window, self).__init__(parent)

        # we should really init non-Qt stuff and set things up as properties
        if project_path is None:
            self.project_path = str(_O3DE_PROJECT_PATH)
        else:
            self.project_path = Path(project_path)

        if default_material_path is None:
            self._default_material_path = Path(_DCCSIG_PATH,
                                               'sdk',
                                               'substance',
                                               'resources',
                                               'atom',
                                               'StandardPBR_AllProperties.material')
        else:
            self._default_material_path = default_material_path

        # the start building up qt specific ui, etc
        # but really, much of this should be moved to a self.setup_ui() method
        self.root_directory = str(_PROJECT_ASSET_PATH)
        # os.chdir(self.root_directory)  # change working dir path

        self.browseButton = self.create_button("&Browse...", self.browse)
        self.browseButton1 = self.create_button("&Browse...", self.browse)
        self.findButton = self.create_button("&Find", self.find)
        self.removeButton = self.create_button("&Remove", self.remove)
        self.startWatch = self.create_button("&Start Watch", self.watch)
        self.killButton = self.create_button("&Stop Watch", self.kill)
        self.clearlogButton = self.create_button("&Clear Log", self.clear_log)
        self.atomMaterial = self.create_button("&Atom Material", self.atom_material)

        self.substance_ext_filters = ["*.sbs", "*.sbsar"]

        self.textComboBox = self.createComboBox()
        # ^^ this is never added to a layout ... it's a phatom text entry that is empty
        # it is never updated to add text
        # but it's contents are retreived for use in self.find()

        self.atomMatNameComboBox = self.createComboBox("Setup Material name here(default:sbsar name)")

        # _texturePath = Path(self.project_path, 'Assets', 'SubstanceLibrary', 'textures').resolve()
        self.texRenderPathComboBox = self.createComboBox(str(_TEX_RNDR_PATH))
        self.matOutputPathComboBox = self.createComboBox(str(_MAT_OUTPUT_PATH))

        # self.directoryComboBox = self.createComboBox(QtCore.QDir.currentPath())
        #  I changed this to scan the _O3DE_PROJECT
        # self.sbsarDirectory = self.return_1st_sbsar(Path(self.project_path, 'Assets')).resolve().parent
        self.sbsarDirectory = QtCore.QDir()
        self.sbsarDirectory.setCurrent(str(_PROJECT_ASSET_PATH))
        self.sbsarDirectory.setNameFilters(self.substance_ext_filters)

        self.directoryComboBox = self.createComboBox(self.sbsarDirectory.canonicalPath())
        self.builderDirComboBox = self.createComboBox(self.sbsarDirectory.canonicalPath())

        # I DO NOT like or want all of this ui creation here in the __init__,
        # it is literally no better then a .ui file
        # Rob G is wrong, except he is right if we are building with custom widgets
        # but almost nothing here is custom so it's waste of lines of code
        # extremely difficult to iterate on ... I feel this contributes to
        # the length of time spent to stand this tool up
        # textLabel = QtWidgets.QLabel("Containing text:")
        self.directoryLabel = QtWidgets.QLabel("Watch directory:")
        self.builderdirLabel = QtWidgets.QLabel("Watch directory:")
        self.filesFoundLabel = QtWidgets.QLabel()
        self.watcherLogLabel = QtWidgets.QLabel("Watcher Log")
        self.fileListLabel = QtWidgets.QLabel("File List")
        self.sbsarPresetLabel = QtWidgets.QLabel("Sbsar Preset:")
        self.OutputResLabel = QtWidgets.QLabel("Output Size:")
        self.OutputsLabel = QtWidgets.QLabel("Outputs:")
        self.atomMatLabel = QtWidgets.QLabel("Material  Name: ")
        self.atomMatDirLabel = QtWidgets.QLabel("Material  Path  : ")
        self.textOutputPathLabel = QtWidgets.QLabel("Texture Output:")
        self.randomSeedLabel = QtWidgets.QLabel("Random Seed:")

        self.watcher_log = QPlainTextEdit()
        self.watcher_log.setReadOnly(True)
        # self.watcher_log.setFixedHeight(500)
        self.watcher_log.setMaximumBlockCount(10000)  # limit console to 10000 lines
        self.watcher_log._cursor_output = self.watcher_log.textCursor()

        self.create_files_table()
        buttonsLayout = QtWidgets.QHBoxLayout()
        buttonsLayout.addStretch()
        # buttonsLayout.addWidget(self.atomMaterial)
        # buttonsLayout.addSpacing(50)
        buttonsLayout.addWidget(self.findButton)
        # buttonsLayout.addWidget(self.removeButton)
        watcherBtnLayout = QtWidgets.QHBoxLayout()
        watcherBtnLayout.addStretch()
        watcherBtnLayout.addWidget(self.startWatch)
        watcherBtnLayout.addWidget(self.killButton)
        watcherBtnLayout.addWidget(self.clearlogButton)

        self.setObjectName("Dialog")
        self.gridLayout = QtWidgets.QGridLayout(self)
        self.gridLayout.setObjectName("gridLayout1_0")
        self.tabLayout = QtWidgets.QTabWidget(self)
        self.tabLayout.setObjectName("tabLayout")
        self.tabBuilder = QtWidgets.QTabWidget()
        self.tabBuilder.setObjectName("tabBuilder")
        self.gridLayout1_1 = QtWidgets.QGridLayout(self.tabBuilder)
        self.gridLayout1_1.setObjectName("gridLayout1_1")
        self.builderLayout = QtWidgets.QGridLayout()
        self.builderLayout.setObjectName("builderLayout")

#         self.extensionLabel = QtWidgets.QLabel("Watch extensions:")
#         self.builderLayout.addWidget(self.extensionLabel, 0, 0, 1, 1)
#         self.extFilterComboBox = self.createComboBox(str(self.substance_ext_filters))
#         self.builderLayout.addWidget(self.extFilterComboBox, 0, 1, 1, 2)

        # filename for search
        self.filenameSearchLabel = QtWidgets.QLabel("Filename search:")
        self.builderLayout.addWidget(self.filenameSearchLabel, 0, 0, 1, 1)
        self.filenameSearchComboBox = self.createComboBox(None)
        self.builderLayout.addWidget(self.filenameSearchComboBox, 0, 1, 1, 2)

        self.builderLayout.addWidget(self.builderdirLabel, 1, 0)
        self.builderLayout.addWidget(self.builderDirComboBox, 1, 1, 1, 1)
        self.builderLayout.addWidget(self.browseButton1, 1, 2, 1, 1)
        self.verticalLayout = QtWidgets.QVBoxLayout()
        self.verticalLayout.addWidget(self.filesTable)
        self.verticalLayout.addWidget(self.filesFoundLabel)
        self.builderLayout.addLayout(self.verticalLayout, 2, 0, 1, 3)
        self.builderLayout.addLayout(buttonsLayout, 4, 0, 1, 3)
        self.inputGroupBox = QtWidgets.QGroupBox("Input Parameters")
        self.inputGridLayout = QtWidgets.QGridLayout(self.inputGroupBox)
        self.inputGridLayout.addWidget(self.randomSeedLabel, 2, 0, 1, 1)
        self.randomSeedComboBox = self.createComboBox("0000000000")
        self.inputGridLayout.addWidget(self.randomSeedComboBox, 2, 1, 1, 2)
        self.randomSeedBtn = self.create_button("&Generate", self.randomseed)
        self.inputGridLayout.addWidget(self.randomSeedBtn, 2, 3, 1, 1)
        self.randomSeedComboBox.setDisabled(True)
        self.gridLayout1_1.addLayout(self.builderLayout, 0, 0, 1, 1)
        self.tabLayout.addTab(self.tabBuilder, "Builder")

        self.tabWatcher = QtWidgets.QTabWidget()
        self.tabWatcher.setObjectName("tabWatcher")
        self.gridLayout2_1 = QtWidgets.QGridLayout(self.tabWatcher)
        self.gridLayout2_1.setObjectName("gridLayout2_0")
        self.watcherLayout = QtWidgets.QGridLayout()
        self.watcherLayout.setObjectName("watcherLayout")
        self.watcherLayout.addWidget(self.directoryLabel, 0, 0)
        self.watcherLayout.addWidget(self.directoryComboBox, 0, 1)
        self.watcherLayout.addWidget(self.browseButton, 0, 2)
        self.watcherHBoxLayout = QtWidgets.QHBoxLayout()
        self.watcherHBoxLayout.addWidget(self.watcher_log)
        self.watcherLayout.addLayout(self.watcherHBoxLayout, 1, 0, 1, 3)
        self.watcherLayout.addLayout(watcherBtnLayout, 2, 0, 1, 3)
        self.gridLayout2_1.addLayout(self.watcherLayout, 0, 0, 1, 1)
        self.tabLayout.addTab(self.tabWatcher, "Watcher")
        self.gridLayout.addWidget(self.tabLayout)
        # self.setLayout(self.watcherLayout)

        self.setWindowTitle("Substance Builder")
        self.resize(800, 600)
        # self.path = self.directoryComboBox.currentText()
        self.path = Path(self.project_path)
        self.groupboxOutputs = QtWidgets.QGroupBox("Outputs")
        self.groupboxMat = QtWidgets.QGroupBox("Atom Material && Textures")
        self.watcher_script = _WATCHER_SCRIPT_PATH
        self.outputRenderPath = _TEX_RNDR_PATH
        self.preset = ''
        self.presetItem = ''
        self.texRenderpath = self.texRenderPathComboBox.currentText()
        self.res = 11
        self.output_name = '{inputGraphUrl}_{outputNodeName}'
        self.randomstr = 1
        self.preset_index = -1
        self.outputCookPath = _SBSAR_COOK_PATH
        self.selected_tex = []
        self.material_path = _MAT_OUTPUT_PATH

        self._reader = self.start_reader()

    def start_reader(self):
        # set up
        self._reader = ProcessOutputReader()
        self._reader.produce_output.connect(self.append_output)
        self._reader.start('python', ['-u', self.watcher_script, self.sbsarDirectory])
        return self._reader

    @Slot(str)
    def append_output(self, text):
        self.watcher_log._cursor_output.insertText(text)
        self.scroll_to_last_line()

    def scroll_to_last_line(self):
        cursor = self.watcher_log.textCursor()
        cursor.movePosition(QTextCursor.End)
        cursor.movePosition(QTextCursor.Up if cursor.atBlockStart() else
                            QTextCursor.StartOfLine)
        self.watcher_log.setTextCursor(cursor)

    def output_text(self, text):
        self.watcher_log._cursor_output.insertText(text)
        self.watcher_log.scroll_to_last_line()

    def browse(self):
        self.root_directory = QtWidgets.QFileDialog.getExistingDirectory(self,
                                                                         "Find watcher folder",
                                                                         str(_SUB_LIBRARY_PATH))
        if self.root_directory:
            if self.directoryComboBox.findText(self.root_directory) or self.builderDirComboBox.findText(
                    self.root_directory) == -1:
                self.directoryComboBox.addItem(self.root_directory)
                self.builderDirComboBox.addItem(self.root_directory)

            self.directoryComboBox.setCurrentIndex(self.directoryComboBox.findText(self.root_directory))
            self.builderDirComboBox.setCurrentIndex(self.builderDirComboBox.findText(self.root_directory))

        self.path = self.root_directory
        self.find()
        return self.path

    def browseRenderPath(self):
        self.texRenderpath = QtWidgets.QFileDialog.getExistingDirectory(self,
                                                                        "Setup textures output path",
                                                                        str(_TEX_RNDR_PATH))
        if self.texRenderpath:
            if self.texRenderPathComboBox.findText(self.texRenderpath) == -1:
                self.texRenderPathComboBox.addItem(self.texRenderpath)

            self.texRenderPathComboBox.setCurrentIndex(self.texRenderPathComboBox.findText(self.texRenderpath))
        self.texRenderPathComboBox.setCurrentText(self.texRenderpath)
        return self.texRenderpath

    def browseMaterialPath(self):
        self.material_path = QtWidgets.QFileDialog.getExistingDirectory(self,
                                                                        "Setup Atom Material output path",
                                                                        str(_MAT_OUTPUT_PATH))
        if self.material_path:
            if self.matOutputPathComboBox.findText(self.material_path) == -1:
                self.matOutputPathComboBox.addItem(self.material_path)

            self.matOutputPathComboBox.setCurrentIndex(self.matOutputPathComboBox.findText(self.material_path))
        self.matOutputPathComboBox.setCurrentText(self.material_path)
        return self.material_path

    @staticmethod
    def updateComboBox(comboBox):
        if comboBox.findText(comboBox.currentText()) == -1:
            comboBox.addItem(comboBox.currentText())

    def find(self, rootpath=None, filename=None, FILE_TEMPLATES=['*.sbs', '*.sbsar']):
        self.filesTable.setRowCount(0)

        if not filename:
            fileName = self.filenameSearchComboBox.currentText()
            # ^ this looks like it is meant to either be a file name pattern for search
            # or ext patterns?  Or both?

        text = self.textComboBox.currentText()
        # ^ what is this even for???
        # it never gets filled out but we continue to pass it on?
        # I assume it's really meant to be a file name and file ext pattern search?

        if rootpath:
            self.path = rootpath
        else:
            self.path = self.builderDirComboBox.currentText()

        self.directoryComboBox.setCurrentText(self.path)
        # self.path = "C:/Users/chunghao/Documents/Allegorithmic/Substance Designer/sbsar"

        self.updateComboBox(self.filenameSearchComboBox)
        self.updateComboBox(self.textComboBox)
        self.updateComboBox(self.directoryComboBox)

        self.currentDir = QtCore.QDir(str(Path(self.path).resolve()))

        dirModel = QtWidgets.QFileSystemModel()
        dirModel.setRootPath(self.currentDir.canonicalPath())
        dirModel.setFilter(QtCore.QDir.NoDotAndDotDot | QtCore.QDir.Dirs)
        if fileName:
            FILE_TEMPLATES = [str(fileName)]
        dirModel.setNameFilters(FILE_TEMPLATES)

        # right now this filters to *.sbs and *.sbsar
        # we use the project\assets dir as the root and query the filesystem
        # instead of only a single directory
        iterator = QtCore.QDirIterator(self.currentDir.canonicalPath(),
                                       FILE_TEMPLATES,
                                       QtCore.QDir.Files,
                                       QtCore.QDirIterator.Subdirectories)

        # now we can iterate over that filtered filesystem
        files = []
        while iterator.hasNext():
            files.append(iterator.next())
        files.sort()

#         if not fileName or filename == '':
#             fileName = "*"
#         files = self.currentDir.entryList([fileName],
#                                           QtCore.QDir.Files | QtCore.QDir.NoSymLinks)

        files = self.find_files(files, text)
        self.show_files(files)
        return self.path

    def find_files(self, files, text):
        progressDialog = QtWidgets.QProgressDialog(self)

        progressDialog.setCancelButtonText("&Cancel")
        progressDialog.setRange(0, len(files))
        progressDialog.setWindowTitle("Find Files")

        foundFiles = []

        for i in range(len(files)):
            progressDialog.setValue(i)
            progressDialog.setLabelText("Searching file number %d of %d..." % (i, len(files)))
            QtCore.qApp.processEvents()

            if progressDialog.wasCanceled():
                break

            inFile = QtCore.QFile(self.currentDir.absoluteFilePath(files[i]))

            if inFile.open(QtCore.QIODevice.ReadOnly):
                stream = QtCore.QTextStream(inFile)
                while not stream.atEnd():
                    if progressDialog.wasCanceled():
                        break
                    line = stream.readLine()
                    if text in line:
                        foundFiles.append(files[i])
                        break

        progressDialog.close()

        return foundFiles

    def show_files(self, files):
        for fn in files:
            file = QtCore.QFile(self.currentDir.absoluteFilePath(fn))
            size = QtCore.QFileInfo(file).size()

            fileNameItem = QtWidgets.QTableWidgetItem(fn)
            fileNameItem.setFlags(fileNameItem.flags() ^ QtCore.Qt.ItemIsEditable)

            sizeItem = QtWidgets.QTableWidgetItem("%d KB" % (int((size + 1023) / 1024)))
            sizeItem.setTextAlignment(QtCore.Qt.AlignVCenter | QtCore.Qt.AlignRight)
            sizeItem.setFlags(sizeItem.flags() ^ QtCore.Qt.ItemIsEditable)
            row = self.filesTable.rowCount()
            self.filesTable.insertRow(row)
            self.filesTable.setItem(row, 0, fileNameItem)
            self.filesTable.setItem(row, 1, sizeItem)
            self.filesTable.setAlternatingRowColors(True)
            self.filesTable.setStyleSheet("alternate-background-color: #444444; background-color: #4d4d4d;")
        self.filesFoundLabel.setText(
            "%d file(s) found (Double click on a SBSAR to cook, SBS file to load.)" % len(files))

    def create_button(self, text, member):
        button = QtWidgets.QPushButton(text)
        button.clicked.connect(member)
        return button

    def createComboBox(self, text=""):
        comboBox = QtWidgets.QComboBox()
        comboBox.setEditable(True)
        comboBox.addItem(text)
        comboBox.setSizePolicy(QtWidgets.QSizePolicy.Expanding,
                               QtWidgets.QSizePolicy.Preferred)
        return comboBox

    def create_files_table(self):
        self.filesTable = QtWidgets.QTableWidget(0, 2)
        # self.filesTable.setMinimumHeight(300)
        # self.filesTable.setFixedHeight(300)
        self.filesTable.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.filesTable.setHorizontalHeaderLabels(("File Name", "Size"))
        self.filesTable.horizontalHeaderItem(0).setTextAlignment(QtCore.Qt.AlignVCenter | QtCore.Qt.AlignLeft)
        self.filesTable.horizontalHeaderItem(1).setTextAlignment(QtCore.Qt.AlignVCenter | QtCore.Qt.AlignRight)
        # self.filesTable.horizontalHeader().setDefaultAlignment(QtCore.Qt.AlignRight)
        self.filesTable.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        self.filesTable.horizontalHeader().setDefaultSectionSize(5)
        self.filesTable.verticalHeader().hide()
        # self.filesTable.verticalHeader().show()
        self.filesTable.setColumnWidth(1, 80)
        self.filesTable.setShowGrid(False)
        self.filesTable.verticalHeader().setDefaultSectionSize(5)
        # style = "::section {""background-color: (241, 255, 175); }"
        # self.filesTable.setStyleSheet(style)
        # self.filesTable.setFixedHeight(200)
        self.filesTable.cellActivated.connect(self.open_file_of_item)

    def create_sbsar_tex(self):
        _LOGGER.debug('Running: .create_sbsar_tex')
        # self.builderLayout.addWidget(self.OutputsLabel, 10, 0)
        self.sbsarName = self.item.text().replace(".sbsar", "")
        self.groupboxOutputs = QtWidgets.QGroupBox("Output Texture Maps")
        self.horizontalLayoutOuputs = QtWidgets.QHBoxLayout(self.groupboxOutputs)
        self.horizontalLayoutOuputs.setObjectName(("horizontalLayoutOuputs"))
        # self.vertical_layout_outputs = QtWidgets.QVBoxLayout(self.groupboxOutputs)
        # self.vertical_layout_outputs.setObjectName("vertical_layout_outputs")
        self.sbsar_tex_outputs = sbsar_utils.output_info(self.path, self.sbsarName, 'tex_maps')

        for index, tex in enumerate(self.sbsar_tex_outputs):
            _LOGGER.info(tex)
            #text_item_checkbox = QtWidgets.QCheckBox(self.groupboxOutputs)
            #text_item_checkbox.setObjectName(self.sbsarName + "_" + tex)
            # text_item_checkbox.setEnabled(True)
            # text_item_checkbox.setChecked(True)
            # text_item_checkbox.setText(tex)
            # self.horizontalLayoutOuputs.addWidget(text_item_checkbox)

            self.sbsar_tex_outputs[index] = QtWidgets.QCheckBox(self.groupboxOutputs)
            self.sbsar_tex_outputs[index].setObjectName(self.sbsarName + "_" + tex)
            self.sbsar_tex_outputs[index].setEnabled(True)
            self.sbsar_tex_outputs[index].setChecked(True)
            self.sbsar_tex_outputs[index].setText(tex)
            self.horizontalLayoutOuputs.addWidget(self.sbsar_tex_outputs[index])

        self.builderLayout.addWidget(self.groupboxOutputs, 7, 0, 1, 3)

    def create_sbsar_presets(self):
        _LOGGER.debug('Running: .create_sbsar_presets')
        self.inputGridLayout.addWidget(self.sbsarPresetLabel, 0, 0, 1, 1)
        self.sbsar_presets = sbsar_utils.output_info(self.path, self.item.text().replace(".sbsar", ""), 'presets')
        self.comboBox_SbsarPreset = QtWidgets.QComboBox(self)
        self.comboBox_SbsarPreset.addItem("Default preset")
        for self.presetItem in self.sbsar_presets:
            _LOGGER.info(self.presetItem)
            # preset_index = 0
            self.comboBox_SbsarPreset.addItem(self.presetItem, self.presetItem)
            # self.comboBox_SbsarPreset.setItemText(preset_index, self.presetItem)
            # preset_index += 1

        self.inputGridLayout.addWidget(self.comboBox_SbsarPreset, 0, 1, 1, 3)
        self.comboBox_SbsarPreset.activated.connect(self.preset_selected)

    def preset_selected(self, index):
        self.preset = self.comboBox_SbsarPreset.itemData(index)
        if self.preset in self.sbsar_presets:
            self.atomMatNameComboBox.setCurrentText(self.sbsarName + "_" + self.preset.replace(" ", ""))
            _LOGGER.info('Preset(variant): ' + self.atomMatNameComboBox.currentText())
            self.preset_index = index - 1
            _LOGGER.info('The Preset index: ' + str(self.preset_index))
            return self.preset_index
            # return self.atomMatNameComboBox.currentText()
        else:
            self.atomMatNameComboBox.setCurrentText(self.sbsarName)
            _LOGGER.info('Default preset selected.')
            self.preset_index = index - 1
            _LOGGER.info(self.preset_index)
            return self.preset_index

    def create_tex_res(self):
        self.inputGridLayout.addWidget(self.OutputResLabel, 1, 0, 1, 1)
        self.resSelector = QtWidgets.QComboBox(self)
        # self.resSelector.addItem("512x512", "$outputsize@9,9")
        # self.resSelector.addItem("1024x1024", "$outputsize@10,10")
        # self.resSelector.addItem("2048x2048", "$outputsize@11,11")
        self.resSelector.addItem("512x512", 9)
        self.resSelector.addItem("1024x1024", 10)
        self.resSelector.addItem("2048x2048", 11)
        self.resSelector.addItem("4096x4096", 12)
        self.resSelector.addItem("8096x8096", 13)
        self.inputGridLayout.addWidget(self.resSelector, 1, 1, 1, 3)
        self.resSelector.activated.connect(self.res_selected)
        self.resSelector.setCurrentIndex(2)

    def res_selected(self, index):
        self.res = self.resSelector.itemData(index)
        _LOGGER.info(self.res)
        return self.res

    def remove(self):
        self.builderLayout.removeWidget(self.groupboxOutputs)
        self.groupboxOutputs.close()

    def kill(self):
        self._reader.kill()

    def clear_log(self):
        self.watcher_log.clear()

    def atom_material(self, preset=None,
                      material_type=None,
                      tex_ext='.tif',
                      mat_ext='.material'):
        """To Do"""
        if preset is None:
            preset = self.preset

        preset = str(preset).replace(" ", "-")

        # setup material template
        if material_type is None:
            material = AtomMaterial(self._default_material_path)

        mat_file_tag = str(self.atomMatNameComboBox.currentText()).replace(" ", "_")

        # file_path = Path(self.atomMatNameComboBox.currentText())
        # file_name = file_path.stem
        # ^^ object oriented paths do all these things you want without destrpying the path
        # write less code, use your autocomplete, reed the docs
        # p.name, Final component only (name.ext)
        # p.stem, Final component without extension.
        # p.ext, Extension only. .suffix in pathlib

        # if you want relative paths here is a better way
        # first of all, assume we know the project we are in
        #_O3DE_PROJECT_PATH

        texture_output_path = Path(self.texRenderPathComboBox.currentText()).resolve()
        rel_tex_path = None
        for p in texture_output_path.parts:
            if _O3DE_PROJECT == p:
                index = texture_output_path.parts.index(_O3DE_PROJECT)
                rel_tuple = texture_output_path.parts[index + 1:]
                rel_tex_path = Path(*list(rel_tuple))

        # if 'ambientocclusion' in self.preset:
        #     num_index = 5
        # else:
        num_index = 5
        #  ^^ ok what is all this magic mumbo jumbo?

        if preset is None or preset == '':
            preset_slug = preset  # gets rid of _ when not needed
        else:
            preset_slug = '_' + preset

        for tex_index in range(num_index):
            # tex_map_name = material.texture_map[material.tex[tex_index]] + tex_ext
            # ^ this is not so great, AtomMaterial is very rigid and only very specifically
            # supports a hand coded implmentation of StandardPBR
            # we should have a more generic material class that can consume any
            # then we can somehow check for the type we are compatible with
            # after that you might want a wrapper class above the atom material data that knows how
            # to map from substance to the atom material, and builds a specific data structure above
            #  the generic one

            material_texture_slot = material.texture_map[material.tex[tex_index]]
            material_slot_texture = material_texture_slot + tex_ext

            # text_map_path = Path(rel_tex_path + "\\" + tex_file_name + "_" + tex_map_name).resolve()
            texture_slug = mat_file_tag + preset_slug + '_' + material_slot_texture
            rel_texture_map_path = Path(rel_tex_path, texture_slug)  # don't .resolve()

            # posiz / slash
            material.set_map(material.tex[tex_index], str(rel_texture_map_path.as_posix()))

        # mat_path = self.material_path + "/" + self.atomMatNameComboBox.currentText().replace(" ", "") + ".material"

        material_path = Path(self.material_path, mat_file_tag + preset_slug + mat_ext)
        material.write(material_path)

        self.preset = ''

        _LOGGER.info('{}'.format(material_path))

        return material_path

    def create_single(self):
        if self.preset is None:
            self.atom_material(self.preset)
        else:
            self.atom_material(self.preset)

    def create_all_material(self):
        #  always make the make
        # self.atom_material('base')
        self.atom_material(self.preset)

        # then make any others
        if len(self.sbsar_presets) > 0:
            for i in self.sbsar_presets:
                _LOGGER.info('Preset(variant): {}'.format(i.replace(" ", "")))
                self.atom_material(i)

    def print_something(self, words):
        self.words = words
        _LOGGER.info(self.words)

    def watch(self):
        self._reader.start('python', ['-u', self.watcher_script, self.sbsarDirectory])

    def create_atom_mat(self, item_path=None):
        self.atom_matPath_button = self.create_button("&Browse", self.browseMaterialPath)
        self.atom_material_button = self.create_button("&Create Single", self.create_single)
        self.atom_material_output = self.create_button("&Create All", self.create_all_material)
        self.renderPathbutton = self.create_button("&Output Path", self.browseRenderPath)
        self.rendeTexbutton = self.create_button("&Render Textures", self.render_tex)

        if item_path is None:
            item_path = Path(self.item.text()).resolve()

        # self.sbsarName = self.item.text().replace(".sbsar", "")
        self.sbsarName = item_path.stem
        self.groupboxMat = QtWidgets.QGroupBox("Atom Material")
        self.atomMatNameComboBox.setCurrentText(self.sbsarName)
        # self.atomMatNameComboBox.setDisabled(True)
        # self.texRenderPathComboBox.setCurrentText(self.path)
        self.gridLayoutMat = QtWidgets.QGridLayout(self.groupboxMat)
        self.MatHBox0 = QtWidgets.QHBoxLayout()
        self.MatHBox0.addWidget(self.atomMatDirLabel)
        self.MatHBox0.addWidget(self.matOutputPathComboBox)
        self.MatHBox0.addWidget(self.atom_matPath_button)
        # self.MatHBox0.addWidget(self.atom_matPath_button2)
        self.gridLayoutMat.addLayout(self.MatHBox0, 0, 0, 1, 4)
        self.MatHBox = QtWidgets.QHBoxLayout()
        self.MatHBox.addWidget(self.atomMatLabel)
        self.MatHBox.addWidget(self.atomMatNameComboBox)
        self.MatHBox.addWidget(self.atom_material_output)
        self.MatHBox.addWidget(self.atom_material_button)
        self.gridLayoutMat.addLayout(self.MatHBox, 1, 0, 1, 4)
        self.MatHBox1 = QtWidgets.QHBoxLayout()
        self.MatHBox1.addWidget(self.textOutputPathLabel)
        self.MatHBox1.addWidget(self.texRenderPathComboBox)
        self.MatHBox1.addWidget(self.renderPathbutton)
        self.MatHBox1.addWidget(self.rendeTexbutton)
        self.gridLayoutMat.addLayout(self.MatHBox1, 2, 0, 1, 4)

        self.builderLayout.addWidget(self.groupboxMat, 8, 0, 1, 3)
        # _LOG.info("\n")

    def randomseed(self):
        import random
        random_num = random.randrange(1, 10 ** 10)
        self.randomstr = str(random_num)
        self.randomSeedComboBox.setCurrentText(str(random_num))
        _LOGGER.info('Random Seed: ' + self.randomstr)
        return self.randomstr

    def render_tex(self):
        """To Do"""

        texture_output_path = Path(self.texRenderPathComboBox.currentText()).resolve()

        sbsar_item_path = Path(self.item.text()).resolve()

        if not texture_output_path.exists():
            try:
                texture_output_path.mkdir(mode=0o777)
                _LOGGER.info('mkdir: {}'.format(texture_output_path))
            except Exception as e:
                _LOGGER.ERROR(e)
                raise(e)
        else:
            _LOGGER.info('exists: {}'.format(texture_output_path))

        _LOGGER.info('Render texture from: ' + self.path + "/" + self.item.text())

        # for index, tex in enumerate(self.groupboxOutputs.children()):
            # if self.groupboxOutputs.children()

        for index, tex in enumerate(self.sbsar_tex_outputs):
            if self.sbsar_tex_outputs[index].isChecked() is True:
                self.selected_tex.append(tex.text())
        print(self.selected_tex)

        self.outputCookPath = sbsar_item_path.parent

        sbsar_utils.render_sbsar(self.outputCookPath,
                                 self.selected_tex,
                                 sbsar_item_path.stem,
                                 texture_output_path,
                                 self.preset_index,
                                 self.res,
                                 self.randomstr)
        self.selected_tex = []

    def find_1st_sbsar(self, seedpath=None):
        i = None  # previously broke if found none
        
        if seedpath is None:
            seedpath = Path(self.project_path, 'Assets')

        filelist = []
        for r, d, f in os.walk(seedpath):
            for file in f:
                filelist.append(file)
        for i in range(len(filelist)):
            if filelist[i].split(".")[-1] == "sbsar":
                return i
            else:
                i += 1
        return i

    def return_1st_sbsar(self, seedpath=None):
        if seedpath is None:
            seedpath = Path(self.project_path, 'Assets')

        filelist = []
        for r, d, f in os.walk(seedpath):
            for file in f:
                if Path(file).ext == ".sbsar":
                    filelist.append(Path(r, file).absolute())

        return filelist[0]

    def open_file_of_item(self, row, column):
        self.item = self.filesTable.item(row, 0)
        item_path = Path(self.item.text()).resolve()
        _LOGGER.debug('Item path: {}'.format(item_path))

        # Load sbsar parameters.
        if PurePath(item_path).suffix == ".sbsar":
            self.create_sbsar_presets()
            self.create_tex_res()
            self.builderLayout.removeWidget(self.groupboxOutputs)
            self.groupboxOutputs.close()
            self.create_sbsar_tex()
            self.builderLayout.removeWidget(self.groupboxMat)
            self.groupboxMat.close()
            self.builderLayout.addWidget(self.inputGroupBox, 5, 0, 1, 3)
            self.randomSeedComboBox.setCurrentText("0000000000")
            self.create_atom_mat(item_path)

        # Cook sbsar file and trigger watcher to generate textures.
        elif self.item.text().split(".")[-1] == "sbs":
            sbsar_utils.cook_sbsar(self.path + "/" + self.item.text(), sbsar_utils._PYSBS_CONTEXT, self.path,
                                   self.item.text().split(".")[0])
            sbsar_utils.output_info()


class ProcessOutputReader(QProcess):
    produce_output = Signal(str)

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        # merge stderr channel into stdout channel
        self.setProcessChannelMode(QProcess.MergedChannels)
        # prepare decoding process' output to Unicode
        self._codec = QTextCodec.codecForLocale()
        self._decoder_stdout = self._codec.makeDecoder()
        # only necessary when stderr channel isn't merged into stdout:
        # self._decoder_stderr = codec.makeDecoder()

        self.readyReadStandardOutput.connect(self._ready_read_standard_output)
        # only necessary when stderr channel isn't merged into stdout:
        # self.readyReadStandardError.connect(self._ready_read_standard_error)

    @Slot()
    def _ready_read_standard_output(self):
        raw_bytes = self.readAllStandardOutput()
        text = self._decoder_stdout.toUnicode(raw_bytes)
        self.produce_output.emit(text)

    # only necessary when stderr channel isn't merged into stdout:
    # @Slot()
    # def _ready_read_standard_error(self):
    #     raw_bytes = self.readAllStandardError()
    #     text = self._decoder_stderr.toUnicode(raw_bytes)
    #     self.produce_output.emit(text)


def substance_builder_launcher():

    app = QApplication(sys.argv)
    window = Window()

    _LOGGER.info('cwd: {}'.format(os.getcwd()))  # project
    _LOGGER.info('file: {}'.format(__file__))     # *might* come back relative

    # we should ensure we know the abs path
    qss_filepath = Path(_DCCSIG_PATH).resolve().absolute()
    qss_filepath = Path(qss_filepath, 'SDK', 'substance', 'builder',
                        'ui', 'stylesheets', 'LYstyle.qss').resolve().absolute()
    window.setStyleSheet(qss_filepath.read_text())
    # window.find()  # uhm ... this is VERY odd, shouldn't you do this in __init__?
    window.find()
    first_item = window.find_1st_sbsar()
    window.show()
    # window.open_file_of_item(first_item, 0)
    sys.exit(app.exec_())


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

    _LOGGER.info("Test Run:: {0}.".format({_MODULENAME}))
    _LOGGER.info("{0} :: if __name__ == '__main__':".format(_MODULENAME))

    _LOGGER.info('file: {}'.format(os.getcwd()))  # project
    _LOGGER.info('file: {}'.format(__file__))     # *might* come back relative

    substance_builder_launcher()
