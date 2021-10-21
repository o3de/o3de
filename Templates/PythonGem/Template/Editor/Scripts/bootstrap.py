"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""${SanitizedCppName}\\editor\\scripts\\boostrap.py
Generated from O3DE PythonGem Template"""

import azlmbr

# -------------------------------------------------------------------------
def get_o3de_mainwindow():
    widget_main_window = QtWidgets.QWidget.find(params.mainWindowId)
    widget_main_window = wrapInstance(int(getCppPointer(widget_main_window)[0]), QtWidgets.QMainWindow)
    return widget_main_window
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
    _widget_main_window = get_o3de_mainwindow()
    # ---------------------------------------------------------------------

    
    # ---------------------------------------------------------------------
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
    @Slot() 
    def clicked_${SanitizedCppName}_dialog():
        while 1:  # simple PySide2 test, set to 0 to disable
            try:
                import az_qt_helpers
                from ${SanitizedCppName}_dialog import ${SanitizedCppName}Dialog
                az_qt_helpers.register_view_pane('${SanitizedCppName} Popup', ${SanitizedCppName}Dialog)
            except:
                print('Skipping register our ${SanitizedCppName}Dialog with the Editor.')
            ${SanitizedCppName}_dialog = ${SanitizedCppName}Dialog(parent=editor_main_window)
            ${SanitizedCppName}_dialog.show()
        break
    return
    # Add click event to menu bar
    action_launch_${SanitizedCppName}_dialog.triggered.connect(clicked_${SanitizedCppName}_dialog)
    # ---------------------------------------------------------------------

    # end