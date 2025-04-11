"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import pytest

from ly_test_tools.o3de.editor_test import EditorSingleTest, EditorTestSuite


@pytest.mark.SUITE_main
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomationDPE(EditorTestSuite):

    # Disable Batch Mode and autotest mode and enable the DPE
    global_extra_cmdline_args = ["-ed_enableDPEInspector=true"]

    @pytest.mark.xfail(reason="https://github.com/o3de/o3de/issues/15718, https://github.com/o3de/o3de/issues/15704, "
                              "https://github.com/o3de/o3de/issues/15695, https://github.com/o3de/o3de/issues/15579")
    class test_DPE_AllComponentsAddedRemoved(EditorSingleTest):
        from .EditorScripts import DPE_AllComponentsAddedRemoved as test_module

    class test_DPE_AllComponentPropertyTypesEditable(EditorSingleTest):
        from .EditorScripts import DPE_AllComponentPropertyTypesEditable as test_module
