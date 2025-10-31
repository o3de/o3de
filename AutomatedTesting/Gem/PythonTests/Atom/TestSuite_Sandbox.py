"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os

import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorSharedTest, EditorTestSuite

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    # this test is intermittently timing out without ever having executed. sandboxing while we investigate cause.
    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from Atom.tests import hydra_AtomEditorComponents_DisplayMapperAdded as test_module

    # The "Sponza" level is failing with a hard lock 4-12% of the time, needs root causing and fixing.
    @pytest.mark.test_case_id("C36529679")
    class AtomLevelLoadTest_Editor_Sandbox(EditorSharedTest):
        from Atom.tests import hydra_Atom_LevelLoadTest_Sandbox as test_module

    # GHI: https://github.com/o3de/o3de/issues/13819
    @pytest.mark.test_case_id("C36553404")
    class AtomEditorComponents_HairAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_HairAdded as test_module

    # GHI: https://github.com/o3de/o3de/issues/13818
    @pytest.mark.test_case_id("C32078124")
    class AtomEditorComponents_MeshAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_MeshAdded as test_module

    # GHI: https://github.com/o3de/o3de/issues/14580
    @pytest.mark.test_case_id("C32078115")
    class AtomEditorComponents_GlobalSkylightIBLAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_GlobalSkylightIBLAdded as test_module
