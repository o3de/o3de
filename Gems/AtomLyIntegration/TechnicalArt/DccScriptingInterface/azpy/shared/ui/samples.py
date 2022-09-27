#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! Thos module contains some lightweight PySide2 sample ui code.

:file: < DCCsi >\\azpy\\shared\\ui\\samples.py
:Status: Prototype
:Version: 0.0.1
"""
# -------------------------------------------------------------------------
# standard imports
from pathlib import Path
import logging as _logging
#PySide2 imports
from PySide2 import QtWidgets
# -------------------------------------------------------------------------
# global scope
_MODULENAME = 'azpy.shared.ui.samples'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))

_MODULE_PATH = Path(__file__)
_LOGGER.debug(f'_MODULE_PATH: {_MODULE_PATH.as_posix()}')
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
