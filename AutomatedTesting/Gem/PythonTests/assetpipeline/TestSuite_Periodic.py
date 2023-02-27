"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

import pytest

import ly_test_tools
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest


@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    @pytest.mark.skipif(ly_test_tools.LINUX, reason="https://github.com/o3de/o3de/issues/14514")
    class SubID_NoChange_MeshChanged(EditorSingleTest):
        from .fbx_tests import hydra_SubID_NoChange_MeshChanged as test_module

    @pytest.mark.skipif(ly_test_tools.LINUX, reason="https://github.com/o3de/o3de/issues/14514")
    class SubID_WarningReported_AssetRemoved(EditorSingleTest):
        from .fbx_tests import hydra_SubID_WarningReported_AssetRemoved as test_module
