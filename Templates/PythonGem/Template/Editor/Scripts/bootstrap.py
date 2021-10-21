"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""Generated from O3DE PythonGem Template"""

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
    print("Generated from O3DE PythonGem Template")
    
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
    def clicked_generic_dialog():
        while 1:  # simple PySide2 test, set to 0 to disable
            try:
                import az_qt_helpers
                from custom_dialog import GenericDialog
                az_qt_helpers.register_view_pane('Generic Popup', GenericDialog)
            except:
                print('Skipping register our GenericDialog with the Editor.')
            custom_dialog = GenericDialog(parent=editor_main_window)
            custom_dialog.show()
        break
    return
    # Add click event to menu bar
    action_launch_generic_dialog.triggered.connect(clicked_generic_dialog)
    # ---------------------------------------------------------------------

    # end