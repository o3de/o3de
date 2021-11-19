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
import uuid
import weakref
import logging as _logging

# 3rd Party
from unipath import Path

# azpy extensions
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
settings = _config.get_config_settings(setup_ly_pyside=True)

import PySide2.QtWidgets as QtWidgets
import PySide2.QtCore as QtCore
from shiboken2 import wrapInstance


# -------------------------------------------------------------------------
#  global space debug flag
_DCCSI_GDEBUG = settings.DCCSI_GDEBUG

#  global space debug flag
_DCCSI_DEV_MODE = settings.DCCSI_DEV_MODE

# global maya state (if we are running in maya with gui)
#  or another dcc tool with pyside2
#  TODO implement that check
_G_PYSIDE2_DCC = None

_MODULE_PATH = Path(__file__)

_MODULENAME = 'azpy.shared.ui.azpy_base_widget'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Something invoked :: {0}.'.format(_MODULENAME))
# -------------------------------------------------------------------------


class DccWidget(object):
    """This is an experimental Class to make a widget compatible with
    a number of PySide2/Python compatible DCC Tools like Maya and Houdini"""

    _LABEL_NAME = 'no name window'  # Window display name
    _instances = list()

    # --constructor--------------------------------------------------------
    def __init__(self, parent=None, dcc_gui=_G_PYSIDE2_DCC, *args, **kwargs):

        # False or None, 'Maya', 'Houdini' ... or another pysdie2 dcc
        self._dcc_gui = dcc_gui

        self._name = '{0}_{1}'.format(self.__class__.__name__, uuid.uuid4())

        self._parent = parent
        self._parent = self._base_setup()

        # Init all baseclasses (including QWidget) of the main class
        try:
            super().__init__(*args, **kwargs)
        except Exception as Err:
            print(Err)

        self.__class__._instances.append(weakref.proxy(self))

        if isinstance(self, QtWidgets.QWidget):
            self.setParent(self._parent)

            # camel case because this is a Qt|Pyside2 widget method
            if self.objectName() == '':
                # Set a unique object name string so Maya can easily look it up
                self.setObjectName('{0}_{1}'.format(self._name))

    # -- properties -------------------------------------------------------
    @property
    def dcc_gui(self):
        return self._dcc_gui

    @dcc_gui.setter
    def dcc_gui(self, type):
        self._dcc_gui = type
        return self._dcc_gui

    @dcc_gui.getter
    def dcc_gui(self):
        return self._dcc_gui

    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, type):
        self._parent = type
        return self._parent

    @parent.getter
    def parent(self):
        return self._parent
    # ---------------------------------------------------------------------

    def _base_setup(self, *args, **kwargs):
        '''TODO'''
        # if there is not a parent, we want a mainwindow to parent to
        if self.parent == None:
            self.parent = self._make_standalone_mainwindow()
        return self.parent

    def _make_standalone_mainwindow(self):
        '''Make a standalone mainwindow parent to existing Qapp
        We parent so that this Qwidget will not be auto-destroyed or garbage
        collected if the instance variable goes out of scope.

        If a Qapp doesn't exist, start one.
        '''
        original_parent = self._parent
        new_parent = None

        if self.dcc_gui:
            if self.dcc_gui == 'Maya':
                #  import the Maya ui class
                import OpenMaya as omui
                # Parent under the main Maya window
                mainWindowPtr = omui.MQtUtil.mainWindow()
                new_parent = wrapInstance(long(mainWindowPtr), QMainWindow)
            elif self.dcc_gui == 'Houdini':
                pass  # not implemented
        else:
            from azpy.shared.ui.templates import TemplateMainWindow
            new_parent = TemplateMainWindow()

        return new_parent

    def objectName(self):
        # stand in for Qt call on mixed objects, thus camelCase
        return None

    def show(self):
        # stand in for Qt call on mixed objects, thus camelCase
        return None

    def thing(self):
        # Make this widget appear as a standalone window even though it is parented
        if isinstance(self, QtWidgets.QDockWidget):
            self.setWindowFlags(QtCore.Qt.Dialog | QtCore.Qt.FramelessWindowHint)
        else:
            try:
                self.setWindowFlags(QtCore.Qt.Window)
            except:
                pass

        # Delete the parent QDockWidget if applicable
        if isinstance(original_parent, QtWidgets.QDockWidget):
            original_parent.close()
# -------------------------------------------------------------------------


