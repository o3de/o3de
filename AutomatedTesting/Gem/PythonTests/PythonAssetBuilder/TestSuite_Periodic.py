"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import shutil, os, time
import pytest
import ly_test_tools.o3de.editor_test_utils as editor_test_utils
from ly_test_tools.o3de.editor_test import EditorTestSuite, EditorSingleTest

@pytest.mark.SUITE_periodic
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestAutomation(EditorTestSuite):

    class MaterialProperty_ProcPrefabWorks(EditorSingleTest):
        from .tests import MaterialProperty_ProcPrefabWorks as test_module
