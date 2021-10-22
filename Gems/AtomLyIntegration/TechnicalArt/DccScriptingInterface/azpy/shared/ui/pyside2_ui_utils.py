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
#
from __future__ import unicode_literals
# from builtins import str

# built in's
import os
import site
import uuid
import logging as _logging
import xml.etree.ElementTree as xml  # Qt .ui files are xml
from io import StringIO  # for handling unicode strings

# azpy extensions
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
# ^ this is effectively an import and retreive of <dccsi>\config.py
# and init's access to Qt/Pyside2
# init lumberyard Qy/PySide2 access

# now default settings are extended with PySide2
# this is an alternative to "from dynaconf import settings" with Qt
settings = _config.get_config_settings(setup_ly_pyside=True)

# 3rd Party (we may or do provide)
from unipath import Path

# now we can import lumberyards PySide2
import PySide2.QtCore as QtCore
import PySide2.QtWidgets as QtWidgets
import PySide2.QtGui as QtGui
from PySide2.QtWidgets import QApplication, QSizePolicy
import PySide2.QtUiTools as QtUiTools

# special case for import pyside2uic
site.addsitedir(settings.DCCSI_PYSIDE2_TOOLS)
import pyside2uic

#  azpy
import azpy.shared.ui.qt_settings as qt_settings
import azpy.shared.ui.help_menu as help_menu

# -------------------------------------------------------------------------
#  global space debug flag
_DCCSI_GDEBUG = settings.DCCSI_GDEBUG

#  global space debug flag
_DCCSI_DEV_MODE = settings.DCCSI_DEV_MODE

_MODULE_PATH = Path(__file__)

_ORG_TAG = 'Amazon_Lumberyard'
_APP_TAG = 'DCCsi'
_TOOL_TAG = 'azpy.shared.ui.pyside2_ui_utils'
_TYPE_TAG = 'test'

_MODULENAME = _TOOL_TAG
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Something invoked :: {0}.'.format(_MODULENAME))

_UI_FILE = Path(_MODULE_PATH.parent, 'resources', 'example.ui')
# -------------------------------------------------------------------------


class UiLoader(QtUiTools.QUiLoader):
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
# -------------------------------------------------------------------------


class UiWidget(QtWidgets.QWidget):
    def __init__(self, ui_file=_UI_FILE, parent=None):
        super().__init__(parent)
        loader = UiLoader(parent)
        file = QFile(ui_file)
        file.open(QFile.ReadOnly)
        loader.load(file, self)
        file.close()
# -------------------------------------------------------------------------


def from_ui_generate_form_and_base_class(filename, return_output=False):
    """Parse a Qt Designer .ui file and return Pyside2 Form and Base Class
    Usage:
            import azpy.shared.ui as azpyui
            form_class, base_class = azpyui.from_ui_generate_form_and_class(r'C:\my\filepath\tool.ui')
    """
    ui_file = Path(filename)
    output = ''
    parsed_xml = None
    try:
        ui_file.exists()
    except FileNotFoundError as error:
        output += 'File does not exist: {0}/r'.format(error)
        if _DCCSI_GDEBUG:
            print(error)
        if return_output:
            return False, output
        else:
            return False

    try:
        ui_file.ext == 'ui'
    except IOError as error:
        output += 'Not a Qt Designer .ui file: {0}/r'.format(error)
        if return_output:
            return False, output
        else:
            return False

    parsed_xml = xml.parse(ui_file)
    form_class = parsed_xml.find('class').text
    widget_class = parsed_xml.find('widget').get('class')

    with open(ui_file, 'r') as ui_file:
        stream = StringIO()  # create a file io stream
        frame = {}
        
        _uic_compiler_path = Path(settings.DCCSI_PYSIDE2_TOOLS)
        site.addsitedir(_uic_compiler_path)
        
        import pyside2uic

        # compile the .ui file as a .pyc represented in steam
        pyside2uic.compileUi(ui_file, stream, indent=4)
        # compile the .pyc bytecode from stream
        pyc = compile(stream.getvalue(), '', 'exec')
        # execute the .pyc bytecode
        exec (pyc, frame)

        # Retreive the form_class and base_class based on type in designer .ui (xml)
        form_class = frame['Ui_{0}'.format(form_class)]
        base_class = eval('QtWidgets.{0}'.format(widget_class))

        ui_file.close()

    if return_output:
        return form_class, base_class, output
    else:
        return form_class, base_class
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
