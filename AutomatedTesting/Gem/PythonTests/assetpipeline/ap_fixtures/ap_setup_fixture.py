"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

A fixture for Setting Up Asset Processor Batch workspace for tests
"""

# Import builtin libraries
import os
import time
import pytest
from typing import Dict
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

@pytest.fixture
def ap_setup_fixture(request, workspace) -> Dict:
    """
    Sets up useful file locations based on the current workspace.
    :return: useful locations as a dictionary
    """
    test_asset_dir_name = "ap_test_assets"
    resources = {
        # Path to directory where AssetProcessor[Batch].exe lives
        "bin_dir": workspace.paths.build_directory(),
        # Name of temporary folder for assets (scope = function)
        "test_asset_dir_name": test_asset_dir_name,
        # Temporary folder within project cache for use in test
        "cache_test_assets": os.path.join(workspace.paths.asset_cache(ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform]), test_asset_dir_name),
        # Joblog folder location
        "test_joblogs_dir": os.path.join(workspace.paths.build_directory(), "logs", "JobLogs", test_asset_dir_name),
        # Temporary folder in project for use in test
        "project_test_assets_dir": os.path.join(workspace.paths.project(), test_asset_dir_name),
        # Test-level asset folder. Directory contains a subfolder for each test (i.e. C1234567)
        "tests_dir": "",
        # Path to AP_Batch.log
        "ap_batch_log_file": workspace.paths.ap_batch_log(),
        # Path to AP_GUI.log
        "ap_logFile": workspace.paths.ap_gui_log(),
        # Path to the engine root directory
        "engine_dir": workspace.paths.engine_root(),
        # Path to the current project directory
        "project_dir": workspace.paths.project(),
        # Path to AP Batch file
        "ap_batch_file": workspace.paths.asset_processor_batch(),
        # Path to shared TestAssets folder
        "shared_TestAssets": os.path.join(workspace.paths.project(), "TestAssets"),
        # Path to the asset cache folder
        "asset_cache_dir": os.path.join(workspace.paths.asset_cache(ASSET_PROCESSOR_PLATFORM_MAP[workspace.asset_processor_platform])),
    }

    # Changing modtime of UserSettings.xml so it is always processed by AssetProcessorBatch
    # to prevent unexpected processing
    user_settings_file = os.path.join(workspace.paths.project(), "user", "UserSettings.xml")
    if os.path.exists(user_settings_file):
        timestamp = time.time()
        os.utime(user_settings_file, (timestamp, timestamp))

    return resources
