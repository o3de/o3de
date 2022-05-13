# Temporary only. Modify these two values to match your locations.

# Location of the unpacked new version of pyside.
package_path='C:\\ly\\3rd_party\\packages\\pyside2-qt-5.15.2-rev1-windows\\pyside2'

# Location of the top directory of the o3de project.
project_path='C:\\ly\\git\\o3de'

import sys

# Add the path to the new version of pyside. Modify this to match your location.
sys.path.append(package_path)

sys.path.append('.')

# Now the path is correct, import Qt libs.
from PySide2 import QtWidgets, QtCore

# Set up a QApplication.
app = QtWidgets.QApplication()

# Import AzQtComponents module. Doing this before creating the app will break it.
import azqtpyside
from azqtpyside import AzQtComponents as az

# Create a window and add some content.
win = az.DockMainWindow()

centralWidget = QtWidgets.QWidget(win);
layout = QtWidgets.QVBoxLayout(centralWidget);
layout.setMargin(0);
layout.setContentsMargins(0, 0, 0, 0);
centralWidget.setLayout(layout);
win.setCentralWidget(centralWidget);
        
one = QtWidgets.QPushButton("One")
two = QtWidgets.QPushButton("two")
three = QtWidgets.QPushButton("three")

layout.addWidget(one)
layout.addWidget(two)
layout.addWidget(three)

# Set up an event loop to wait for the window to close.
win.setAttribute(QtCore.Qt.WA_DeleteOnClose)
eventLoop = QtCore.QEventLoop()
win.destroyed.connect(eventLoop.quit)

# Set up the StyleManager and load the stylesheet so that window looks correct.
style = az.StyleManager(win)

style.initializePath(project_path)
style.setStyleSheet(win, 'Code\\Editor\\Style\\Editor.qss')

win.resize(600, 400)
win.show()

# Wait for the window to close.
eventLoop.exec_()