class BaseQwidgetAzpy(object):
    """Inheret from to handle common base functionality for Qt Widgets
    Parents to a standalone Qapp and mainwindow if not explicitly provided
    Place this before the Qt Widget Class in inheretance order"""

    _LABEL_NAME = 'no name window'  # Window display name
    _instances = list()

    _ORG_TAG = 'Amazon_Lumberyard'
    _APP_TAG = 'DCCsi'

    # --constructor--------------------------------------------------------
    def __init__(self,
                 parent=None,
                 logger=None,
                 qapp=None,
                 window_title=_LABEL_NAME,
                 *args, **kwargs):
        """To DO"""
        self._name_uuid = '{0}_{1}'.format(self.__class__.__name__, uuid.uuid4())

        self._logger = logger
        if not self._logger:
            self._logger = _logging.getLogger(self._name_uuid)

        self._parent = parent

        self._qapp = qapp or QtWidgets.QApplication.instance()
        if self._qapp == None:
            self.logger.debug('No QApplication has been instantiated')
            self._qapp = self._base_setup(window_title)

        # Init all baseclasses (including QWidget) of the main class
        try:
            super(BaseQwidgetAzpy, self).__init__(*args, **kwargs)
        except Exception as Err:
            print(Err)

        self.__class__._instances.append(weakref.proxy(self))

        if isinstance(self, QtWidgets.QWidget):
            self.setParent(self._parent)

            # camel case because this is a Qt|Pyside2 widget method
            if self.objectName() == '':
                # Set a unique object name string so Maya can easily look it up
                self.setObjectName(self._name)

    # -- properties -------------------------------------------------------
    @property
    def parent(self):
        return self._parent

    @parent.setter
    def parent(self, type):
        self._parent = type
        return self._parent

    @parent.getter
    def parent(self):
        return self._parent

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
    def qapp(self):
        return self._qapp

    @qapp.setter
    def qapp(self, qapp):
        self._qapp = qapp
        return self._qapp

    @qapp.getter
    def qapp(self):
        return self._qapp
    # ----------------------------------------------------------------------

    def objectName(self):
        # stand in for Qt call on mixed objects, thus camelCase
        return self._name_uuid

    def show(self):
        # stand in for Qt call on mixed objects, thus camelCase
        return None

    def _base_setup(self, *args, **kwargs):
        '''TODO'''
        # if there is not a parent, we want a mainwindow to parent to
        if self.parent == None:
            try:
                self.parent = self._make_standalone_app(self.objectName())
            except:
                self.parent = self._make_standalone_app(self._name_uuid)
        return self.parent

    def _make_standalone_app(self, name):
        useGUI = not '-no-gui' in sys.argv
        self.qapp = QtWidgets.QApplication(sys.argv) if useGUI else QtWidgets.QCoreApplication(sys.argv)
        self.qapp.setOrganizationName(self.__class__._ORG_TAG)
        self.qapp.setApplicationName('{app}:{tool}'.format(app=self.__class__._APP_TAG,
                                                           tool=name))
        return self.qapp

    @QtCore.Slot()
    def closeEvent(self, *args, **kwargs):
        """Event which is run when window closes"""
        self.logger.debug("Method: {0}.{1}".format(self.__class__.__name__, 'closeEvent'))
        self.logger.debug("Closing: {0}".format(self.objectName()))
        self.__class__._instances.remove(self)
        self.qapp.instance().quit
        self.qapp.exit()
    # ----------------------------------------------------------------------
# -------------------------------------------------------------------------


class TestWidget(BaseQwidgetAzpy, QtWidgets.QPushButton):
    def __init__(self, parent=None, *args, **kwargs):

        # Init all baseclasses (including QWidget) of the main class
        try:
            super().__init__(*args, **kwargs)
        except Exception as Err:
            print(Err)

        try:
            self.logger.debug('{0}'.format(self.parent))
        except Exception as Err:
            print(Err)

        # self.setParent(parent)

        self.setText('Push Me')
# -------------------------------------------------------------------------


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


    # test raw BaseWidget
    _TEST_BASE_WIDGET = BaseQwidgetAzpy()
    _LOGGER.info(_TEST_BASE_WIDGET.objectName())
    _TEST_BASE_WIDGET.show()  # this should do nothing,it is a dummy call
    # this call is replaced with version the QWidget
    _TEST_BASE_WIDGET.closeEvent()  # mimic Qt close
    _TEST_BASE_WIDGET = None

    # NEED TO DELETE ^ Makes a Qapp

    _TEST_WIDGET = TestWidget()
    _LOGGER.info(_TEST_WIDGET.objectName())
    _TEST_WIDGET.show()

    del _LOGGER
    sys.exit(_TEST_WIDGET.qapp.exec_())
# -------------------------------------------------------------------------
