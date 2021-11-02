"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
# -------------------------------------------------------------------------
"""${SanitizedCppName}\\editor\\scripts\\${SanitizedCppName}_dialog.py
Generated from O3DE PythonToolGem Template"""

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QDialog, QLabel, QVBoxLayout

class ${SanitizedCppName}Dialog(QDialog):
    def __init__(self, parent=None):
        super(${SanitizedCppName}Dialog, self).__init__(parent)

        self.mainLayout = QVBoxLayout(self)

        self.introLabel = QLabel("Put your cool stuff here!")

        self.mainLayout.addWidget(self.introLabel, 0, Qt.AlignCenter)

        self.helpText = str("For help getting started,"
            "visit the <a href=\"https://o3de.org/docs/tools-ui/ui-dev-intro/\">UI Development</a> documentation<br/>"
            "or come ask a question in the <a href=\"https://discord.gg/R77Wss3kHe\">sig-ui-ux channel</a> on Discord")

        self.helpLabel = QLabel()
        self.helpLabel.setTextFormat(Qt.RichText)
        self.helpLabel.setText(self.helpText)
        self.helpLabel.setOpenExternalLinks(True)

        self.mainLayout.addWidget(self.helpLabel, 0, Qt.AlignCenter)

        self.setLayout(self.mainLayout)
