"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest


@pytest.mark.parametrize("launcher_platform", ['windows_editor']) # This test works on Windows version of O3DE Editor
@pytest.mark.parametrize("project", ["AutomatedTesting"]) # Use AutomatedTesting project
class TestAutomation(EditorTestSuite):
    class ShaderAssetBuilder_RGAData(EditorSingleTest):
        from Atom.tests import RGAtest as test_module