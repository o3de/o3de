"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest


from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    enable_prefab_system = False

    # this test is intermittently timing out without ever having executed. sandboxing while we investigate cause.
    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DisplayMapperAdded as test_module

    # this test causes editor to crash when using slices. once automation transitions to prefabs it should pass
    @pytest.mark.test_case_id("C36529666")
    class AtomEditorComponentsLevel_DiffuseGlobalIlluminationAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponentsLevel_DiffuseGlobalIlluminationAdded as test_module

    # this test causes editor to crash when using slices. once automation transitions to prefabs it should pass
    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponentsLevel_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponentsLevel_DisplayMapperAdded as test_module
