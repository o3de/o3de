"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest
import os
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest


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
    class test_Multiplayer_AutoComponent_NetworkInput(EditorSingleTest):
        from .tests import Multiplayer_AutoComponent_NetworkInput as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            save_multiplayer_level_cache_folder_artifact(workspace, "autocomponent_networkinput")

    class test_Multiplayer_AutoComponent_RPC(EditorSingleTest):
        from .tests import Multiplayer_AutoComponent_RPC as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            save_multiplayer_level_cache_folder_artifact(workspace, "autocomponent_rpc")

    class test_Multiplayer_BasicConnectivity_Connects(EditorSingleTest):
        from .tests import Multiplayer_BasicConnectivity_Connects as test_module
        
        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            save_multiplayer_level_cache_folder_artifact(workspace, "basicconnectivity_connects")

    class test_Multiplayer_SimpleNetworkLevelEntity(EditorSingleTest):
        from .tests import Multiplayer_SimpleNetworkLevelEntity as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            save_multiplayer_level_cache_folder_artifact(workspace, "simplenetworklevelentity")
