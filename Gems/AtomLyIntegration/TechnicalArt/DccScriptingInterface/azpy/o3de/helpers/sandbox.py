# # coding:utf-8
# #!/usr/bin/python
# #
# # All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# # its licensors.
# #
# # For complete copyright and license terms please see the LICENSE at the root of this
# # distribution (the "License"). All use of this software is governed by the License,
# # or, if provided, by the license below or the license accompanying this file. Do not
# # remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# #

"""! @brief MockTool is a collection of template files that can be used to start a new tool within the DCCsi. """

# Required Imports
import config
import logging
from pathlib import Path
from dynaconf import settings
from PySide2 import QtWidgets, QtCore, QtGui
from SDK.Python import general_utilities as utils
from azpy.shared import qt_process
from azpy.dcc.maya.utils import maya_client

# O3DE Imports
# import azlmbr.bus as bus
# import azlmbr.components as components
# import azlmbr.editor as editor
# import azlmbr.entity as entity
# import azlmbr.math as math
#
# from PySide2.QtCore import Qt
# from PySide2.QtGui import QDoubleValidator


# Logging Formatting
_MODULENAME = 'azpy.o3de.helpers.sandbox'
_LOGGER = logging.getLogger(_MODULENAME)


class Sandbox(QtWidgets.QWidget):
    def __init__(self, *args, **kwargs):
        super(Sandbox, self).__init__()
        _LOGGER.info('Sandbox Added')
        self.main_container = QtWidgets.QVBoxLayout(self)
        self.engine_path_field = QtWidgets.QLineEdit()



def get_tool(*args, **kwargs):
    _LOGGER.info('Get tool fired')
    return Sandbox(*args, **kwargs)


if __name__ == '__main__':
    Sandbox()