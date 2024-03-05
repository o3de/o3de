"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# bit more complex example set to demo connecting O3DE tech to PySide2 widgets
import azlmbr.bus
from PySide2 import QtWidgets
from PySide2 import QtGui
from shiboken2 import wrapInstance, getCppPointer

params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetQtBootstrapParameters')
if(params is not None and params.mainWindowId != 0):
    mainWidgetWindow = QtWidgets.QWidget.find(params.mainWindowId)
    mainWindow = wrapInstance(int(getCppPointer(mainWidgetWindow)[0]), QtWidgets.QMainWindow)
    mainWindow.menuBar().addMenu("&Hello")

allWindows = QtGui.QGuiApplication.allWindows()
print ('allWindows = {}'.format(allWindows))

focusWin = QtGui.QGuiApplication.focusWindow()
print ('focusWin = {}'.format(focusWin))

buttonFlags = QtWidgets.QMessageBox.information(QtWidgets.QApplication.activeWindow(), 'title', 'ok')
print ('buttonFlags = {}'.format(buttonFlags))


