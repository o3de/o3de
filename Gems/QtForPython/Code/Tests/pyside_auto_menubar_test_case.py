"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

# Import the Editor API to test the PySide2 & Qt 5.12.x integration
import azlmbr.bus
import azlmbr.editor as editor
import azlmbr.legacy.general as general


def printOrExcept(expression, message):
    if(expression):
        print (message)
        return
    failed = 'FAILED - '.format(message)
    print (failed)
    general.exit_no_prompt()
    raise Exception(failed)


printOrExcept(azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'IsActive'), 'QtForPython Is Ready')

# the PySide2 and shiboken2 libraries should import cleanly
try:
    from shiboken2 import wrapInstance, getCppPointer
    from PySide2 import QtWidgets
    from PySide2 import QtGui
except:
    printOrExcept(False, 'Importing PySide2 and Shiboken2')

allWindows = QtGui.QGuiApplication.allWindows()
printOrExcept(len(allWindows) > 0, 'Value allWindows greater than zero')

azMainWidgetId = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetMainWindowId')
printOrExcept(azMainWidgetId is not 0, 'GetMainWindowId')

mainWidgetWindow = QtWidgets.QWidget.find(azMainWidgetId)
mainWindow = wrapInstance(int(getCppPointer(mainWidgetWindow)[0]), QtWidgets.QMainWindow)
printOrExcept(mainWindow is not None, 'Get QtWidgets.QMainWindow')

menuBar = mainWindow.menuBar()
printOrExcept(menuBar is not None, 'Value menuBar is valid')
for action in menuBar.actions():
    if("File" in action.text()):
        print('Found File action')
    elif("Edit" in action.text()):
        print('Found Edit action')
    elif("Game" in action.text()):
        print('Found Game action')
    elif("Tools" in action.text()):
        print('Found Tools action')


general.exit_no_prompt()
