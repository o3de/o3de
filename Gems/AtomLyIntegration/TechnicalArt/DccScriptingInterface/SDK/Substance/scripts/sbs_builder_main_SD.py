# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

import sys
import os
from PySide2 import QtCore, QtWidgets
# from PySide2.QtCore import QTimer
from PySide2.QtCore import QProcess, Signal, Slot, QTextCodec
from PySide2.QtGui import QTextCursor, QColor
from PySide2.QtWidgets import QApplication, QPlainTextEdit
import pysbs.batchtools as sbs
from LyPy.si_substance import builder
from LyPy.si_substance.builder.atom_material import AtomMaterial

# sys.path.insert(0, os.path.abspath('../ui/'))
# import main_win
# sys.path.insert(0, os.path.abspath('..'))
# from watchdog import *


class Window(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(Window, self).__init__(parent)
        self.browseButton = self.create_button("&Browse...", self.browse)
        self.browseButton1 = self.create_button("&Browse...", self.browse)
        self.findButton = self.create_button("&Find", self.find)
        self.removeButton = self.create_button("&Remove", self.remove)
        self.startWatch = self.create_button("&Start Watch", self.watch)
        self.killButton = self.create_button("&Stop Watch", self.kill)
        self.clearlogButton = self.create_button("&Clear Log", self.clear_log)
        self.atomMaterial = self.create_button("&Atom Material", self.atom_material)
        self.fileComboBox = self.createComboBox("*")
        self.textComboBox = self.createComboBox()
        self.atomMatNameComboBox = self.createComboBox("Setup Material name here(default:sbsar name)")
        self.texRenderPathComboBox = self.createComboBox("C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/"
                                                         "Materials/Substance/textures")
        # self.directoryComboBox = self.createComboBox(QtCore.QDir.currentPath())
        self.sbsarDirectory = "C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/Materials/Substance"
        self.directoryComboBox = self.createComboBox(self.sbsarDirectory)
        self.builderdirComboBox = self.createComboBox(self.sbsarDirectory)
        self.extensionLabel = QtWidgets.QLabel("Watch extensions:")
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
        self.builderLayout.addWidget(self.extensionLabel, 0, 0, 1, 1)
        self.builderLayout.addWidget(self.fileComboBox, 0, 1, 1, 2)
        self.builderLayout.addWidget(self.builderdirLabel, 1, 0)
        self.builderLayout.addWidget(self.builderdirComboBox, 1, 1, 1, 1)
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
        self.path = self.directoryComboBox.currentText()
        self.groupboxOutputs = QtWidgets.QGroupBox("Outputs")
        self.groupboxMat = QtWidgets.QGroupBox("Atom Material && Textures")
        self.watcher_script = "C:/dccapi/dev/Gems/DccScriptingInterface/LyPy/si_substance/builder/watchdog/__init__.py"
        self.outputRenderPath = 'C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/Materials/textures/'
        self.preset = ""
        self.presetItem = ""
        self.texRenderpath = self.texRenderPathComboBox.currentText()
        self.res = "$outputsize@9,9"
        self.output_name = '{inputGraphUrl}_{outputNodeName}'
        self.randomstr = ""

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
        # self.directory = QtWidgets.QFileDialog.getExistingDirectory(self, "Find Files",
        #         QtCore.QDir.currentPath())
        self.directory = QtWidgets.QFileDialog.getExistingDirectory(self, "Find watcher folder",
                            "C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/Materials/Substance")
        if self.directory:
            if self.directoryComboBox.findText(self.directory) or self.builderdirComboBox.findText(self.directory) == -1:
                self.directoryComboBox.addItem(self.directory)
                self.builderdirComboBox.addItem(self.directory)

            self.directoryComboBox.setCurrentIndex(self.directoryComboBox.findText(self.directory))
            self.builderdirComboBox.setCurrentIndex(self.builderdirComboBox.findText(self.directory))

        self.path = self.directory
        self.find()
        return self.path

    def browseRenderPath(self):
        self.texRenderpath = QtWidgets.QFileDialog.getExistingDirectory(self, "Setup textures output path",
                            "C:/dccapi/dev/Gems/DccScriptingInterface/MockProject/Assets/Materials/textures")
        if self.texRenderpath:
            if self.texRenderPathComboBox.findText(self.texRenderpath) == -1:
                self.texRenderPathComboBox.addItem(self.texRenderpath)

            self.texRenderPathComboBox.setCurrentIndex(self.texRenderPathComboBox.findText(self.texRenderpath))
        self.texRenderPathComboBox.setCurrentText(self.texRenderpath)
        return self.texRenderpath

    @staticmethod
    def updateComboBox(comboBox):
        if comboBox.findText(comboBox.currentText()) == -1:
            comboBox.addItem(comboBox.currentText())

    def find(self):
        self.filesTable.setRowCount(0)

        fileName = self.fileComboBox.currentText()
        text = self.textComboBox.currentText()
        self.path = self.builderdirComboBox.currentText()
        self.directoryComboBox.setCurrentText(self.path)
        # self.path = "C:/Users/chunghao/Documents/Allegorithmic/Substance Designer/sbsar"

        self.updateComboBox(self.fileComboBox)
        self.updateComboBox(self.textComboBox)
        self.updateComboBox(self.directoryComboBox)

        self.currentDir = QtCore.QDir(self.path)
        if not fileName:
            fileName = "*"
        files = self.currentDir.entryList([fileName],
                QtCore.QDir.Files | QtCore.QDir.NoSymLinks)

        if text:
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
        self.filesFoundLabel.setText("%d file(s) found (Double click on a SBSAR to cook, SBS file to load.)" % len(files))

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
        self.filesTable.setShowGrid(True)
        self.filesTable.verticalHeader().setDefaultSectionSize(5)
        # style = "::section {""background-color: (241, 255, 175); }"
        # self.filesTable.setStyleSheet(style)
        # self.filesTable.setFixedHeight(200)
        self.filesTable.cellActivated.connect(self.open_file_of_item)

    def create_sbsar_tex(self):
        # self.builderLayout.addWidget(self.OutputsLabel, 10, 0)
        self.sbsarName = self.item.text().replace(".sbsar", "")
        self.groupboxOutputs = QtWidgets.QGroupBox("Output Texture Maps")
        self.horizontalLayoutOuputs = QtWidgets.QHBoxLayout(self.groupboxOutputs)
        self.horizontalLayoutOuputs.setObjectName(("horizontalLayoutOuputs"))
        # self.vertical_layout_outputs = QtWidgets.QVBoxLayout(self.groupboxOutputs)
        # self.vertical_layout_outputs.setObjectName("vertical_layout_outputs")
        self.sbsar_tex_outputs = builder.output_info(self.path, self.sbsarName)['_outputs']

        for index, tex in enumerate(self.sbsar_tex_outputs):
            print(tex, end=" ")
            self.sbsar_tex_outputs[index] = QtWidgets.QCheckBox(self.groupboxOutputs)
            self.sbsar_tex_outputs[index].setEnabled(False)
            self.sbsar_tex_outputs[index].setChecked(True)
            self.sbsar_tex_outputs[index].setObjectName(self.sbsarName+"_"+tex)
            self.sbsar_tex_outputs[index].setText(tex)
            self.horizontalLayoutOuputs.addWidget(self.sbsar_tex_outputs[index])
        self.builderLayout.addWidget(self.groupboxOutputs, 7, 0, 1, 3)
        print("\n")

    def create_sbsar_presets(self):
        self.inputGridLayout.addWidget(self.sbsarPresetLabel, 0, 0, 1, 1)
        self.sbsar_presets = builder.output_info(self.path, self.item.text().replace(".sbsar", ""))['_presets']
        self.comboBox_SbsarPreset = QtWidgets.QComboBox(self)
        self.comboBox_SbsarPreset.addItem("Please select a preset")
        for self.presetItem in self.sbsar_presets:
            print(self.presetItem, end=", ")
            # preset_index = 0
            self.comboBox_SbsarPreset.addItem(self.presetItem, self.presetItem)
            # self.comboBox_SbsarPreset.setItemText(preset_index, self.presetItem)
            # preset_index += 1

        self.inputGridLayout.addWidget(self.comboBox_SbsarPreset, 0, 1, 1, 3)
        print("\n")
        self.comboBox_SbsarPreset.activated.connect(self.preset_selected)

    def preset_selected(self, index):
        self.preset = self.comboBox_SbsarPreset.itemData(index)
        if self.preset in self.sbsar_presets:
            self.atomMatNameComboBox.setCurrentText(self.sbsarName + "_" + self.preset.replace(" ", ""))
            print(self.atomMatNameComboBox.currentText())
            return self.atomMatNameComboBox.currentText()
            # return self.preset
        else:
            self.atomMatNameComboBox.setCurrentText(self.sbsarName)
            print(self.atomMatNameComboBox.currentText())
            return self.atomMatNameComboBox.currentText()
            # return self.preset

    def create_tex_res(self):
        self.inputGridLayout.addWidget(self.OutputResLabel, 1, 0, 1, 1)
        self.resSelector = QtWidgets.QComboBox(self)
        self.resSelector.addItem("512x512", "$outputsize@9,9")
        self.resSelector.addItem("1024x1024", "$outputsize@10,10")
        self.resSelector.addItem("2048x2048", "$outputsize@11,11")
        self.inputGridLayout.addWidget(self.resSelector, 1, 1, 1, 3)
        self.resSelector.activated.connect(self.res_selected)

    def res_selected(self, index):
        self.res = self.resSelector.itemData(index)
        print(self.res)
        return self.res

    def remove(self):
        self.builderLayout.removeWidget(self.groupboxOutputs)
        self.groupboxOutputs.close()

    def kill(self):
        reader.kill()

    def clear_log(self):
        self.watcher_log.clear()

    def atom_material(self, preset):
        # setup material template
        self._material = AtomMaterial('C:/dccapi/dev/Gems/DccScriptingInterface/LyPy/'
                                       'si_substance/builder/atom.material')
        self.preset = preset
        self._material.DiffuseMap_tex = self.texRenderPathComboBox.currentText()+"/"+self.sbsarName+"_"+self.preset.\
            replace(" ", "")+"_diffuse.tif"
        self._material.set_map(self._material.tex[0], self._material.DiffuseMap_tex)
        self._material.NormalMap_tex = self.texRenderPathComboBox.currentText()+"/"+self.sbsarName+"_"+self.preset.\
            replace(" ", "")+"_normal.tif"
        self._material.set_map(self._material.tex[1], self._material.NormalMap_tex)
        self._material.SpeculareMap_tex =self.texRenderPathComboBox.currentText()+"/"+self.sbsarName+"_"+self.preset.\
            replace(" ", "")+"_specular.tif"
        self._material.set_map(self._material.tex[2], self._material.SpeculareMap_tex)

        # self.mat_path = self.sbsarDirectory + "/" + self.atomMatNameComboBox.currentText() + ".material"
        if not self.preset:
            mat_path = self.sbsarDirectory + "/" + self.sbsarName + ".material"
        else:
            mat_path = self.sbsarDirectory + "/" + self.sbsarName + "_" + self.preset.replace(" ", "") + ".material"

        self._material.write(mat_path)
        print(mat_path)

    def create_single(self):
        self.atom_material(self.preset)

    def create_all_material(self):
        for i in self.sbsar_presets:
            print(i.replace(" ", ""))
            self.atom_material(i)

    def print_something(self, words):
        self.words = words
        print(self.words)

    def watch(self):
        reader.start('python', ['-u', window.watcher_script, window.sbsarDirectory])

    def create_atom_mat(self):
        self.atom_material_button = self.create_button("&Create Single", self.create_single)
        self.atom_material_output = self.create_button("&Create All", self.create_all_material)
        self.renderPathbutton = self.create_button("&Output Path", self.browseRenderPath)
        self.rendeTexbutton = self.create_button("&Render Textures", self.render_tex)
        self.sbsarName = self.item.text().replace(".sbsar", "")
        self.atomMatNameComboBox.setCurrentText(self.sbsarName)
        self.atomMatNameComboBox.setDisabled(True)
        # self.texRenderPathComboBox.setCurrentText(self.path)
        self.gridLayoutMat = QtWidgets.QGridLayout(self.groupboxMat)
        self.MatHBox = QtWidgets.QHBoxLayout()
        self.MatHBox.addWidget(self.atomMatLabel)
        self.MatHBox.addWidget(self.atomMatNameComboBox)
        self.MatHBox.addWidget(self.atom_material_output)
        self.MatHBox.addWidget(self.atom_material_button)
        self.gridLayoutMat.addLayout(self.MatHBox, 0, 0, 1, 4)
        self.MatHBox1 = QtWidgets.QHBoxLayout()
        self.MatHBox1.addWidget(self.textOutputPathLabel)
        self.MatHBox1.addWidget(self.texRenderPathComboBox)
        self.MatHBox1.addWidget(self.renderPathbutton)
        self.MatHBox1.addWidget(self.rendeTexbutton)
        self.gridLayoutMat.addLayout(self.MatHBox1, 1, 0, 1, 4)
        self.builderLayout.addWidget(self.groupboxMat, 8, 0, 1, 3)
        print("\n")

    def randomseed(self):
        import random
        random_num = random.randrange(1,10**10)
        self.randomstr = '$randomseed@'+str(random_num)
        self.randomSeedComboBox.setCurrentText(str(random_num))
        print(self.randomstr)
        return self.randomstr

    def render_tex(self):
        self.output_name = self.atomMatNameComboBox.currentText()
        if not os.path.exists(self.texRenderPathComboBox.currentText()):
            os.makedirs(self.texRenderPathComboBox.currentText())
            print(self.texRenderPathComboBox.currentText())
        else:
            print(self.texRenderPathComboBox.currentText())

        sbs.sbsrender_render(inputs=self.path+"/"+self.item.text(),
                             # inputs=os.path.join(self.outputCookPath, self.outputName + '.sbsar'),
                             # input_graph=_inputGraphPath,
                             # output_path=self.texRenderpath,
                             output_path=self.texRenderPathComboBox.currentText(),
                             # output_name=self.output_name+'_{outputNodeName}',
                             output_name=self.output_name+'_{outputNodeName}',
                             output_format='tif',
                             # set_value=['$outputsize@%s,%s' % (_outputSize, _outputSize), '$randomseed@1'],
                             set_value=[self.res, self.randomstr],
                             # use_preset = _user_preset[0]
                             no_report=True,
                             verbose=True
                             ).wait()

    def find_1st_sbsar(self):
        filelist = []
        pathx = self.directoryComboBox.currentText()
        for r, d, f in os.walk(pathx):
            for file in f:
                filelist.append(file)
        for i in range(len(filelist)):
            if filelist[i].split(".")[-1] == "sbsar":
                return i
            else:
                i += 1

    def open_file_of_item(self, row, column):
        self.item = self.filesTable.item(row, 0)
        print(self.path+"/"+self.item.text())

        # Load sbsar parameters.
        if self.item.text().split(".")[-1] == "sbsar":
            self.create_sbsar_presets()
            self.create_tex_res()
            self.builderLayout.removeWidget(self.groupboxOutputs)
            self.groupboxOutputs.close()
            self.create_sbsar_tex()
            self.builderLayout.removeWidget(self.groupboxMat)
            self.groupboxMat.close()
            self.builderLayout.addWidget(self.inputGroupBox, 5, 0, 1, 3)
            self.randomSeedComboBox.setCurrentText("0000000000")
            self.create_atom_mat()

        # Cook sbsar file and trigger watcher to generate textures.
        elif self.item.text().split(".")[-1] == "sbs":
            builder.cook_sbsar(self.path+"/"+self.item.text(), builder._context, self.path,
                               self.item.text().split(".")[0])
            # builder.output_info()


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

# Creaste Substance Builder QtWidget
def createWin():
    mainWindow = uiMgr.getMainWindow()
    window = Window(parent=mainWindow)
    reader = ProcessOutputReader()
    reader.produce_output.connect(window.append_output)
    reader.start('python', ['-u', window.watcher_script, window.sbsarDirectory])
    window.show()
    window.find()


if __name__ == '__main__':
    app = sd.getContext().getSDApplication()
    uiMgr = app.getQtForPythonUIMgr()
    menu = uiMgr.newMenu(menuTitle="Atom", objectName="atom")
    # Create a new action.
    act = QtWidgets.QAction("Substance Builder", menu)
    act.triggered.connect(createWin)
    # Add the action to the menu.
    menu.addAction(act)
    #sys.exit(app.exec_())
