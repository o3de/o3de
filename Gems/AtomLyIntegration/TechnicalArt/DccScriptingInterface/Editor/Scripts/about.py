#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! brief

An about dialog for the DccScriptingInterface

:file: DccScriptingInterface\\editor\\scripts\\about.py
"""
# standard imports
import os
from pathlib import Path
import logging as _logging
from PySide2.QtCore import Qt
from PySide2.QtGui import QDoubleValidator
from PySide2.QtWidgets import QCheckBox, QComboBox, QDialog, QFormLayout, QGridLayout, QGroupBox, QLabel, QLineEdit, QPushButton, QVBoxLayout, QWidget#
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
_DCCSI_SLUG = 'DccScriptingInterface'
_MODULENAME = 'DCCsi.editor.scripts.about'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class about_dccsi(QDialog):
    """! Creates the DcScriptingInterface About Dialog"""

    def __init__(self, parent=None):
        super(about_dccsi, self).__init__(parent)

        # set main layout
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setSpacing(20)

        dccsi_readme_url = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md'

        dccsi_maya_readme_url = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/readme.md'

        dccsi_maya_scene_exporter_url = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/Scripts/Python/scene_exporter/readme.md'

        dccsi_blender_readme_url = ''

        dccsi_blender_scene_exporter_url = ''

        dccsi_wing_readme_url = ''

        self.intro_label = QLabel()
        self.intro_label.setOpenExternalLinks(True)
        self.intro_label.setText(f'DccScriptingInterface Gem (DCCsi)\r'
                                 f'The DCCsi is an external tools integration framework, for popular technical artists tools such as Digital Content Creation tools (DCC) and python IDEs for tool development - stand up your studio tools to be configured for your project and o3de workflows.\r\r'

                                 f'This early iteration of the DCCsi includes these features:'

                                 f'More information about the DCCsi can be found here'
                                 f'<a href=\"https://beta.dreamstudio.ai/\">beta.dreamstudio.ai</a> website<br/>\r\r'

                                 f'2. Copy your API key:\r'
                                 f'<a href=\"https://beta.dreamstudio.ai/membership?tab=apiKeys/\">dreamstudio user API key</a><br/>\r\r')
        self.main_layout.addWidget(self.intro_label, 0, Qt.AlignCenter)


        self.about_label = QLabel()
        self.about_label.setWordWrap(True)
        self.about_label.setOpenExternalLinks(True)
        self.about_label.setText('The O3DE DreamStudioAI Gem was creaded by <a href=\"https://github.com/HogJonny-AMZN/\">HogJonny-AMZN</a>. Please let me know if you have ideas, enahncement requests, find any bugs, or would like to contribute. Enjoy Dreaming!')
        self.main_layout.addWidget(self.about_label, 0, Qt.AlignCenter)

        self.help_text = str("For help getting started,"
            "visit the <a href=\"https://o3de.org/docs/tools-ui/\">UI Development</a> documentation<br/>"
            "or come ask a question in the <a href=\"https://discord.gg/R77Wss3kHe\">sig-ui-ux channel</a> on Discord")

        self.help_label = QLabel()
        self.help_label.setTextFormat(Qt.RichText)
        self.help_label.setText(self.help_text)
        self.help_label.setOpenExternalLinks(True)

        self.main_layout.addWidget(self.help_label, 0, Qt.AlignCenter)

        self.setLayout(self.main_layout)
