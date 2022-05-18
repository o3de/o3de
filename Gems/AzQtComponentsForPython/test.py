# Temporary only. Modify these two values to match your locations.

# Location of the unpacked new version of pyside.
package_path='C:\\ly\\3rd_party\\packages\\pyside2-qt-5.15.2-rev1-windows\\pyside2'

# Location of the top directory of the o3de project.
project_path='C:\\ly\\git\\o3de'


import sys
import os

if not os.path.exists(package_path):
    print("Error: pyside2-qt-5.15.2-rev1-windows not found")
    sys.exit()
    
toRemove = []

# Look for the old pyside module being in the path and remove it if it is - remove this when new package is official.
for pathItem in sys.path:
    if "pyside2-qt-5.15.1-rev2" in pathItem:
        sys.path.remove(pathItem)

#sys.path.pop()

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
dialog = az.DockMainWindow()

centralWidget = QtWidgets.QWidget(dialog);

layout = QtWidgets.QVBoxLayout(centralWidget);
layout.setMargin(0);
layout.setContentsMargins(0, 0, 0, 0);
centralWidget.setLayout(layout);

dialog.setCentralWidget(centralWidget);

buttonOne = QtWidgets.QPushButton("One")
buttonTwo = QtWidgets.QPushButton("Two")
buttonThree = QtWidgets.QPushButton("Three")

layout.addWidget(buttonOne)
layout.addWidget(buttonTwo)
layout.addWidget(buttonThree)

# Set up an event loop to wait for the window to close.
dialog.setAttribute(QtCore.Qt.WA_DeleteOnClose)
eventLoop = QtCore.QEventLoop()
dialog.destroyed.connect(eventLoop.quit)

# Set up the StyleManager and load the stylesheet so that window looks correct.
style = az.StyleManager(dialog)

style.initializePath(project_path)
style.setStyleSheet(dialog, 'Code\\Editor\\Style\\Editor.qss')

dialog.resize(600, 400)
dialog.show()

# Wait for the window to close.
eventLoop.exec_()