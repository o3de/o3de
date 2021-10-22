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
import os
import sys
import logging as _logging
import uuid
import xml.etree.ElementTree as xml  # Qt .ui files are xml
from io import StringIO  # for handling unicode strings

# 3rd Party (we may or do provide)
from unipath import Path

# azpy extensions
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
# ^ this is effectively an import and retreive of <dccsi>\config.py
# and init's access to Qt/Pyside2
# init lumberyard Qy/PySide2 access

# now default settings are extended with PySide2
# this is an alternative to "from dynaconf import settings" with Qt
settings = _config.get_config_settings(setup_ly_pyside=True)

# now we can import lumberyards PySide2
import azpy.shared.ui.qt_settings as qt_settings
import azpy.shared.ui.help_menu as help_menu
import azpy.shared.ui.pyside2_ui_utils as ui_utils

import pyside2uic
import PySide2.QtCore as QtCore
import PySide2.QtWidgets as QtWidgets
import PySide2.QtGui as QtGui
import PySide2.QtUiTools as QtUiTools

# -------------------------------------------------------------------------
#  global space debug flag
_DCCSI_GDEBUG = settings.DCCSI_GDEBUG

#  global space debug flag
_DCCSI_DEV_MODE = settings.DCCSI_DEV_MODE

_MODULE_PATH = Path(__file__)

_MODULENAME = 'azpy.shared.ui.teamplates'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Something invoked :: {0}.'.format(_MODULENAME))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# example .ui used as default (for tests, etc.)
_UI_FILE = Path(_MODULE_PATH.parent, 'resources', 'example.ui')
# Hmmm.... should be a better way to handle this?
_FORM_CLASS, _BASE_CLASS = ui_utils.from_ui_generate_form_and_base_class(_UI_FILE)
# looks like either we aren't compiling it or it's not provided in the current
# version of Qt we use (check mack Qt5.15)
# https://doc-snapshots.qt.io/qtforpython-5.15/PySide2/QtUiTools/ls.loadUiType.html)
#_FORM_CLASS, _BASE_CLASS = QtUiTools.loadUiType(_UI_FILE)
# print(QtUiTools.QUiLoader) <-- this one works differently but maybe that pattern is better?

# default dark styling for standalone apps
_DARK_STYLE = Path(_MODULE_PATH.parent, 'resources', 'qdarkstyle', 'style.qss')
# ^ lumberyard style doesn't work for all widgets, so this can be direcly applied
# to sidgets that look funny, they won't be a perfect match but also won't look odd
# To Do: make optional and/or force only when standalone
# adopt the Qt app style by default (use lumberyards style when it's the parent)
# include a standlaone lumberyard-like styling also
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# https://doc.qt.io/qt-5/designer-using-a-ui-file-python.html
# pattern #1

# pyside2-uic.exe form.ui > form.py
# results might be something like this:

#from ui_form import Ui_Form
# class UicWidget(QtWidgets.QWidget):
    #def __init__(self, parent=None):
        #super(Window, self).__init__(parent)
        #self.m_ui = Ui_Form()
        #self.m_ui.setupUi(self)

# pattern #2
# that could also be represented like?

# class SomeWidget(Ui_Form, QtWidgets.QWidget):
    #def __init__(self, parent=None):
        #super(Window, self).__init__(parent)
        # self.setupUi(self)

# pattern 3, use QUiLoader
# you need to know the main widget class (QtWidgets.QWidget), load .ui form within class
#from PySide2.QtUiTools import QUiLoader
#from PySide2 import QtWidgets
#from PySide2.QtCore import QFile

#class MyForm(QtWidgets.QWidget):
    #def __init__(self, parent=None):
        #QtWidgets.QWidget.__init__(self, parent)
        
        ## read/load my .ui file
        #file = QFile("form.ui")
        #file.open(QFile.ReadOnly)
        #self.loader = QUiLoader()
        #self.my_widget = self.loader.load(file, self)
        #file.close()
        
        ## setup my own layout
        #layout = QtWidgets.QVBoxLayout()
        #layout.addWidget(self.my_widget)
        #self.setLayout(layout)

#if __name__ == '__main__':
    #app = QtWidgets.QApplication(sys.argv)
    #myapp = MyForm()
    #myapp.show()
    #sys.exit(app.exec_())
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# pattern 4 (I like this)
#_UI_FILE = Path(_MODULE_PATH.parent, 'resources', 'example.ui')
#_FORM_CLASS, _BASE_CLASS = ui_utils.from_ui_generate_form_and_base_class(_UI_FILE)

