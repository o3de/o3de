# -*- coding: utf-8 -*-
# !/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
import sys, os

from PySide2 import QtCore, QtWidgets
from PySide2.QtCore import QTimer
from PySide2.QtCore import QProcess, Signal, Slot, QTextCodec
from PySide2.QtGui import QTextCursor
from PySide2.QtWidgets import QApplication, QPlainTextEdit
sys.path.insert(0, os.path.abspath('../..'))
import builder
# sys.path.insert(0, os.path.abspath('../ui/'))
# import main_win
sys.path.insert(0, os.path.abspath('../watchdog/'))
import watchdog

class Window(QtWidgets.QDialog):
    def __init__(self, parent=None):
        super(Window, self).__init__(parent)
        self.browseButton = self.create_button("&Browse...", self.browse)
        self.findButton = self.create_button("&Find", self.find)
        self.removeButton = self.create_button("&Remove", self.remove)
        self.startWatch = self.create_button("&Start Watch", self.watch)
        self.killButton = self.create_button("&Stop Watch", self.kill)
        self.fileComboBox = self.createComboBox("*")
        self.textComboBox = self.createComboBox()
        # self.directoryComboBox = self.createComboBox(QtCore.QDir.currentPath())
        self.sbsarDirectory = "C:/Users/chunghao/Documents/Allegorithmic/Substance Designer/sbsar"
        self.directoryComboBox = self.createComboBox(self.sbsarDirectory)
        self.extensionLabel = QtWidgets.QLabel("Watch extensions:")
        # textLabel = QtWidgets.QLabel("Containing text:")
        self.directoryLabel = QtWidgets.QLabel("Watch directory:")
        self.filesFoundLabel = QtWidgets.QLabel()
        self.wacherLogLabel = QtWidgets.QLabel("Watcher Log")
        self.fileListLabel = QtWidgets.QLabel("File List")
        self.sbsarPresetLabel = QtWidgets.QLabel("Sbsar Preset")
        self.OutputResLabel = QtWidgets.QLabel("Output Size")
        self.OutputsLabel = QtWidgets.QLabel("Outputs")

        self.watcher_log = QPlainTextEdit()
        self.watcher_log.setReadOnly(True)
        self.watcher_log.setMaximumBlockCount(10000)  # limit console to 10000 lines

        self.watcher_log._cursor_output = self.watcher_log.textCursor()

        self.create_files_table()

        buttonsLayout = QtWidgets.QHBoxLayout()
        buttonsLayout.addStretch()
        buttonsLayout.addWidget(self.findButton)
        # buttonsLayout.addWidget(self.removeButton)
        buttonsLayout.addWidget(self.startWatch)
        buttonsLayout.addWidget(self.killButton)

        self.mainLayout = QtWidgets.QGridLayout()
        self.mainLayout.addWidget(self.extensionLabel, 0, 0)
        self.mainLayout.addWidget(self.fileComboBox, 0, 1, 1, 2)
        # self.mainLayout.addWidget(textLabel, 1, 0)
        # self.mainLayout.addWidget(self.textComboBox, 1, 1, 1, 2)
        self.mainLayout.addWidget(self.directoryLabel, 2, 0)
        self.mainLayout.addWidget(self.wacherLogLabel, 3, 0)
        self.mainLayout.addWidget(self.fileListLabel, 4, 0)
        self.mainLayout.addWidget(self.directoryComboBox, 2, 1)
        self.mainLayout.addWidget(self.browseButton, 2, 2)
        self.mainLayout.addWidget(self.watcher_log, 3, 1, 1, 3)
        self.mainLayout.addWidget(self.filesTable, 4, 1, 1, 3)
        self.mainLayout.addWidget(self.filesFoundLabel, 5, 1)
        self.mainLayout.addLayout(buttonsLayout, 6, 0, 1, 3)
        self.setLayout(self.mainLayout)

        self.setWindowTitle("Substance Builder")
        self.resize(800, 300)
        self.path = self.directoryComboBox.currentText()
        self.groupbox_outputs = QtWidgets.QGroupBox("Outputs")
        self.watcher_script = "C:/dccapi/dev/Gems/DccScriptingInterface/LyPy/si_substance/builder/watchdog/__init__.py"

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
                            "C:/Users/chunghao/Documents/Allegorithmic/Substance Designer/sbsar")
        if self.directory:
            if self.directoryComboBox.findText(self.directory) == -1:
                self.directoryComboBox.addItem(self.directory)

            self.directoryComboBox.setCurrentIndex(self.directoryComboBox.findText(self.directory))
        self.path = self.directory
        self.find()
        return self.path

    @staticmethod
    def updateComboBox(comboBox):
        if comboBox.findText(comboBox.currentText()) == -1:
            comboBox.addItem(comboBox.currentText())

    def find(self):
        self.filesTable.setRowCount(0)

        fileName = self.fileComboBox.currentText()
        text = self.textComboBox.currentText()
        self.path = self.directoryComboBox.currentText()
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
            # file = QtCore.QFile(self.currentDir.absoluteFilePath(fn))
            # size = QtCore.QFileInfo(file).size()

            fileNameItem = QtWidgets.QTableWidgetItem(fn)
            fileNameItem.setFlags(fileNameItem.flags() ^ QtCore.Qt.ItemIsEditable)
            # sizeItem = QtWidgets.QTableWidgetItem("%d KB" % (int((size + 1023) / 1024)))
            # sizeItem.setTextAlignment(QtCore.Qt.AlignVCenter | QtCore.Qt.AlignRight)
            # sizeItem.setFlags(sizeItem.flags() ^ QtCore.Qt.ItemIsEditable)
            row = self.filesTable.rowCount()
            self.filesTable.insertRow(row)
            self.filesTable.setItem(row, 0, fileNameItem)
            # self.filesTable.setItem(row, 1, sizeItem)

        self.filesFoundLabel.setText("%d file(s) found (Double click on a file to open it)" % len(files))

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
        self.filesTable = QtWidgets.QTableWidget(0, 1)
        self.filesTable.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)

        self.filesTable.setHorizontalHeaderLabels(("File Name", ""))
        self.filesTable.horizontalHeader().setSectionResizeMode(0, QtWidgets.QHeaderView.Stretch)
        # self.filesTable.verticalHeader().hide()
        self.filesTable.verticalHeader().show()
        self.filesTable.setShowGrid(True)
        self.filesTable.cellActivated.connect(self.open_fileOfitem)

    def create_sbsar_params(self):
        self.mainLayout.addWidget(self.OutputsLabel, 9, 0)
        self.sbsarName = self.item.text().replace(".sbsar", "")
        self.groupbox_outputs = QtWidgets.QGroupBox("Outputs")
        self.horizontalLayoutOuputs = QtWidgets.QHBoxLayout(self.groupbox_outputs)
        self.horizontalLayoutOuputs.setObjectName(("horizontalLayoutOuputs"))
        # self.vertical_layout_outputs = QtWidgets.QVBoxLayout(self.groupbox_outputs)
        # self.vertical_layout_outputs.setObjectName("vertical_layout_outputs")
        self.sbsar_tex_outputs = builder.output_info(self.path, self.sbsarName)['_outputs']

        for index, tex in enumerate(self.sbsar_tex_outputs):
            print(tex, end=" ")
            self.sbsar_tex_outputs[index] = QtWidgets.QCheckBox(self.groupbox_outputs)
            self.sbsar_tex_outputs[index].setEnabled(True)
            self.sbsar_tex_outputs[index].setChecked(True)
            self.sbsar_tex_outputs[index].setObjectName(self.sbsarName+"_"+tex)
            self.sbsar_tex_outputs[index].setText(tex)
            self.horizontalLayoutOuputs.addWidget(self.sbsar_tex_outputs[index])
        self.mainLayout.addWidget(self.groupbox_outputs, 9, 1, 1, 3)
        print("\n")

    def create_sbsar_presets(self):
        self.mainLayout.addWidget(self.sbsarPresetLabel, 7, 0)
        self.sbsar_presets = builder.output_info(self.path, self.item.text().replace(".sbsar", ""))['_presets']
        self.comboBox_SbsarPreset = QtWidgets.QComboBox(self)
        for self.presetItem in self.sbsar_presets:
            print(self.presetItem, end=", ")
            preset_index = 0
            self.comboBox_SbsarPreset.addItem(self.presetItem)
            self.comboBox_SbsarPreset.setItemText(preset_index, self.presetItem)
            preset_index += 1

        self.mainLayout.addWidget(self.comboBox_SbsarPreset, 7, 1, 1, 3)
        print("\n")
        self.comboBox_SbsarPreset.activated[str].connect(self.preset_selected)

    def preset_selected(self, text):
        self.selected_text = text
        print(self.selected_text)

    def create_tex_res(self):
        self.mainLayout.addWidget(self.OutputResLabel, 8, 0)
        self.resSelector = QtWidgets.QComboBox(self)
        self.res = ["512x512", "1024x1024", "2048x2048"]
        self.resSelector.addItems(self.res)

        self.mainLayout.addWidget(self.resSelector, 8, 1, 1, 3)
        self.resSelector.activated[str].connect(self.res_selected)

    def res_selected(self):
        if self.resSelector.currentIndex() == 0: print("$outputsize@9,9")
        elif self.resSelector.currentIndex() == 1: print("$outputsize@10,10")
        elif self.resSelector.currentIndex() == 2: print("$outputsize@11,11")

    def remove(self):

        self.mainLayout.removeWidget(self.groupbox_outputs)
        self.groupbox_outputs.close()

    def kill(self):
        reader.kill()

    def watch(self):
        reader.start('python', ['-u', window.watcher_script, window.sbsarDirectory])

    def open_fileOfitem(self, row, column):
        self.item = self.filesTable.item(row, 0)
        print(self.path+"/"+self.item.text())
        if self.item.text().split(".")[-1] == "sbsar":
            self.create_sbsar_presets()
            self.create_tex_res()
            self.mainLayout.removeWidget(self.groupbox_outputs)
            self.groupbox_outputs.close()
            self.create_sbsar_params()
        elif self.item.text().split(".")[-1] == "sbs":
            print("This is a Substance File")
            builder.cook_sbsar(self.path+"/"+self.item.text(), builder._context, self.path, self.item.text().split(".")[0])
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


if __name__ == '__main__':
    app = QApplication(sys.argv)
    reader = ProcessOutputReader()
    window = Window()
    reader.produce_output.connect(window.append_output)
    reader.start('python', ['-u', window.watcher_script, window.sbsarDirectory])
    window.show()
    timer = QTimer()
    timer.timeout.connect(lambda: None)
    timer.start(10)
    # sys.exit(reader.kill())
    sys.exit(app.exec_())
