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

from PySide2.QtCore import QProcess, Signal, Slot, QTextCodec
from PySide2.QtGui import QTextCursor
from PySide2.QtWidgets import QApplication, QPlainTextEdit
from PySide2.QtCore import QTimer

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


class MyConsole(QPlainTextEdit):

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        self.setReadOnly(True)
        self.setMaximumBlockCount(10000)  # limit console to 10000 lines

        self._cursor_output = self.textCursor()

    @Slot(str)
    def append_output(self, text):
        self._cursor_output.insertText(text)
        self.scroll_to_last_line()

    def scroll_to_last_line(self):
        cursor = self.textCursor()
        cursor.movePosition(QTextCursor.End)
        cursor.movePosition(QTextCursor.Up if cursor.atBlockStart() else
                            QTextCursor.StartOfLine)
        self.setTextCursor(cursor)

    def output_text(self, text):
        self._cursor_output.insertText(text)
        self.scroll_to_last_line()

# create the application instance
app = QApplication(sys.argv)

# create a process output reader
reader = ProcessOutputReader()

# create a console and connect the process output reader to it
console = MyConsole()
reader.produce_output.connect(console.append_output)

reader.start('python', ['-u', 'C:\\dccapi\\dev\\Gems\\DccScriptingInterface\\LyPy\\si_substance\\builder\\watchdog'
                              '\\__init__.py', 'C:\\Users\\chunghao\\Documents\\Allegorithmic\\Substance Designer'
                                               '\\sbsar'])  # start the process
console.show()                              # make the console visible
# app.exec_()                                 # run the PyQt main loop
timer = QTimer()
timer.timeout.connect(lambda: None)
timer.start(100)

sys.exit(app.exec_())
