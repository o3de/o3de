"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest

from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    @pytest.mark.test_case_id("C36525657")
    class AtomEditorComponents_BloomAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_BloomAdded as test_module

    class AtomEditorComponents_ChromaticAberrationAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_ChromaticAberrationAdded as test_module

    @pytest.mark.test_case_id("C36553393")
    class AtomEditorComponents_CubeMapCaptureAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_CubeMapCaptureAdded as test_module

    @pytest.mark.test_case_id("C32078118")
    class AtomEditorComponents_DecalAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_DecalAdded as test_module

    @pytest.mark.test_case_id("C36525658")
    class AtomEditorComponents_DeferredFogAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_DeferredFogAdded as test_module

    @pytest.mark.test_case_id("C32078119")
    class AtomEditorComponents_DepthOfFieldAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_DepthOfFieldAdded as test_module

    @pytest.mark.test_case_id("C36525659")
    class AtomEditorComponents_DiffuseProbeGridAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_DiffuseProbeGridAdded as test_module

    @pytest.mark.test_case_id("C32078120")
    class AtomEditorComponents_DirectionalLightAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_DirectionalLightAdded as test_module

    @pytest.mark.test_case_id("C36525661")
    class AtomEditorComponents_EntityReferenceAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_EntityReferenceAdded as test_module

    @pytest.mark.test_case_id("C32078121")
    class AtomEditorComponents_ExposureControlAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_ExposureControlAdded as test_module
