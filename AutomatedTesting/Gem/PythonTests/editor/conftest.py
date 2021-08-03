"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import pytest
import winreg
import automatedtesting_shared.registry_utils as reg


logger = logging.getLogger(__name__)


layout = {
    'path': r'Software\O3DE\O3DE\Editor\fancyWindowLayouts',
    'value': 'last'
}
restore_camera = {
    'new': 16384,
    'path': r'Software\O3DE\O3DE\Editor\AutoHide',
    'value': 'ViewportCameraRestoreOnExitGameMode'
}


@pytest.fixture(autouse=True)
def set_editor_registry_defaults(request):
    # Records editor settings at start, sets to default, then returns to original at teardown.
    logger.debug('Executing an Editor settings fixture. If not executing an Editor test, this may be in error.')

    layout['original'] = reg.get_ly_registry_value(layout['path'], layout['value'])
    restore_camera['original'] = reg.get_ly_registry_value(restore_camera['path'], restore_camera['value'])

    # Deleting current layout value to restore defaults
    reg.delete_ly_registry_value(layout['path'], layout['value'])

    # Setting restore camera dialog to not display
    reg.set_ly_registry_value(restore_camera['path'], restore_camera['value'], restore_camera['new'])

    # Revert settings to original values
    def teardown():
        reg.set_ly_registry_value(layout['path'], layout['value'], layout['original'], value_type=winreg.REG_BINARY)
        reg.set_ly_registry_value(restore_camera['path'], restore_camera['value'], restore_camera['original'])

    request.addfinalizer(teardown)
