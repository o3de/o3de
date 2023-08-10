"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import functools

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
    return (qlist.item(i) for i in range(0, qlist.count()))

SortAlpha = 0
SortRank = 1
SortCost = 2

# dict lookup accelerator (name to descriptor)
optionsByNames = {}

def getRank(name):
    return optionsByNames[name].GetOrder()

def getCost(name):
    return optionsByNames[name].GetCostEstimate()

def infoStr(name):
    return f"rank: {getRank(name)} | cost: {getCost(name)}"

def sumCost(descriptors):
    return sum((x.GetCostEstimate() for x in descriptors))

@functools.cache   # memoize
def totalCost():
    return sumCost(optionsByNames.values())

class DoubleList(QtCore.QObject):
    left = QListWidget()
    leftSubLbl = QLabel()
    right = QListWidget()
    rightSubLbl = QLabel()
    add = QPushButton(">")
    rem = QPushButton("<")
    layout = QHBoxLayout()
    changed = QtCore.Signal()
    order = SortCost

    def __init__(self, optionNamesList):
        super(DoubleList, self).__init__()
        self.left.setSelectionMode(QListWidget.MultiSelection)
        self.right.setSelectionMode(QListWidget.MultiSelection)
        for i in optionNamesList:
            self.left.addItem(i)
        self.maintainOrder()
        subV1 = QVBoxLayout()
        subV1.addWidget(self.left)
        subV1.addWidget(self.leftSubLbl)
        self.layout.addLayout(subV1)
        midstack = QVBoxLayout()
        midstack.addStretch()
        midstack.addWidget(self.add)
        midstack.addWidget(self.rem)
        midstack.addStretch()
        self.layout.addLayout(midstack)
        subV2 = QVBoxLayout()
        subV2.addWidget(self.right)
        subV2.addWidget(self.rightSubLbl)
        self.layout.addLayout(subV2)
        self.left.setStyleSheet("""QListWidget{ background: #27292D; }""")
        self.right.setStyleSheet("""QListWidget{ background: #262B35; }""")
        self.add.clicked.connect(lambda: self.addClick())
        self.rem.clicked.connect(lambda: self.remClick())
        self.refreshCountLabels()

    def addClick(self):
        transferSelection(self.left, self.right)
        self.changed.emit()
        self.refreshCountLabels()

    def remClick(self):
        transferSelection(self.right, self.left)
        self.maintainOrder()
        self.changed.emit()
        self.refreshCountLabels()

    def resetAllToLeft(self):
        self.right.selectAll()
        self.remClick()
        self.left.clearSelection()

    def maintainOrder(self):
        elems = [li.text() for li in listItems(self.left)]
        self.left.clear()
        keyGetters = [lambda x: x, getRank, getCost]
        elems.sort(reverse = self.order==SortCost, key=keyGetters[self.order])
        self.left.addItems(elems)
        for li in listItems(self.left):
            li.setToolTip(infoStr(li.text()))

    def refreshCountLabels(self):
        self.leftSubLbl.setText(str(self.left.count()) + " elements")
        self.rightSubLbl.setText(str(self.right.count()) + " elements")

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

        sortHbox = QHBoxLayout()
        sortHbox.addWidget(QLabel("Sort options by:"))
        self.sortChoices = QComboBox()
        self.sortChoices.addItem("Alphabetical")
        self.sortChoices.addItem("Rank")
        self.sortChoices.addItem("Analyzed Cost")
        self.sortChoices.setCurrentIndex(2)
        self.sortChoices.currentIndexChanged.connect(self.changeSort)
        self.sortChoices.view().setMinimumWidth(self.sortChoices.minimumSizeHint().width() * 1.4)  # fix a Qt bug that doesn't prepare enough space
        #self.sortChoices.SizeAdjustPolicy(QComboBox.AdjustToMinimumContentsLengthWithIcon)
        sortHbox.addWidget(self.sortChoices)
        sortHbox.addStretch()
        actionPaneLayout.addLayout(sortHbox)

        autoSelGp = QGroupBox("Auto selection suggestion from targeted count (prioritizing highest sorted options)")
        autoSelGpHL = QHBoxLayout()
        autoSelGp.setLayout(autoSelGpHL)
        autoSelGpHL.addWidget(QLabel("Target:"))
        self.target = QSpinBox()
        self.target.setMaximum(100000)
        self.target.setValue(32)
        autoSelGpHL.addWidget(self.target)
        suggest = QPushButton("Generate auto selection", self)
        suggest.clicked.connect(self.autogenSelectionBucket)
        autoSelGpHL.addWidget(suggest)
        autoSelGpHL.addStretch()
        actionPaneLayout.addWidget(autoSelGp)

        genBox = QGroupBox("Estimated variants generation count from current selection:")
        genBoxVL = QVBoxLayout()
        genBox.setLayout(genBoxVL)
        self.estimate = QLineEdit()
        self.estimate.setReadOnly(True)
        genBoxVL.addWidget(self.estimate)

        self.coveredLbl = QLabel()
        genBoxVL.addWidget(self.coveredLbl)
        self.makeCostLabel()

        genListBtn = QPushButton("Generate variant list", self)
        genBoxVL.addWidget(genListBtn)

        actionPaneLayout.addWidget(genBox)

        exitBtn = QPushButton("Exit", self)
        actionPaneLayout.addWidget(exitBtn)
        exitBtn.clicked.connect(self.close)

        actionPaneLayout.addStretch()

        phonywg = QWidget()
        phonywg.setLayout(actionPaneLayout)
        vsplitter.addWidget(phonywg)
        leftrightCut.addWidget(vsplitter)

        self.optionsSelector.changed.connect(self.refreshSelection)

    def changeSort(self):
        self.optionsSelector.order = self.sortChoices.currentIndex()
        self.optionsSelector.maintainOrder()

    def getParticipantOptionDescs(self):
        '''access option descriptors selected in the right bucket (in the form of a generator)'''
        return (optionsByNames[x.text()] for x in listItems(self.optionsSelector.right))

    def refreshSelection(self):
        '''triggered after a change in participants'''
        participants = self.getParticipantOptionDescs()
        # calculate the count of variants that will result from enumerating participant options
        total = 0
        for p in participants:
            if total == 0:
                total = p.GetValuesCount()  # initial value
            else:
                total = total * p.GetValuesCount()
        self.estimate.setText(str(total))
        self.makeCostLabel()

    def makeCostLabel(self):
        curSum = sumCost(self.getParticipantOptionDescs())
        total = totalCost()
        percent = curSum * 100 // total
        self.coveredLbl.setText(f"Cost covered by current selection: {curSum}/{total} ({percent}%)")

    def autogenSelectionBucket(self):
        '''reset right bucket to an automatically suggested content'''
        self.optionsSelector.resetAllToLeft()
        startBucket = listItems(self.optionsSelector.left)
        expandCount = 1
        for itemWidget in startBucket:
            newCount = expandCount * optionsByNames[itemWidget.text()].GetValuesCount()
            if newCount > self.target.value():
                break # stop here
            else:
                itemWidget.setSelected(True)
                expandCount = newCount
        self.optionsSelector.addClick()

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

    global optionsByNames
    for optDesc in optionDescriptors:
        optionsByNames[str(optDesc.GetName())] = optDesc

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
