#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# -------------------------------------------------------------------------

"""! Manages a QSettings for a GUI tool

:file: < DCCsi >/azpy/shared/ui/settings.py
:Status: Prototype
:Version: 0.0.1
"""
# built in's
import os
import logging as _logging

# 3rd Party (we may provide)
from unipath import Path
from dynaconf import settings

# azpy extensions
import azpy.config_utils
_config = azpy.config_utils.get_dccsi_config()
# ^ this is effectively an import and retreive of <dccsi>\config.py
# init lumberyard Qy/PySide2 access
_config.init_o3de_pyside(settings.O3DE_DEV)

# now we can import lumberyards PySide2
import PySide2.QtCore as QtCore
import PySide2.QtWidgets as QtWidgets

# -------------------------------------------------------------------------
#  global space debug flag
_DCCSI_GDEBUG = settings.DCCSI_GDEBUG

#  global space debug flag
_DCCSI_DEV_MODE = settings.DCCSI_DEV_MODE

_MODULE_PATH = Path(__file__)

_ORG_TAG = 'Amazon_Lumberyard'
_APP_TAG = 'DCCsi'
_TOOL_TAG = 'azpy.shared.ui.qt_settings'
_TYPE_TAG = 'test'

_MODULENAME = _TOOL_TAG
_LOGGER = _logging.getLogger(_MODULENAME)
_LOGGER.debug('Something invoked :: {0}.'.format(_MODULENAME))
# -------------------------------------------------------------------------


# -------------------------------------------------------------------------
def createSettings(org='Amazon_Lumberyard', app='DCCsi',
                   tool='azpy', type='default'):
    """Sets up a settings .ini

    Returns a QSettings instance"""

    settings_folder = '{org}//{app}'.format(org=org, app=app)
    settings_name = '{tool}-{type}'.format(tool=tool, type=type)

    settings = QtCore.QSettings(QtCore.QSettings.IniFormat,
                                QtCore.QSettings.UserScope,
                                settings_folder, settings_name)

    return settings
# -------------------------------------------------------------------------


if __name__ == '__main__':
    """Run this file as main"""
    import sys

    app = QtWidgets.QApplication(sys.argv)
    app.setOrganizationName(_ORG_TAG)
    app.setApplicationName('{app}:{tool}'.format(app=_APP_TAG, tool=_TOOL_TAG))

    test_esttings = createSettings(_ORG_TAG, _APP_TAG, _TOOL_TAG, _TYPE_TAG)
    _LOGGER.info(test_esttings)
    _LOGGER.info(test_esttings.fileName())
