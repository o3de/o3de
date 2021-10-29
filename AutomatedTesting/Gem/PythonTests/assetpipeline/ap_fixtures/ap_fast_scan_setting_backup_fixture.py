"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Fixtures for handling system settings entry for FastScanEnabled

"""

# Import builtin libraries
import pytest

import logging

# ly-shared import
from automatedtesting_shared.platform_setting import PlatformSetting
from ly_test_tools.o3de.pipeline_utils import AP_FASTSCAN_KEY as fast_scan_key
from ly_test_tools.o3de.pipeline_utils import AP_FASTSCAN_SUBKEY as fast_scan_subkey

logger = logging.getLogger(__name__)

@pytest.fixture
def ap_fast_scan_setting_backup_fixture(request, workspace) -> PlatformSetting:
    """
    PyTest Fixture for backing up and restoring the system entry for "Fast Scan Enabled"
    :return: A PlatformSetting object targeting the system setting for AP Fast Scan
    """
    if workspace.asset_processor_platform == 'mac':
        pytest.skip("Mac plist file editing not implemented yet")

    if workspace.asset_processor_platform == 'linux':
        pytest.skip("Linux system settings not implemented yet")

    key = fast_scan_key
    subkey = fast_scan_subkey

    fast_scan_setting = PlatformSetting.get_system_setting(workspace, subkey, key)
    original_value = fast_scan_setting.get_value()

    def teardown():
        if original_value is None:
            fast_scan_setting.delete_entry()
        else:
            fast_scan_setting.set_value(original_value)

    request.addfinalizer(teardown)

    return fast_scan_setting
