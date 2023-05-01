# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
from __future__ import unicode_literals
# from builtins import str

# built in's
import sys
import os
import uuid

#  azpy
from azpy import initialize_logger
from azpy.shared.ui.base_widget import BaseQwidgetAzpy

# 3rd Party
from unipath import Path
import PySide2.QtCore as QtCore
import PySide2.QtWidgets as QtWidgets

from PySide2.QtCore import QProcess, Signal, Slot, QTextCodec
from PySide2.QtGui import QTextCursor
from PySide2.QtWidgets import QPlainTextEdit
from PySide2.QtCore import QTimer

# -------------------------------------------------------------------------
#  global space debug flag
_DCCSI_GDEBUG = os.getenv('DCCSI_GDEBUG', False)

#  global space debug flag
_DCCSI_DEV_MODE = os.getenv('DCCSI_DEV_MODE', False)

_MODULE_PATH = Path(__file__)

_ORG_TAG = 'Amazon_Lumberyard'
_APP_TAG = 'DCCsi'
_TOOL_TAG = 'azpy.shared.ui.pyside2_qtextedit_stdout'
_TYPE_TAG = 'test'

_MODULENAME = __name__
if _MODULENAME is '__main__':
    _MODULENAME = _TOOL_TAG

if _DCCSI_GDEBUG:
    _LOGGER = initialize_logger(_MODULENAME, log_to_file=True)
    _LOGGER.debug('Something invoked :: {0}.'.format({_MODULENAME}))
else:
    _LOGGER = initialize_logger(_MODULENAME)
# -------------------------------------------------------------------------


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
# --------------------------------------------------------------------------


class MyConsole(BaseQwidgetAzpy, QPlainTextEdit):

    def __init__(self, parent=None):
        super().__init__(parent=parent)

        # Set a unique object name string so Maya can easily look it up
        self.setObjectName('{0}_{1}'.format(self.__class__.__name__,
                                            uuid.uuid4()))

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
# --------------------------------------------------------------------------


if __name__ == '__main__':
    """Run this file as main"""
    import sys

    _ORG_TAG = 'Amazon_Lumberyard'
    _APP_TAG = 'DCCsi'
    _TOOL_TAG = 'azpy.shared.ui.pyside2_qtextedit_stdout'
    _TYPE_TAG = 'test'

    if _DCCSI_GDEBUG:
        _LOGGER = initialize_logger('{0}-TEST'.format(_TOOL_TAG), log_to_file=True)
        _LOGGER.debug('Something invoked :: {0}.'.format({_MODULENAME}))

    _LOGGER.debug("{0} :: if __name__ == '__main__':".format(_TOOL_TAG))
    _LOGGER.debug("Starting App:{0} TEST ...".format(_TOOL_TAG))

#     # create the application instance
#     _APP = QtWidgets.QApplication(sys.argv)
#     _APP.setOrganizationName(_ORG_TAG)
#     _APP.setApplicationName('{app}:{tool}'.format(app=_APP_TAG, tool=_TOOL_TAG))

    # create a console and connect the process output reader to it
    _CONSOLE = MyConsole()

    # create a process output reader
    _READER = ProcessOutputReader(parent=_CONSOLE)
    _READER.produce_output.connect(_CONSOLE.append_output)

    # start something and log (including to console)
    #  this starts a test app
    _TEST_PY_FILE = Path(_MODULE_PATH.parent, 'pyside2_ui_utils.py')
    _READER.start('python', ['-u', _TEST_PY_FILE])  # start the process

    # after that starts, this will show the console
    # O3DE_QSS = Path(_MODULE_PATH.parent, 'resources', 'stylesheets', 'LYstyle.qss')
    _DARK_STYLE = Path(_MODULE_PATH.parent, 'resources', 'qdarkstyle', 'style.qss')
    _CONSOLE.qapp.setStyleSheet(_DARK_STYLE.read_file())
    _CONSOLE.show()  # make the console visible

    _LOGGER.debug(_CONSOLE.objectName())

    _TIMER = QTimer()
    _TIMER.timeout.connect(lambda: None)
    _TIMER.start(100)

    del _LOGGER
    sys.exit(_CONSOLE.qapp.exec_())
