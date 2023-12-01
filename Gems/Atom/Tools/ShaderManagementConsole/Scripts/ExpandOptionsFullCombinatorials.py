"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import functools

import azlmbr.bus
import azlmbr.shadermanagementconsole
import azlmbr.shader
import azlmbr.math
import azlmbr.name
import azlmbr.rhi
import azlmbr.atom
import azlmbr.atomtools
import GenerateShaderVariantListUtil

from PySide2 import QtWidgets
from PySide2 import QtCore
from PySide2 import QtGui
from PySide2.QtWidgets import QDialog, QVBoxLayout, QHBoxLayout, QGridLayout, QListWidget, QPushButton, QMessageBox, QLabel, QGroupBox, QComboBox, QSplitter, QWidget, QLineEdit, QSpinBox, QCheckBox, QButtonGroup, QRadioButton, QAction, QMenu

# globals
numVariantsInDocument = 0
optionsByNames = {} # dict lookup accelerator (name to descriptor)
valueRestrictions = {} # option name to list of values that are accepted for this option (absence of registration in this dict = use the whole range)

# constants
SortAlpha = 0
SortRank = 1
SortCost = 2

def nextElement(inlist, after):
    """ O(N) complexity, generic utility """
    """ nextElement(inlist=['a', 'b', 'c'], after='b') is 'c' """
    try:
        idx = inlist.index(after)
        return inlist[idx + 1]
    except:
        return after + 1  # need to return something comparable to `end` so that the test "digit > maxValue(desc)" in the `increment` function may pass

def inclusiveRange(start, end):
     return range(start, end + 1)

def isDocumentOpen(document_id: azlmbr.math.Uuid) -> bool:
    return azlmbr.atomtools.AtomToolsDocumentSystemRequestBus(
        azlmbr.bus.Broadcast, "IsDocumentOpen", document_id)

def beginEdit(document_id):
    azlmbr.atomtools.AtomToolsDocumentRequestBus(
        azlmbr.bus.Event, "BeginEdit", document_id)

def endEdit(document_id):
    azlmbr.atomtools.AtomToolsDocumentRequestBus(
        azlmbr.bus.Event, "EndEdit", document_id)

def transferSelection(qlistSrc, qlistDst):
    '''intended to work with 2 QListWidget'''
    items = qlistSrc.selectedItems()
    qlistDst.addItems([x.text() for x in items])
    for i in items:
        qlistSrc.takeItem(qlistSrc.row(i))

def listItems(qlist):
    return (qlist.item(i) for i in range(0, qlist.count()))

def getRank(name):
    return optionsByNames[name].GetOrder()

def getCost(name):
    return optionsByNames[name].GetCostEstimate()

def infoStr(name):
    '''for tooltip'''
    return f"rank: {getRank(name)} | cost: {getCost(name)}"

def sumCost(descriptors):
    return sum((x.GetCostEstimate() for x in descriptors))

@functools.cache   # memoize
def totalCost():
    return sumCost(optionsByNames.values())

def toInt(digit):
    '''extract the python integer from a boxed option description'''
    return digit.GetIndex() + 0

def createOptionValue(intVal):
    return azlmbr.shadermanagementconsole.ShaderManagementConsoleRequestBus(
        azlmbr.bus.Broadcast, 'MakeShaderOptionValueFromInt',
        intVal)

def intFromOptionValueName(optionName, valueNameStr):
    n = azlmbr.name.Name(valueNameStr)
    return toInt(optionsByNames[optionName].FindValue(n))

def hasRestrictions(optionName):
    return optionName in valueRestrictions \
        and len(valueRestrictions[optionName]) < optionsByNames[optionName].GetValuesCount() # if everything is "checked" that's no restriction

# check if a value should be skipped for counting (not included in enumeration because it's unchecked)
def isValueRestricted(optionName, valueNameStr):
    valAsInt = intFromOptionValueName(optionName, valueNameStr)
    if hasRestrictions(optionName):
        return valAsInt not in valueRestrictions[optionName]
    return False

# "virtualize" access to GetMinValue to reflect the potential value-space restriction
def getMinValue(descriptor):
    global valueRestrictions
    name = str(descriptor.GetName())
    if hasRestrictions(name):
        return createOptionValue(valueRestrictions[name][0])
    else:
        return descriptor.GetMinValue()

# "virtualize" access to GetMaxValue to reflect the potential value-space restriction
def getMaxValue(descriptor):
    global valueRestrictions
    name = str(descriptor.GetName())
    if hasRestrictions(name):
        return createOptionValue(valueRestrictions[name][-1])
    else:
        return descriptor.GetMaxValue()

