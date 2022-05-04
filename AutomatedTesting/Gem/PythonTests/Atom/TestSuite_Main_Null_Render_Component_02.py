"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import os
import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite

logger = logging.getLogger(__name__)
TEST_DIRECTORY = os.path.join(os.path.dirname(__file__), "tests")


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    @pytest.mark.test_case_id("C32078115")
    class AtomEditorComponents_GlobalSkylightIBLAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_GlobalSkylightIBLAdded as test_module
        
    @pytest.mark.test_case_id("C32078122")
    class AtomEditorComponents_GridAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_GridAdded as test_module

    @pytest.mark.test_case_id("C36525671")
    class AtomEditorComponents_HDRColorGradingAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_HDRColorGradingAdded as test_module

    @pytest.mark.test_case_id("C32078116")
    class AtomEditorComponents_HDRiSkyboxAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_HDRiSkyboxAdded as test_module

    @pytest.mark.test_case_id("C32078117")
    class AtomEditorComponents_LightAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_LightAdded as test_module

    @pytest.mark.test_case_id("C36525662")
    class AtomEditorComponents_LookModificationAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_LookModificationAdded as test_module

    @pytest.mark.test_case_id("C32078123")
    class AtomEditorComponents_MaterialAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_MaterialAdded as test_module

    @pytest.mark.test_case_id("C32078124")
    class AtomEditorComponents_MeshAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_MeshAdded as test_module

    @pytest.mark.test_case_id("C36525663")
    class AtomEditorComponents_OcclusionCullingPlaneAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_OcclusionCullingPlaneAdded as test_module
