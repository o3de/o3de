"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest
import os
import sys
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from ly_test_tools.environment import process_utils
from ly_test_tools.launchers import launcher_helper
from ly_test_tools.log.log_monitor import LogMonitor

import ly_test_tools.environment.waiter as waiter

# Saves the level cache folder.
# These artifacts will be saved in the test results so developers can access the level assets
# to debug should the test ever fail.
def save_multiplayer_level_cache_folder_artifact(workspace, multiplayer_level):
    level_cache_folder_path = os.path.join(workspace.paths.platform_cache(), "levels", "multiplayer", multiplayer_level)

    if os.path.exists(level_cache_folder_path):
        workspace.artifact_manager.save_artifact(level_cache_folder_path)
    else:
        pytest.fail(f"Failed to find level asset cache for '{multiplayer_level}', located here: '{level_cache_folder_path}'! Make sure AssetProcessor successfully built the level.")


@pytest.mark.SUITE_main
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    class test_Multiplayer_BasicConnectivity_Connects(EditorSingleTest):
        from .tests import Multiplayer_BasicConnectivity_Connects as test_module
        
        timeout = 60.0 * 15.0 # increase timeout to ~15 minutes to accommodate for slow server startup

        def __init__(self):
            super(test_Multiplayer_BasicConnectivity_Connects, self).__init__()

        @classmethod
        def setup(cls, instance, request, workspace):
            save_multiplayer_level_cache_folder_artifact(workspace, "basicconnectivity_connects")

    class test_Multiplayer_BasicConnectivity_Connects_ClientServer(EditorSingleTest):
        from .tests import Multiplayer_BasicConnectivity_Connects_ClientServer as test_module

        timeout = 60.0 * 15.0 # increase timeout to ~15 minutes to accommodate for slow server startup

        def __init__(self):
            super(test_Multiplayer_BasicConnectivity_Connects_ClientServer, self).__init__()