# here is basically what that returns ...
#parsed_xml = xml.parse(ui_file)
# form_class = parsed_xml.find('class').text              # --> <class 'Ui_Form'>
# widget_class = parsed_xml.find('widget').get('class')   # --> <class 'PySide2.QtWidgets.QWidget'>

#class MyToolWidget(TemplateToolWidget):
    #def __init__(self, parent, *args, **kwargs):
        #super().__init__(parent, *args, **kwargs)

#my_tool = MyToolWidget()


class TemplateToolWidget(_FORM_CLASS, _BASE_CLASS):
    def __init__(self, parent, logger=None, *args, **kwargs):
        '''A custom tool window with a demo set of template ui functionality'''

        super().__init__(parent, *args, **kwargs)

        self._logger = logger
        if not self._logger:
            try:
                self._logger = self.parent().logger
            except:
                self._logger = azpy.initialize_logger(self.objectName())

        self.logger.debug("Method: {0}.{1}".format(__class__, '__init__'))

        # lets hope the parent is a mainwindow?
        self.mainwindow = parent

        self._project_directory = Path('C:\Lumberyard', 'Dev', 'Gems',
                                       'DccScriptingInterface', 'MockProject')

        # uic adds a function to our class called setupUi,
        # calling this creates all the widgets from the .ui file
        self.setupUi(self)

        # this one is local to this class
        self.setup_ui()

        # Connect the interface controls
        self.connect_interface()
    # ----------------------------------------------------------------------

    # -- properties --------------------------------------------------------
    @property
    def logger(self):
        return self._logger

    @logger.setter
    def logger(self, logger):
        self._logger = logger
        return self._logger

    @logger.getter
    def logger(self):
        return self._logger
    # ----------------------------------------------------------------------

    def setup_ui(self):
        """TODO: Doc String"""
        # override this method to inject your own ui widgets amd layout

        self.logger.debug("Method: {0}.{1}".format(__class__, 'setup_ui'))

        self.helpMenu = help_menu.HelpMenu(self.mainwindow, 'TemplateToolTool Help...', 'http://dccSI.com/NewTool')

        _DARK_STYLE = Path(_MODULE_PATH.parent, 'resources', 'qdarkstyle', 'style.qss')

        try:
            self.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        except Exception as e:
            self.logger.warning('warning! : {}'.format(e))

        # override some of widgets to a better dark style
        self.WatchdogList_treeView.setStyleSheet(_DARK_STYLE.read_file())
        self.WatchdogList_treeView.setMinimumHeight(250)
        self.output_console_textEdit.setStyleSheet(_DARK_STYLE.read_file())
        self.output_console_textEdit.setMinimumHeight(250)
        pass
    # ----------------------------------------------------------------------

    def set_defaults(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'set_defaults'))
        pass
    # ----------------------------------------------------------------------

    def connect_interface(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'connect_interface'))
        # Connect widgets to methods
        # QtCore.QObject.connect(self.renameButton, QtCore.SIGNAL("clicked()"), self.NewCommand)
        pass
    # ----------------------------------------------------------------------

    def new_command(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'new_command'))
        pass
    # ----------------------------------------------------------------------
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
class TemplateMainWindow(QtWidgets.QMainWindow):
    """TODO"""

    _ORG_TAG = 'Amazon_Lumberyard'
    _APP_TAG = 'DCCsi'

    def __init__(self,
                 parent=None,
                 logger=None,
                 app=None,
                 custom_tool_widget=None,
                 window_title='TemplateToolWidget',
                 app_icon="icon.png",
                 *args, **kwargs):
        """TODO"""

        # secret demo
        demo = kwargs.pop('demo', False)

        self._logger = logger
        self._qapp = None
        self._name_uuid = '{0}_{1}'.format(self.__class__.__name__, uuid.uuid4())

        if not self._logger:
            self._logger = azpy.initialize_logger(self._name_uuid)
            # self._logger = initialize_logger()

        self._qapp = app or QtWidgets.QApplication.instance()
        if self._qapp is not None:
            pass
        else:
            # self.logger.debug('No QApplication has been instantiated')
            self._qapp = self._make_standalone_app(window_title)

        super(TemplateMainWindow, self).__init__(parent=parent, *args, **kwargs)

        # camel case because this is a Qt|Pyside2 widget method
        if self.objectName() == '':
            # Set a unique object name string so Maya can easily look it up
            self.setObjectName(self._name_uuid)

        self.window_title = window_title
        if self.window_title == '':
            self.window_title = self.objectName()
        self.setWindowTitle(self.window_title)

        self._style_sheet = None
        if not self._style_sheet:
            self._style_sheet = Path(_MODULE_PATH.parent, 'resources',
                                     'stylesheets', 'LYstyle.qss')
            self.setStyleSheet(self._style_sheet.read_file())

        self.app_icon = app_icon

        self._custom_widget = custom_tool_widget
        if self._custom_widget:
            self._custom_widget.parent(self)

        self.setup_ui()
        if demo:
            custom_tool_widget = TemplateToolWidget(self)

        self.add_custom_widget(custom_tool_widget)

        # Setup the settings .ini
        _TYPE_TAG = 'TEMPLATE'
        self.settings = qt_settings.createSettings(TemplateMainWindow._ORG_TAG,
                                                   TemplateMainWindow._APP_TAG,
                                                   self.window_title, _TYPE_TAG)

        # Read the saved settings
        self.read_settings()
    # ---------------------------------------------------------------------

    def _make_standalone_app(self, name):
        useGUI = not '-no-gui' in sys.argv
        self.qapp = QtWidgets.QApplication(sys.argv) if useGUI else QtWidgets.QCoreApplication(sys.argv)
        self.qapp.setOrganizationName(TemplateMainWindow._ORG_TAG)
        self.qapp.setApplicationName('{app}:{tool}'.format(app=TemplateMainWindow._APP_TAG,
                                                           tool=name))
        return self.qapp
    # ----------------------------------------------------------------------

    # -- properties --------------------------------------------------------
    @property
    def logger(self):
        return self._logger

    @logger.setter
    def logger(self, logger):
        self._logger = logger
        return self._logger

    @logger.getter
    def logger(self):
        return self._logger

    @property
    def custom_widget(self):
        return self._lcustom_widget

    @custom_widget.setter
    def custom_widget(self, custom_widget):
        self._custom_widget = custom_widget
        return self._custom_widget

    @custom_widget.getter
    def custom_widget(self):
        return self._custom_widget

    @property
    def qapp(self):
        return self._qapp

    @qapp.setter
    def qapp(self, qapp):
        self._qapp = qapp
        return self._qapp

    @qapp.getter
    def qapp(self):
        return self._qapp

    @property
    def app_icon(self):
        return self._app_icon

    @app_icon.setter
    def app_icon(self, icon):
        self._app_icon = QtGui.QIcon(icon)
        self.setWindowIcon(self._app_icon)
        return self._app_icon

    @app_icon.getter
    def app_icon(self):
        return self._app_icon

    def setIcon(self, icon):
        self.app_icon = icon
    # ----------------------------------------------------------------------

    def setup_ui(self, *args, **kwargs):
        #         self.toolbar = QtWidgets.QToolBar()
        #         self.addToolBar(self.toolbar)
        self.logger.debug("Method: {0}.{1}".format(__class__, 'setup_ui'))
        self.setIcon = self.app_icon

        # Exit QAction on hotkey
        exit_tag = "Exit"
        exit_action = QtWidgets.QAction(exit_tag, self)
        exit_action.setShortcut("Ctrl+Q")
        exit_action.triggered.connect(self.close)

        # basic menubar File > Exit event
        self.menu_bar = self.menuBar()  # type: QMenuBar
        file_menu = self.menu_bar.addMenu("File")  # type: QMenu
        file_menu.addAction(exit_tag, self.close)

        # main widget
        self.central_widget = QtWidgets.QWidget(self)
        self.setCentralWidget(self.central_widget)

        # layout initialize
        self.global_layout = QtWidgets.QVBoxLayout(self.central_widget)
        self.global_layout.setContentsMargins(8, 8, 8, 8)
        self.central_widget.setLayout(self.global_layout)

        self.createStatusBar()

        # TODO: create a progress bar
        # https://codeloop.org/how-to-create-progressbar-in-pyside2/amp/
    # ----------------------------------------------------------------------

    def add_custom_widget(self, custom_tool_widget=None):
        # Add our TemplateToolWidget
        if custom_tool_widget:
            self.custom_widget = custom_tool_widget
            self.custom_widget.setSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
            layout = QtWidgets.QFormLayout()
            self.global_layout.addLayout(layout)
            self.global_layout.addWidget(self.custom_widget)
