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

@pytest.mark.SUITE_main
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):
    class test_Multiplayer_AutoComponent_NetworkInput(EditorSingleTest):
        from .tests import Multiplayer_AutoComponent_NetworkInput as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            level_cache_folder_path = os.path.join(workspace.paths.platform_cache(), "levels", "multiplayer", "autocomponent_networkinput")
            if os.path.exists(level_cache_folder_path):
                workspace.artifact_manager.save_artifact(level_cache_folder_path)

    class test_Multiplayer_AutoComponent_RPC(EditorSingleTest):
        from .tests import Multiplayer_AutoComponent_RPC as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            level_cache_folder_path = os.path.join(workspace.paths.platform_cache(), "levels", "multiplayer", "autocomponent_rpc")
            if os.path.exists(level_cache_folder_path):
                workspace.artifact_manager.save_artifact(level_cache_folder_path)

    class test_Multiplayer_BasicConnectivity_Connects(EditorSingleTest):
        from .tests import Multiplayer_BasicConnectivity_Connects as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            level_cache_folder_path = os.path.join(workspace.paths.platform_cache(), "levels", "multiplayer", "basicconnectivity_connects")
            if os.path.exists(level_cache_folder_path):
                workspace.artifact_manager.save_artifact(level_cache_folder_path)

    class test_Multiplayer_SimpleNetworkLevelEntity(EditorSingleTest):
        from .tests import Multiplayer_SimpleNetworkLevelEntity as test_module

        @classmethod
        def setup(cls, instance, request, workspace, editor, editor_test_results, launcher_platform):
            level_cache_folder_path = os.path.join(workspace.paths.platform_cache(), "levels", "multiplayer", "simplenetworklevelentity")
            if os.path.exists(level_cache_folder_path):
                workspace.artifact_manager.save_artifact(level_cache_folder_path)

