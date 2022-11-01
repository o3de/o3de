#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------
"""! brief: An about dialog for the DccScriptingInterface

:file: DccScriptingInterface\\editor\\scripts\\about.py
"""
# standard imports
import os
from pathlib import Path
import logging as _logging
from PySide2.QtCore import Qt
from PySide2.QtGui import QDoubleValidator
from PySide2.QtWidgets import QCheckBox, QComboBox, QDialog, QFormLayout, QGridLayout, QGroupBox, QLabel, QLineEdit, QPushButton, QVBoxLayout, QWidget, QSizePolicy
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
# global scope
from DccScriptingInterface.Editor.Scripts import _PACKAGENAME
_MODULENAME = f'{_PACKAGENAME}.about'
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Initializing: {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
class DccsiAbout(QDialog):
    """! Creates the DcScriptingInterface About Dialog"""

    def __init__(self, parent=None):
        super(DccsiAbout, self).__init__(parent)

        # set main layout
        self.main_layout = QVBoxLayout(self)
        self.main_layout.setSpacing(20)

        dccsi_readme_url = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/readme.md'

        dccsi_maya_readme_url = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/readme.md'

        dccsi_maya_scene_exporter_url = 'https://github.com/o3de/o3de/tree/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Maya/Scripts/Python/scene_exporter/readme.md'

        dccsi_blender_readme_url = 'https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/readme.md'

        dccsi_blender_scene_exporter_url = 'https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/DCC/Blender/Scripts/AddOns/SceneExporter/README.md'

        dccsi_wing_readme_url = 'https://github.com/o3de/o3de/blob/development/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/IDE/Wing/readme.md'

        self.intro_label = QLabel()
        self.intro_label.setOpenExternalLinks(True)
        self.intro_label.setText(f"About the O3DE, DccScriptingInterface Gem (DCCsi)<br/><br/>"

                                 f"The DCCsi is an external tools integration framework. It includes popular technical artists tools such as Digital Content Creation tools (DCC). As well as python IDEs for tool development.<br/><br/>"

                                 f"Stand up your studio tools to be configured for your project and o3de workflows.<br/><br/>"

                                 f"This early iteration of the DCCsi includes these features:<br/>"

                                 f"<a href=\'{dccsi_readme_url}/\'>DccScriptingInterface (DCCsi): Readme.md</a><br/>"

                                 f"<a href=\'{dccsi_maya_readme_url}/\'>DCCsi Maya: Readme.md</a><br/>"

                                 f"<a href=\'{dccsi_maya_scene_exporter_url}/\'>DCCsi Maya, Scene Exporter: Readme.md</a><br/>"

                                 f"<a href=\'{dccsi_blender_readme_url}/\'>DCCsi Blender: Readme.md</a><br/>"

                                 f"<a href=\'{dccsi_blender_scene_exporter_url}/\'>DCCsi Blender, Scene Exporter: Readme.md</a><br/>"

                                 f"<a href=\'{dccsi_wing_readme_url}/\'>DCCsi Wing IDE: Readme.md</a><br/><br/>"

                                 f"For help getting started, visit the <a href=\"{dccsi_readme_url}/\">DccScriptingInterface</a> documentation<br/>"

                                 f'or come ask a question in the <a href=\"https://discord.com/channels/805939474655346758/842110573625081876\">O3DE #TechnicalArtists Channel</a> on Discord.</a><br/><br/>'

                                 f'The DccScriptingInterface Gem was created by <a href=\"https://github.com/HogJonny-AMZN/\">HogJonny-AMZN</a> Along with other AWS Design Technologists. Please let me know if you have ideas, enhancement requests, find any bugs, or would like to contribute.<br/><br/>'

                                 f'Enjoy building your studio toolchain!</a><br/><br/>')

        size_policy = QSizePolicy(QSizePolicy.Minimum, QSizePolicy.Minimum)
        size_policy.setHorizontalStretch(0)
        size_policy.setVerticalStretch(0)

        self.intro_label.setSizePolicy(size_policy)
        self.intro_label.setWordWrap(True)

        self.main_layout.addWidget(self.intro_label, 0, Qt.AlignCenter)

        self.setLayout(self.main_layout)
# -------------------------------------------------------------------------