# same concept as above
def getValuesCount(descriptor):
    global valueRestrictions
    name = str(descriptor.GetName())
    if hasRestrictions(name):
        return len(valueRestrictions[name])
    else:
        return descriptor.GetValuesCount()

# "virtualize" `increment by one` to be able to jump over restricted values
def getNextValueInt(descriptor, digit):
    global valueRestrictions
    name = str(descriptor.GetName())
    if hasRestrictions(name):
        possibles = valueRestrictions[name]
        return nextElement(inlist=possibles, after=digit)
    else:
        return digit + 1

# small window to select sub-ranges into an option's possible values
class ValueSelector(QDialog):
    def __init__(self, optionName, optionValuesNames):
        super().__init__()
        self.setWindowTitle("[" + optionName + "] - restrict enumerated values")
        self.optionName = optionName
        self.optionValuesNames = optionValuesNames
        self.initUI()

    def initUI(self):
        mainvl = QVBoxLayout(self)
        self.valueList = QListWidget()
        for idx, optVN in enumerate(self.optionValuesNames):
            self.valueList.insertItem(idx, optVN)
            added = self.valueList.item(idx)
            added.setFlags(added.flags() | QtCore.Qt.ItemIsUserCheckable)
            added.setCheckState(QtCore.Qt.Unchecked if isValueRestricted(self.optionName, optVN) else QtCore.Qt.Checked)
        mainvl.addWidget(self.valueList)
        twoBtnsHL = QHBoxLayout(self)
        self.cancel = QPushButton("Cancel")
        twoBtnsHL.addWidget(self.cancel)
        self.ok = QPushButton("Ok")
        twoBtnsHL.addWidget(self.ok)
        mainvl.addLayout(twoBtnsHL)
        self.cancel.clicked.connect(self.close)
        self.ok.clicked.connect(self.storeRestriction)
        self.setMaximumWidth(500)
        self.setMaximumHeight(1100)
        self.resize(380, 300)

    # on OK click
    def storeRestriction(self):
        '''update the global dictionary of usable values for this option'''
        global valueRestrictions
        restrictedList = [intFromOptionValueName(self.optionName, x.text()) for x in listItems(self.valueList) if x.checkState() == QtCore.Qt.Checked]
        if len(restrictedList) == 0:
            msgBox = QMessageBox()
            msgBox.setText("Keep at least one value")
            msgBox.setInformativeText("Nothing to enumerate: just remove the option from the pariticipants altogether.")
            msgBox.setStandardButtons(QMessageBox.Ok)
            msgBox.exec()
            return
        # save:
        valueRestrictions[self.optionName] = restrictedList
        self.accept() # exit dialog

# that's the control that holds 2 face to face lists doing communicating vases
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
        self.right.viewport().installEventFilter(self)
        self.right.setMouseTracking(True)

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

    def refreshLabelColors(self):  # to mark value-restricted options
        for elem in listItems(self.right):
            if hasRestrictions(elem.text()):
                elem.setForeground(QtGui.QColor(0xbf, 0x8f, 0xff)) # mauve
            else:
                elem.setForeground(QtGui.QBrush()) # default

    def eventFilter(self, source, event):
        if event.type() == QtCore.QEvent.ContextMenu and source is self.right.viewport():
            menu = QMenu()
            menu.addAction("Customize generated values")
            if menu.exec_(event.globalPos()):
                self.restrictValueSpace()
                return True
        return False

    def restrictValueSpace(self):
        items = self.right.selectedItems()
        if len(items) == 1:
            optName = items[0].text()
            desc = optionsByNames[optName]
            optionValuesNames = [desc.GetValueName(createOptionValue(v)).ToString() for v in inclusiveRange(toInt(desc.GetMinValue()), toInt(desc.GetMaxValue()))]
            subdialog = ValueSelector(optName, optionValuesNames)
            subdialog.setModal(True)
            if subdialog.exec() == QDialog.Accepted:
                self.changed.emit()
                self.refreshLabelColors()

