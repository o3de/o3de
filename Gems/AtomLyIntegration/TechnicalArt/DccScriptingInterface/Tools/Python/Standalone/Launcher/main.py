# coding:utf-8
#!/usr/bin/python
# coding:utf-8
#!/usr/bin/python
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -- This line is 75 characters -------------------------------------------
# built-ins
import os
import sys
import logging as _logging

# azpy extensions
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
settings = _config.get_config_settings(setup_ly_pyside=True)

# 3rd Party (we may or do provide)
from pathlib import Path
from pathlib import PurePath

# Lumberyard extensions
from azpy.env_bool import env_bool
from azpy.constants import ENVAR_DCCSI_GDEBUG
from azpy.constants import ENVAR_DCCSI_DEV_MODE

# -------------------------------------------------------------------------
# set up global space, logging etc.
_G_DEBUG = env_bool(ENVAR_DCCSI_GDEBUG, settings.DCCSI_GDEBUG)
_DCCSI_DEV_MODE = env_bool(ENVAR_DCCSI_DEV_MODE, settings.DCCSI_GDEBUG)

for handler in _logging.root.handlers[:]:
    _logging.root.removeHandler(handler)

_MODULENAME = 'DCCsi.SDK.pythontools.launcher.main'

_log_level = _logging.INFO
if _G_DEBUG:
    _log_level = _logging.DEBUG

_LOGGER = azpy.initialize_logger(name=_MODULENAME,
                                 log_to_file=True,
                                 default_log_level=_log_level)

_LOGGER.debug('Starting up:  {0}.'.format({_MODULENAME}))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def main():
    from PySide2.QtWidgets import QApplication, QPushButton
    
    app = QApplication(sys.argv)
# -------------------------------------------------------------------------


# --------------------------------------------------------------------------
if __name__ == '__main__':
    """Run this file as main"""

app = QApplication([])  # Start an application.
window = QWidget()  # Create a window.
layout = QVBoxLayout()  # Create a layout.
button = QPushButton("I'm just a Button man") # Define a button
layout.addWidget(QLabel('Hello World!')) # Add a label
layout.addWidget(button) # Add the button man
window.setLayout(layout) # Pass the layout to the window
window.show() # Show window
app.exec_() # Execute the App
