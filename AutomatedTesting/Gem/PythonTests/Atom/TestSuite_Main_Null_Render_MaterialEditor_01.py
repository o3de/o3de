"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import pytest

from ly_test_tools.o3de.atom_tools_test import AtomToolsBatchedTest, AtomToolsTestSuite, AtomToolsSingleTest

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_atom_tools'])
class TestMaterialEditor(AtomToolsTestSuite):

    log_name = "material_editor_test.log"
    atom_tools_executable_name = "MaterialEditor"

    class MaterialEditor_Atom_LaunchMaterialEditor_1(AtomToolsBatchedTest):

        from Atom.tests import MaterialEditor_Atom_LaunchMaterialEditor as test_module

    class MaterialEditor_Atom_LaunchMaterialEditor_2(AtomToolsBatchedTest):

        from Atom.tests import MaterialEditor_Atom_LaunchMaterialEditor as test_module

    class MaterialEditor_Atom_BasicTests(AtomToolsBatchedTest):

        from Atom.tests import MaterialEditor_Atom_BasicTests as test_module

    @pytest.mark.xfail
    class MaterialEditor_Atom_ExpectsTestFailure(AtomToolsBatchedTest):

        from Atom.tests import MaterialEditor_Atom_ExpectsTestFailure as test_module

    @pytest.mark.xfail
    class MaterialEditor_Atom_ExpectsTestTimeout(AtomToolsSingleTest):
        timeout = 10

        from Atom.tests import MaterialEditor_Atom_ExpectsTestTimeout as test_module

    @pytest.mark.xfail
    class MaterialEditor_Atom_expectsCrashFailure(AtomToolsSingleTest):

        from Atom.tests import MaterialEditor_Atom_ExpectsCrashFailure as test_module
