"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest

import ly_test_tools
from ly_test_tools.o3de.editor_test import EditorBatchedTest, EditorTestSuite


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    @pytest.mark.test_case_id("C32078125")
    class AtomEditorComponents_PhysicalSkyAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_PhysicalSkyAdded as test_module

    @pytest.mark.test_case_id("C36525664")
    class AtomEditorComponents_PostFXGradientWeightModifierAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_PostFXGradientWeightModifierAdded as test_module

    @pytest.mark.test_case_id("C32078127")
    class AtomEditorComponents_PostFXLayerAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_PostFXLayerAdded as test_module

    @pytest.mark.test_case_id("C32078131")
    class AtomEditorComponents_PostFXRadiusWeightModifierAdded(EditorBatchedTest):
        from Atom.tests import (
            hydra_AtomEditorComponents_PostFXRadiusWeightModifierAdded as test_module)

    @pytest.mark.test_case_id("C36525665")
    class AtomEditorComponents_PostFXShapeWeightModifierAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_PostFxShapeWeightModifierAdded as test_module

    @pytest.mark.skipif(ly_test_tools.LINUX, reason="https://github.com/o3de/o3de/issues/14008")
    @pytest.mark.test_case_id("C32078128")
    class AtomEditorComponents_ReflectionProbeAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_ReflectionProbeAdded as test_module

    class AtomEditorComponents_SkyAtmosphereAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_SkyAtmosphereAdded as test_module

    @pytest.mark.test_case_id("C36525666")
    class AtomEditorComponents_SSAOAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_SSAOAdded as test_module

    class AtomEditorComponents_StarsAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponents_StarsAdded as test_module

    @pytest.mark.test_case_id("C36529666")
    class AtomEditorComponentsLevel_DiffuseGlobalIlluminationAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponentsLevel_DiffuseGlobalIlluminationAdded as test_module

    @pytest.mark.test_case_id("C36525660")
    class AtomEditorComponentsLevel_DisplayMapperAdded(EditorBatchedTest):
        from Atom.tests import hydra_AtomEditorComponentsLevel_DisplayMapperAdded as test_module
