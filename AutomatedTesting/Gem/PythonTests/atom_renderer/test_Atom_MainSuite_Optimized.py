"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Optimized version of the main suite tests for the Atom renderer.
"""
import pytest

from ly_test_tools.o3de.editor_test import EditorSharedTest, EditorTestSuite


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
class TestAutomation(EditorTestSuite):

    class test_AtomEditorComponents_AddedToEntity(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_AddedToEntity as test_module

    class test_AtomEditorComponents_LightComponent(EditorSharedTest):
        from atom_renderer.atom_hydra_scripts import hydra_AtomEditorComponents_LightComponent as test_module
