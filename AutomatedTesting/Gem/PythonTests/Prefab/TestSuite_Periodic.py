"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorSingleTest, EditorTestSuite


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationNoOverrides(EditorTestSuite):

    # These tests will execute with prefab outliner overrides disabled
    EditorTestSuite.global_extra_cmdline_args.append("--regset=/O3DE/Autoexec/ConsoleCommands/ed_enableOutlinerOverrideManagement=false")

    @pytest.mark.skip(reason="Single test case to avoid suite failure. Can be removed when other tests are added.")
    class test_DummyTest(EditorSingleTest):
        pass
