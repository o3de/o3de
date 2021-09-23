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
    use_null_renderer = False  # Note: Default is True

    class AtomGPU_BasicLevelSetup(EditorSharedTest):
        from Atom.tests import hydra_AtomGPU_BasicLevelSetup as test_module
