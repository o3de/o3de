"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest
import sys

from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorBatchedTest, EditorSingleTest


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation_BasicBatch(EditorTestSuite):

    class Editor_CommandLine_Works(EditorBatchedTest):
        from .tests import Editor_CommandLine_Works as test_module

    class Editor_ComponentAssetCommands_Works(EditorBatchedTest):
        from .tests import Editor_ComponentAssetCommands_Works as test_module

    class Editor_ComponentCommands_BuildComponentTypeNameList(EditorBatchedTest):
        from .tests import Editor_ComponentCommands_BuildComponentTypeNameList as test_module

    @pytest.mark.skipif(sys.platform.startswith("linux"), reason="https://github.com/o3de/o3de/issues/11032")
    class Editor_ComponentCommands_Works(EditorSingleTest):
        from .tests import Editor_ComponentCommands_Works as test_module

    class Editor_ComponentPropertyCommands_enum(EditorBatchedTest):
        from .tests import Editor_ComponentPropertyCommands_enum as test_module

    class Editor_ComponentPropertyCommands_set_none(EditorBatchedTest):
        from .tests import Editor_ComponentPropertyCommands_set_none as test_module

    class Editor_ComponentPropertyCommands_visibility(EditorBatchedTest):
        from .tests import Editor_ComponentPropertyCommands_visibility as test_module

    class Editor_DisplaySettingsBus_Work(EditorBatchedTest):
        from .tests import Editor_DisplaySettingsBus_Work as test_module

    class Editor_DisplaySettingsCommands_Works(EditorBatchedTest):
        from .tests import Editor_DisplaySettingsCommands_Works as test_module

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation_EntityTests(EditorTestSuite):

    class Editor_ComponentPropertyCommands_Works(EditorBatchedTest):
        from .tests import Editor_ComponentPropertyCommands_Works as test_module

    class Editor_EntityCommands_Works(EditorBatchedTest):
        from .tests import Editor_EntityCommands_Works as test_module

    class Editor_EntityCRUDCommands_Works(EditorBatchedTest):
        from .tests import Editor_EntityCRUDCommands_Works as test_module

    class Editor_EntitySearchCommands_Works(EditorBatchedTest):
        from .tests import Editor_EntitySearchCommands_Works as test_module

    class Editor_EntitySelectionCommands_Works(EditorBatchedTest):
        from .tests import Editor_EntitySelectionCommands_Works as test_module

    class Editor_GameModeCommands_Works(EditorBatchedTest):
        from .tests import Editor_GameModeCommands_Works as test_module

    class Editor_LevelCommands_Works(EditorSingleTest):
        from .tests import Editor_LevelCommands_Works as test_module

    class Editor_LevelPathsCommands_Works(EditorBatchedTest):
        from .tests import Editor_LevelPathsCommands_Works as test_module

    class Editor_MainWindowCommands_Works(EditorBatchedTest):
        from .tests import Editor_MainWindowCommands_Works as test_module

    class Editor_ObjectStringRepresentation_Works(EditorBatchedTest):
        from .tests import Editor_ObjectStringRepresentation_Works as test_module

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation_EditorTools(EditorTestSuite):

    class Editor_PySide_Example_Works(EditorBatchedTest):
        from .tests import Editor_PySide_Example_Works as test_module

    class Editor_TrackViewCommands_Works(EditorBatchedTest):
        from .tests import Editor_TrackViewCommands_Works as test_module

    class Editor_UtilityCommands_Works(EditorBatchedTest):
        from .tests import Editor_UtilityCommands_Works as test_module

    class Editor_UtilityCommands_Works(EditorBatchedTest):
        from .tests import Editor_UtilityCommands_Works as test_module

    class Editor_UtilityCommandsLegacy_Works(EditorBatchedTest):
        from .tests import Editor_UtilityCommandsLegacy_Works as test_module

    class Editor_ViewCommands_Works(EditorBatchedTest):
        from .tests import Editor_ViewCommands_Works as test_module

    class Editor_ViewportTitleDlgCommands_Works(EditorBatchedTest):
        from .tests import Editor_ViewportTitleDlgCommands_Works as test_module

    class Editor_WaitCommands_Works(EditorBatchedTest):
        from .tests import Editor_WaitCommands_Works as test_module
