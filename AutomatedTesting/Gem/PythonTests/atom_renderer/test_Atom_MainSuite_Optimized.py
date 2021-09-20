"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest

from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite


@pytest.mark.xfail(reason="Optimized tests are experimental, we will enable xfail and monitor them temporarily.")
@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    class AtomEditorComponents_DecalAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_DecalAdded as test_module

    class AtomEditorComponents_DepthOfFieldAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_DepthOfFieldAdded as test_module

    class AtomEditorComponents_DirectionalLightAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_DirectionalLightAdded as test_module

    class AtomEditorComponents_ExposureControlAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_ExposureControlAdded as test_module

    class AtomEditorComponents_GlobalSkylightIBLAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_GlobalSkylightIBLAdded as test_module

    class AtomEditorComponents_PhysicalSkyAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_PhysicalSkyAdded as test_module

    class AtomEditorComponents_PostFXRadiusWeightModifierAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import (
            hydra_AtomEditorComponents_PostFXRadiusWeightModifierAdded as test_module)

    class AtomEditorComponents_LightAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_LightAdded as test_module

    class AtomEditorComponents_DisplayMapperAdded(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_DisplayMapperAdded as test_module
