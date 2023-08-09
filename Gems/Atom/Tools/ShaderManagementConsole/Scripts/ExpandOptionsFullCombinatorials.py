"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys

sys.path.append("D:/o3de/Gems/Atom/Tools/ShaderManagementConsole/Scripts") # TODO remove

import azlmbr.bus
import azlmbr.shadermanagementconsole
import azlmbr.shader
import azlmbr.math
import GenerateShaderVariantListUtil
from PySide2 import QtWidgets
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2.QtWidgets import QDialog, QVBoxLayout, QHBoxLayout, QListWidget, QPushButton, QMessageBox, QLabel, QGroupBox, QComboBox, QSplitter, QWidget, QLineEdit, QSpinBox

def transferSelection(qlistSrc, qlistDst):
    '''intended to work with 2 QListWidget'''
    items = qlistSrc.selectedItems()
    qlistDst.addItems([x.text() for x in items])
    for i in items:
        qlistSrc.takeItem(qlistSrc.row(i))

def listItems(qlist):
    return [qlist.item(i) for i in range(0, qlist.count())]

class DoubleList(QtCore.QObject):
    left = QListWidget()
    right = QListWidget()
    add = QPushButton(">")
    rem = QPushButton("<")
    layout = QHBoxLayout()
    changed = QtCore.Signal()

    def __init__(self, optionNamesList):
        super(DoubleList, self).__init__()
        self.left.setSelectionMode(QListWidget.MultiSelection)
        self.right.setSelectionMode(QListWidget.MultiSelection)
        for i in optionNamesList:
            self.left.addItem(i)
        self.layout.addWidget(self.left)
        midstack = QVBoxLayout()
        midstack.addStretch()
        midstack.addWidget(self.add)
        midstack.addWidget(self.rem)
        midstack.addStretch()
        self.layout.addLayout(midstack)
        self.layout.addWidget(self.right)
        self.left.setStyleSheet("""QListWidget{ background: #27292D; }""")
        self.right.setStyleSheet("""QListWidget{ background: #262B35; }""")
        self.add.clicked.connect(lambda: self.addClick())
        self.rem.clicked.connect(lambda: self.remClick())

    def addClick(self):
        transferSelection(self.left, self.right)
        self.changed.emit()

    def remClick(self):
        transferSelection(self.right, self.left)
        self.changed.emit()

class Dialog(QDialog):
    def __init__(self, options):
        super().__init__()
        self.optionDescriptors = options
        self.initUI()

    def initUI(self):
        self.setWindowTitle("Full variant expansion for select options")

        # Create the layout for the dialog box
        mainvl = QVBoxLayout(self)
        leftrightCut = QHBoxLayout()
        mainvl.addLayout(leftrightCut)

        # Create the list box on the left
        self.optionsSelector = DoubleList([str(x.GetName()) for x in self.optionDescriptors])
        listGroup = QGroupBox("Add desired participating options from the left bucket, to the selection bucket on the right:")
        listGroup.setLayout(self.optionsSelector.layout)
        vsplitter = QSplitter(QtCore.Qt.Horizontal)
        vsplitter.addWidget(listGroup)
        leftrightCut.addWidget(vsplitter)

        # Create the buttons on the right
        actionPaneLayout = QVBoxLayout()

        actionPaneLayout.addWidget(QLabel("Display sorting method:"))
        sortChoices = QComboBox()
        sortChoices.addItem("Alphabetical")
        sortChoices.addItem("Rank")
        sortChoices.addItem("Analyzed Cost")
        sortChoices.setCurrentIndex(2)
        actionPaneLayout.addWidget(sortChoices)

        autoLbl = QLabel("Auto selection suggestion from targeted count (prioritizing highest sorted options)")
        actionPaneLayout.addWidget(autoLbl)
        miniHbox = QHBoxLayout()
        target = QSpinBox()
        target.setMaximum(100000)
        target.setValue(32)
        miniHbox.addWidget(target)
        suggest = QPushButton("Generate selection", self)
        miniHbox.addWidget(suggest)
        actionPaneLayout.addLayout(miniHbox)

        estLbl = QLabel("Estimated variants generation count from current selection:")
        actionPaneLayout.addWidget(estLbl)
        self.estimate = QLineEdit()
        self.estimate.setReadOnly(True)
        actionPaneLayout.addWidget(self.estimate)

        genListBtn = QPushButton("Generate variant list", self)
        actionPaneLayout.addWidget(genListBtn)

        exitBtn = QPushButton("Exit", self)
        actionPaneLayout.addWidget(exitBtn)
        exitBtn.clicked.connect(self.close)

        actionPaneLayout.addStretch()

        phonywg = QWidget()
        phonywg.setLayout(actionPaneLayout)
        vsplitter.addWidget(phonywg)
        leftrightCut.addWidget(vsplitter)

        self.optionsSelector.changed.connect(self.refreshSelection)

        # dict lookup accelerator
        self.optionsByNames = {}
        for optDesc in self.optionDescriptors:
            self.optionsByNames[str(optDesc.GetName())] = optDesc

    def refreshSelection(self):
        participants = [self.optionsByNames[x.text()] for x in listItems(self.optionsSelector.right)]
        if len(participants) == 0:
            self.estimate.setValue(0)
        else:
            total = participants[0].GetValuesCount()  # expansion space counter, initial value
            for p in participants[1:]:
                total = total * p.GetValuesCount()
            self.estimate.setText(str(total))

def main():
    print("==== Begin shader variant expand option combinatorials script ====")

    if externalArgv: sys.argv.append(externalArgv)

    if len(sys.argv) < 2:
        print(f"argv count is {len(sys.argv)}. The script requires a documentID as input argument.")
        return

    documentId = azlmbr.math.Uuid_CreateString(sys.argv[1])

    print(f"getting options for uuid {documentId}")

    # Get options
    optionCount = azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
        azlmbr.bus.Event, 'GetShaderOptionDescriptorCount',
        documentId)

    if optionCount == 0:
        print("No options to work with on that document")
        return

    optionDescriptors = [None] * optionCount
    for i in range(0, optionCount):
        optionDescriptors[i] = azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
            azlmbr.bus.Event, 'GetShaderOptionDescriptor',
            documentId, i)
        if optionDescriptors[i] is None:
            print(f"Error: Couldn't access option descriptor at {i}")
            return
    print(f"got list of {len(optionDescriptors)} options")

    dialog = Dialog(optionDescriptors)
    try:
        dialog.exec()
    except:
        return

    # Update shader variant list
    # azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
    #     azlmbr.bus.Event,
    #     'SetShaderVariantListSourceData',
    #     documentId,
    #     shaderVariantList
    # )
    
    print("==== End shader variant script ====")

#if __name__ == "__main__":
main()