#         else:  # demo test tool
#             self.custom_widget = TemplateToolWidget(self)
    # ----------------------------------------------------------------------

    def createStatusBar(self):
        self.logger.debug("Method: {0}.{1}".format(__class__, 'createStatusBar'))
        self.myStatus = QtWidgets.QStatusBar()
        self.myStatus.showMessage("{0}: Ready".format(self.window_title), 3000)
        self.setStatusBar(self.myStatus)
        # ----------------------------------------------------------------------

    @QtCore.Slot()
    def closeEvent(self, event=None, *args, **kwargs):
        """Event which is run when window closes"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'closeEvent'))

        close = QtWidgets.QMessageBox()
        result = close.question(self,
                                "Confirm Exit...",
                                "Are you sure you want to exit ?",
                                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No)

        if result == QtWidgets.QMessageBox.Yes:
            if event:
                event.accept()
            self.logger.debug("Closing: {0}".format(self.objectName()))
            self.write_settings()
            self.qapp.instance().quit
            self.qapp.exit()
        else:
            if event:
                event.ignore()
    # ----------------------------------------------------------------------

    def write_settings(self):
        """Writes out the windows settings"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'write_settings'))
        # main window
        # saves window size and position
        self.settings.setValue("geometry", self.saveGeometry())
        # saves the state of window's toolbars and dockWidgets
        self.settings.setValue("windowState", self.saveState())
        # widgets, save a setting to persist
        # self.settings.setValue("textFilePath", self.textFileLineEdit.text())
    # ----------------------------------------------------------------------

    def read_settings(self):
        """Reads in the windows settings"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'read_settings'))
        # main window
        self.restoreGeometry(self.settings.value("geometry"))  # restores window size and position
        self.restoreState(self.settings.value("windowState"))  # restores the state of window's toolbars and dockWidgets
        # widgets, restore something from settings
        # if self.settings.value("textFilePath"):
        # textFile = self.qt_settings.value("textFilePath",).toString()
        # self.textFileLineEdit.setText(textFile)
    # ----------------------------------------------------------------------
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
class TestToolWidget(TemplateToolWidget):
    def __init__(self, parent, *args, **kwargs):
        '''A custom window with a demo set of ui widgets'''

        super(TestToolWidget, self).__init__(parent=parent, *args, **kwargs)

        self.logger.debug("Method: {0}.{1}".format(__class__, '__init__'))

        # this one is local to this class
        self.extend_ui()

        self._test_property = 'Property Not set'
        self.logger.debug("self._test_property: {0}".format(self._test_property))

        self.class_tests()
    # ----------------------------------------------------------------------

    # -- properties --------------------------------------------------------
    @property
    def test_property(self):
        return self._test_property

    @test_property.setter
    def test_property(self, test_property):
        self._test_property = test_property
        return self._test_property

    @test_property.getter
    def test_property(self):
        return self._test_property
    # ----------------------------------------------------------------------

    def class_tests(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'class_tests'))
        self.test_property = 'TEST PROPERTY'
        self.logger.debug("self._test_property: {0}".format(self.test_property))
        return
    # ----------------------------------------------------------------------

    def setup_ui(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'setup_ui'))
        return

    def extend_ui(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'extend_ui'))

        # this would be local to this class, runs outside _init__
        #
        # self.setup_ui()

        # Connect the interface controls
        # self.connect_interface()

        self._watchdog_list = QtGui.QStandardItemModel(parent=self)

        self._project_directory = Path('C:\Lumberyard', 'Dev', 'Gems',
                                       'DccScriptingInterface', 'MockProject')

        # we want 2 columns
        self._watchdog_list.setColumnCount(2)

        # set up a temp watch dog folder watch list (as a model)
        self._watchdog_list.setHorizontalHeaderLabels(['Path', 'State'])

        _DARK_STYLE = Path(_MODULE_PATH.parent, 'resources', 'qdarkstyle', 'style.qss')

        # override some of widgets to a better dark style
        self.WatchdogList_treeView.setStyleSheet(_DARK_STYLE.read_file())
        self.WatchdogList_treeView.setMinimumHeight(250)
        self.WatchdogList_treeView.setSelectionBehavior(QtWidgets.QAbstractItemView.SelectRows)
        self.WatchdogList_treeView.setModel(self._watchdog_list)
        self.WatchdogList_treeView.setUniformRowHeights(True)

        # populate treeView data
        parent_item = self._watchdog_list.invisibleRootItem()
        first_row = QtGui.QStandardItem(self._project_directory)
        self._watchdog_list.appendRow(first_row)
        # span container columns
        self.WatchdogList_treeView.setFirstColumnSpanned(0, self.WatchdogList_treeView.rootIndex(), True)

        TemplateMainWindow = QtGui.QStandardItem('\\Assets\\some\\path')
        self._watchdog_list.appendRow(TemplateMainWindow)
        first_row_state = QtGui.QStandardItem('{0}'.format('Active'))
        # TemplateMainWindow.appendRow(first_row_state)

        second_row = QtGui.QStandardItem('\\Assets\\another\\path')
        self._watchdog_list.appendRow(second_row)
        second_row_state = QtGui.QStandardItem('{0}'.format('Active'))
        # second_row.appendRow(second_row_state)

        first_row.appendRow([TemplateMainWindow, second_row])
        # span container columns
        self.WatchdogList_treeView.setFirstColumnSpanned(0, self.WatchdogList_treeView.rootIndex(), True)

        self.output_console_textEdit.setStyleSheet(_DARK_STYLE.read_file())
        self.output_console_textEdit.setMinimumHeight(250)

        # TODO: create a progress bar
        # https://codeloop.org/how-to-create-progressbar-in-pyside2/amp/
        pass
    # ----------------------------------------------------------------------

    def set_defaults(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'extend_ui'))
        pass
    # ----------------------------------------------------------------------

    def connect_interface(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'connect_interface'))
        # Connect widgets to methods
        # QtCore.QObject.connect(self.renameButton, QtCore.SIGNAL("clicked()"), self.NewCommand)
        pass
    # ----------------------------------------------------------------------

    def new_command(self):
        """TODO: Doc String"""
        self.logger.debug("Method: {0}.{1}".format(__class__, 'new_command'))
        pass
    # ----------------------------------------------------------------------
# --------------------------------------------------------------------------


# --------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""
    import sys

    _TEST_APP_NAME = '{0}-{1}'.format(_MODULENAME, 'TEST')

    _LOGGER = azpy.initialize_logger(_TEST_APP_NAME,
                                     log_to_file=True,
                                     default_log_level=_logging.DEBUG)

    from azpy.constants import STR_CROSSBAR
    _LOGGER.info(STR_CROSSBAR)
    _LOGGER.info("{0} :: if __name__ == '__main__':".format(_MODULENAME))
    _LOGGER.info(STR_CROSSBAR)

    # test from_ui_generate_form_and_base_class
    _FORM_CLASS, _BASE_CLASS = ui_utils.from_ui_generate_form_and_base_class(_UI_FILE)

    _LOGGER.info(_FORM_CLASS)
    _LOGGER.info(_BASE_CLASS)

    _LOGGER.info("Starting App: {0} ...".format(_TEST_APP_NAME))

while 0:
    # Test 1, MainWindow with no widget (just the frame)
    _MAINWINDOW = TemplateMainWindow(logger=_LOGGER)
    _LOGGER.info(_MAINWINDOW.objectName())
    _LOGGER.info(_MAINWINDOW.settings)
    _LOGGER.info(_MAINWINDOW.settings.fileName())
    _MAINWINDOW.setWindowTitle(_TEST_APP_NAME)
    _MAINWINDOW.show()
    break

while 0:
    # test 2, demo forces a template widget to be created internally
    _MAINWINDOW = TemplateMainWindow(logger=_LOGGER,
                                     demo=True) #<-- template demo
    _LOGGER.info(_MAINWINDOW.objectName())
    _LOGGER.info(_MAINWINDOW.settings)
    _LOGGER.info(_MAINWINDOW.settings.fileName())
    _MAINWINDOW.setWindowTitle(_TEST_APP_NAME)
    _MAINWINDOW.show()
    break

while 1:
    # test 3, add a custom tool widget
    _MAINWINDOW = TemplateMainWindow(logger=_LOGGER)
    _CUSTOM_WIDGET = TemplateToolWidget(parent=_MAINWINDOW)
    _MAINWINDOW.add_custom_widget(_CUSTOM_WIDGET)
    _LOGGER.info(_MAINWINDOW.objectName())
    _LOGGER.info(_MAINWINDOW.settings)
    _LOGGER.info(_MAINWINDOW.settings.fileName())
    _MAINWINDOW.setWindowTitle(_TEST_APP_NAME)
    _MAINWINDOW.show()
    _MAINWINDOW.close()  # test the close function
    break

del _LOGGER
sys.exit(_MAINWINDOW.qapp.exec_())



