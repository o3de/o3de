"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest

from ly_test_tools.o3de.atom_tools_test import AtomToolsBatchedTest, AtomToolsTestSuite


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_atom_tools'])
class TestMaterialEditorAtomPeriodic(AtomToolsTestSuite):

    log_name = "material_editor_test.log"
    atom_tools_executable_name = "MaterialEditor"

    class MaterialEditor_Atom_PeriodicTests(AtomToolsBatchedTest):
        from Atom.tests import MaterialEditor_Atom_PeriodicTests as test_module
