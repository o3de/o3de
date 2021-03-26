"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# bit more complex example set to demo connecting Lumberyard tech to PySide2 widgets
import azlmbr.bus
from PySide2 import QtWidgets
from PySide2 import QtGui
from shiboken2 import wrapInstance, getCppPointer

params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetQtBootstrapParameters')
if(params is not None and params.mainWindowId is not 0):
    mainWidgetWindow = QtWidgets.QWidget.find(params.mainWindowId)
    mainWindow = wrapInstance(int(getCppPointer(mainWidgetWindow)[0]), QtWidgets.QMainWindow)
    mainWindow.menuBar().addMenu("&Hello")

allWindows = QtGui.QGuiApplication.allWindows()
print ('allWindows = {}'.format(allWindows))

focusWin = QtGui.QGuiApplication.focusWindow()
print ('focusWin = {}'.format(focusWin))

buttonFlags = QtWidgets.QMessageBox.information(QtWidgets.QApplication.activeWindow(), 'title', 'ok')
print ('buttonFlags = {}'.format(buttonFlags))


