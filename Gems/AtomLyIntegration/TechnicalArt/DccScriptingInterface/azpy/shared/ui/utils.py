#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
#
from __future__ import unicode_literals
# from builtins import str

# built in's
import os
import site
import uuid
from pathlib import Path
import logging as _logging
import xml.etree.ElementTree as xml  # Qt .ui files are xml
from io import StringIO  # for handling unicode strings
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.shared.ui.utils'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')

_UI_FILE = Path(_MODULE_PATH.parent, 'resources', 'example.ui')
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# O3DE imports
# propogates settings from __init__ so they aren't initialized over and over
from DccScriptingInterface.azpy.shared.ui import settings_core

# now we can import lumberyards PySide2
import PySide2
from PySide2 import QtGui, QtWidgets, QtGui, QtUiTools
from PySide2.QtWidgets import QApplication, QSizePolicy
from PySide2.QtUiTools import QUiLoader

# azpy
#import azpy.shared.ui.settings as qt_settings
#import azpy.shared.ui.help_menu as help_menu
# -------------------------------------------------------------------------


class UiLoader(QUiLoader):
    def __init__(self, base_instance):
        super(UiLoader, self).__init__(base_instance)
        self._base_instance = base_instance

    def createWidget(self, classname, parent=None, name=""):
        widget = super(UiLoader, self).createWidget(
            classname, parent, name)

        if parent is None:
            return self._base_instance
        else:
            setattr(self._base_instance, name, widget)
            return widget


class UiWidget(QtWidgets.QWidget):
    def __init__(self, ui_file=_UI_FILE, parent=None):
        super().__init__(parent)
        loader = UiLoader(parent)
        file = QFile(ui_file)
        file.open(QFile.ReadOnly)
        loader.load(file, self)
        file.close()


###########################################################################
# Main Code Block, runs this script as main (testing)
# -------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""
    import sys

    _LOGGER = azpy.initialize_logger('{0}-TEST'.format(_TOOL_TAG), log_to_file=True)
    _LOGGER.debug('Something invoked :: {0}.'.format({_MODULENAME}))

    _LOGGER.info("{0} :: if __name__ == '__main__':".format(_TOOL_TAG))
    _LOGGER.info("Starting App:{0} TEST ...".format(_TOOL_TAG))

    _FORM_CLASS, _BASE_CLASS = from_ui_generate_form_and_base_class(_UI_FILE)

    _LOGGER.info(_FORM_CLASS)
    _LOGGER.info(_BASE_CLASS)

    from azpy.shared.ui.templates import TemplateMainWindow

    _MAIN_WINDOW = TemplateMainWindow(logger=_LOGGER)
    _MAIN_WINDOW.show()

    del _LOGGER
    sys.exit(_MAIN_WINDOW.qapp.exec_())
# -------------------------------------------------------------------------
