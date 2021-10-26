"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""${SanitizedCppName}\\editor\\scripts\\boostrap.py
Generated from O3DE PythonGem Template"""

import azlmbr
import az_qt_helpers
from PySide2 import QtCore, QtWidgets, QtGui
from PySide2.QtCore import QEvent, Qt
from PySide2.QtWidgets import QMainWindow, QAction, QDialog, QHeaderView, QLabel, QLineEdit, QPushButton, QSplitter, QTreeWidget, QTreeWidgetItem, QWidget, QAbstractButton
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class SampleUI(QtWidgets.QDialog):
    """Lightweight UI Test Class created a button"""
    def __init__(self, parent, title='Not Set'):
        super(SampleUI, self).__init__(parent)
        self.setWindowTitle(title)
        self.initUI()

    def initUI(self):
        mainLayout = QtWidgets.QHBoxLayout()
        testBtn = QtWidgets.QPushButton("I am just a Button man!")
        mainLayout.addWidget(testBtn)
        self.setLayout(mainLayout)
# -------------------------------------------------------------------------

if __name__ == "__main__":
    print("${SanitizedCppName}.boostrap, Generated from O3DE PythonGem Template")
    
    # ---------------------------------------------------------------------
    # validate pyside before continuing
    try:
        azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'IsActive')
        params = azlmbr.qt.QtForPythonRequestBus(azlmbr.bus.Broadcast, 'GetQtBootstrapParameters')
        params is not None and params.mainWindowId is not 0
        from PySide2 import QtWidgets
    except Exception as e:
        _LOGGER.error(f'Pyside not available, exception: {e}')
        raise e
    
    # keep going, import the other PySide2 bits we will use
    from PySide2 import QtGui
    from PySide2.QtCore import Slot
    from shiboken2 import wrapInstance, getCppPointer

    # Get our Editor main window
    _widget_main_window = None
    try:
        _widget_main_window = az_qt_helpers.get_editor_main_window()
    except:
        pass # may be booting in the AP?
    # ---------------------------------------------------------------------


    # ---------------------------------------------------------------------
    if _widget_main_window:
        # creat a custom menu
        _tag_str = '${SanitizedCppName}'

        # create our own menuBar
        ${SanitizedCppName}_menu = _widget_main_window.menuBar().addMenu(f"&{_tag_str}")

        # nest a menu for util/tool launching
        ${SanitizedCppName}_launch_menu = ${SanitizedCppName}_menu.addMenu("examples")
    else:
        print('No O3DE MainWindow')
    # ---------------------------------------------------------------------

    
    # ---------------------------------------------------------------------
    if _widget_main_window:
        # (1) add the first SampleUI
        action_launch_sample_ui = ${SanitizedCppName}_launch_menu.addAction("O3DE:SampleUI")

        @Slot() 
        def clicked_sample_ui():
            while 1:  # simple PySide2 test, set to 0 to disable
                ui = SampleUI(parent=_widget_main_window, title='O3DE:SampleUI')
                ui.show()
                break
            return
        # Add click event to menu bar
        action_launch_sample_ui.triggered.connect(clicked_sample_ui)
    # ---------------------------------------------------------------------
    

    # ---------------------------------------------------------------------
    if _widget_main_window:
        # (1) and custom external module Qwidget
        action_launch_${SanitizedCppName}_dialog = ${SanitizedCppName}_launch_menu.addAction("O3DE:${SanitizedCppName}_dialog")

        @Slot() 
        def clicked_${SanitizedCppName}_dialog():
            while 1:  # simple PySide2 test, set to 0 to disable
                try:
                    import az_qt_helpers
                    from ${NameLower}_dialog import ${SanitizedCppName}Dialog
                    az_qt_helpers.register_view_pane('${SanitizedCppName} Popup', ${SanitizedCppName}Dialog)
                except Exception as e:
                    print(f'Error: {e}')
                    print('Skipping register our ${SanitizedCppName}Dialog with the Editor.')
                ${SanitizedCppName}_dialog = ${SanitizedCppName}Dialog(parent=_widget_main_window)
                ${SanitizedCppName}_dialog.show()
                break
            return
        # Add click event to menu bar
        action_launch_${SanitizedCppName}_dialog.triggered.connect(clicked_${SanitizedCppName}_dialog)
    # ---------------------------------------------------------------------

    # end