class Dialog(QDialog):
    def __init__(self, options):
        super().__init__()
        self.optionDescriptors = options
        self.initUI()
        self.participantNames = []
        self.listOfDigitArrays = []

    def initUI(self):
        self.setWindowTitle("Full variant combinatorics enumeration for select options")

        # Create the layout for the dialog box
        mainvl = QVBoxLayout(self)
        leftrightCut = QHBoxLayout()
        mainvl.addLayout(leftrightCut)

        # Create the list box on the left
        self.optionsSelector = DoubleList([x.GetName().ToString() for x in self.optionDescriptors])
        listGroup = QGroupBox("Add desired participating options from the left bucket, to the selection bucket on the right. Right click on options in the right bucket to further configure used values.")
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

        mixOptBox = QGroupBox("Generation method:")
        mixOptBoxHL = QHBoxLayout()
        mixOptBox.setLayout(mixOptBoxHL)
        self.mixOpt_append = QRadioButton("Append")
        self.mixOpt_append.setToolTip("Just add the generated combinations at the end of the variant list, with non-participating options set as dynamic")
        self.mixOpt_append.setChecked(True)
        self.mixOpt_multiply = QRadioButton("Multiply")
        self.mixOpt_multiply.setToolTip("generated_combinatorials x current_variants (i.e combine by mixing)")
        mixOptGp = QButtonGroup()
        mixOptGp.addButton(self.mixOpt_append)
        mixOptGp.addButton(self.mixOpt_multiply)
        self.mixOpt_append.clicked.connect(self.refreshSelection)
        self.mixOpt_multiply.clicked.connect(self.refreshSelection)
        mixOptBoxHL.addWidget(self.mixOpt_append)
        mixOptBoxHL.addWidget(self.mixOpt_multiply)
        actionPaneLayout.addWidget(mixOptBox)

        genBox = QGroupBox("Estimations from current selection:")
        genBoxGL = QGridLayout()
        genBox.setLayout(genBoxGL)
        genBoxGL.addWidget(QLabel("Generation count: "), 0,0)
        self.directGenCount = QLineEdit()
        self.directGenCount.setReadOnly(True)
        genBoxGL.addWidget(self.directGenCount, 0,1)
        genBoxGL.addWidget(QLabel("Final variant count: "), 1,0)
        self.totalVariantsPostSend = QLineEdit()
        self.totalVariantsPostSend.setReadOnly(True)
        genBoxGL.addWidget(self.totalVariantsPostSend, 1,1)
        actionPaneLayout.addLayout(genBoxGL)

        self.coveredLbl = QLabel()
        genBoxGL.addWidget(self.coveredLbl, 2,0)
        self.makeCostLabel()

        actionPaneLayout.addWidget(genBox)

        genListBtn = QPushButton("Generate variant list", self)
        genListBtn.clicked.connect(self.generateVariants)
        genListBtn.setToolTip("Enumerate the variants*options matrix, and send to document")
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

    def changeSort(self):
        self.optionsSelector.order = self.sortChoices.currentIndex()
        self.optionsSelector.maintainOrder()

    def getParticipantOptionDescs(self):
        '''access option descriptors selected in the right bucket (in the form of a generator)'''
        return (optionsByNames[x.text()] for x in listItems(self.optionsSelector.right))

    def calculateCombinationCountInducedByCurrentParticipants(self):
        '''calculate the count of variants that will result from enumerating participant options'''
        participants = self.getParticipantOptionDescs()
        count = 0
        for p in participants:
            if count == 0:
                count = getValuesCount(p)  # initial value
            else:
                count = count * getValuesCount(p)
        return count

    def calculateResultVariantCountAfterExpedite(self, calculatedGenCount):
        if self.mixOpt_append.isChecked():
            return numVariantsInDocument + calculatedGenCount
        elif self.mixOpt_multiply.isChecked():
            return numVariantsInDocument * calculatedGenCount

    def makeCostLabel(self):
        curSum = sumCost(self.getParticipantOptionDescs())
        total = totalCost()
        percent = 0 if total == 0 else curSum * 100 // total
        self.coveredLbl.setText(f"Cost covered by current selection: {curSum}/{total} ({percent}%)")

    def refreshSelection(self):
        '''triggered after a change in participants'''
        count = self.calculateCombinationCountInducedByCurrentParticipants()
        self.directGenCount.setText(str(count))
        self.makeCostLabel()
        self.totalVariantsPostSend.setText(str(self.calculateResultVariantCountAfterExpedite(count)))

    def autogenSelectionBucket(self):
        '''reset right bucket to an automatically suggested content'''
        self.optionsSelector.resetAllToLeft()
        startBucket = listItems(self.optionsSelector.left)
        expandCount = 1
        for itemWidget in startBucket:
            newCount = expandCount * getValuesCount(optionsByNames[itemWidget.text()])
            if newCount > self.target.value():
                break # stop here
            else:
                itemWidget.setSelected(True)
                expandCount = newCount
        self.optionsSelector.addClick()

    # requirement len(digitArray) == len(descriptorArray)
    # digits are integers (ShaderOptionValue). descriptors are RPI::ShaderOptionDescriptor
    @staticmethod
    def allMaxedOut(digitArray, descriptorArray):
        '''verify if an array of option-values, has all values corresponding to the described max-value, for their respective option'''
        zipped = zip(digitArray, descriptorArray)
        return all((digit == toInt(getMaxValue(desc)) for digit, desc in zipped))

    @staticmethod
    def increment(digit, descriptor):
        '''increment one digit in its own base-space. return pair or new digit and carry bit'''
        carry = False
        digit = getNextValueInt(descriptor, digit)
        if digit > toInt(getMaxValue(descriptor)):
            digit = toInt(getMinValue(descriptor))
            carry = True
        return (digit, carry)

    @staticmethod
    def incrementArray(digitArray, descriptorArray):
        '''+1 operation in the digitArray'''
        carry = True  # we consider that the LSB is always to increment
        for i in reversed(range(0, len(digitArray))):
            if carry:
                digitArray[i], carry = Dialog.increment(digitArray[i], descriptorArray[i])

    @staticmethod
    def toNames(digitArray, descriptorArray):
        return [desc.GetValueName(createOptionValue(digit)) for digit, desc in zip(digitArray, descriptorArray)]

    def generateVariants(self):
        '''make a list of azlmbr.shader.ShaderVariantInfo that fully enumerate the combinatorics of the selected options space'''
        steps = self.calculateCombinationCountInducedByCurrentParticipants()
        progressDialog = QtWidgets.QProgressDialog("Enumerating", "Cancel", 0, steps)
        progressDialog.setMaximumWidth(400)
        progressDialog.setMaximumHeight(100)
        progressDialog.setModal(True)
        progressDialog.setWindowTitle("Generate variants")

        genOptDescs = list(self.getParticipantOptionDescs())
        self.participantNames = [x.GetName() for x in genOptDescs]
        self.listOfDigitArrays = []
        digits = [toInt(getMinValue(desc)) for desc in genOptDescs]
        c = 0
        if len(digits) > 0:
            while(True):
                self.listOfDigitArrays.extend(self.toNames(digits, genOptDescs))  # flat list
                if Dialog.allMaxedOut(digits, genOptDescs):
                    break # we are done enumerating the "digit" space
                Dialog.incrementArray(digits, genOptDescs)  # go to next, doing digit cascade
                progressDialog.setValue(c)
                c = c + 1
                if progressDialog.wasCanceled():
                    self.listOfDigitArrays = []
                    return
        progressDialog.close()

def main():
    print("==== Begin shader variant expand option combinatorials script ====")

    if len(sys.argv) < 2:
        print(f"argv count is {len(sys.argv)}. The script requires a documentID as input argument.")
        return

    documentId = azlmbr.math.Uuid_CreateString(sys.argv[1])

    if not isDocumentOpen(documentId):
        print(f"document ID {documentId} not opened")
        return

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
        optionsByNames[optDesc.GetName().ToString()] = optDesc

    # Get current variant list to append our expansion after it
    variantList = azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
        azlmbr.bus.Event,
        'GetShaderVariantListSourceData',
        documentId
    )

    global numVariantsInDocument
    numVariantsInDocument = len(variantList.shaderVariants)

    dialog = Dialog(optionDescriptors)
    try:
        dialog.exec()
    except:
        print("exited early due to exception")
        return

    numVal = len(dialog.listOfDigitArrays)
    if numVal == 0:
        return

    mode = "append" if dialog.mixOpt_append.isChecked() else ("multiply" if dialog.mixOpt_multiply.isChecked() else "<error>")
    print(f"sending {numVal} values. ({numVal/len(dialog.participantNames)} new variants). in '{mode}' mode")

    beginEdit(documentId)
    # passing the result
    if dialog.mixOpt_append.isChecked():  # append mode
        azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
            azlmbr.bus.Event,
            'AppendSparseVariantSet',
            documentId,
            dialog.participantNames,
            dialog.listOfDigitArrays
        )
    elif dialog.mixOpt_multiply.isChecked():  # mix mode (enumerate the new variants fully with the old ones)
        azlmbr.shadermanagementconsole.ShaderManagementConsoleDocumentRequestBus(
            azlmbr.bus.Event,
            'MultiplySparseVariantSet',
            documentId,
            dialog.participantNames,
            dialog.listOfDigitArrays
        )
    endEdit(documentId)


#if __name__ == "__main__":
main()

print("==== End shader variant script ====")