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
        
    @pytest.mark.test_case_id("C36530722")
    class Editor_levelLoad_Atom_hermanubis(EditorBatchedTest):
        from Atom.tests import levelLoad_Atom_hermanubis as test_module

    @pytest.mark.test_case_id("C36530724")
    class Editor_levelLoad_Atom_hermanubis_high(EditorBatchedTest):
        from Atom.tests import levelLoad_Atom_hermanubis_high as test_module

    @pytest.mark.test_case_id("C36530725")
    class Editor_levelLoad_Atom_macbeth_shaderballs(EditorBatchedTest):
        from Atom.tests import levelLoad_Atom_macbeth_shaderballs as test_module

    @pytest.mark.test_case_id("C36530726")
    class Editor_levelLoad_Atom_PbrMaterialChart(EditorBatchedTest):
        from Atom.tests import levelLoad_Atom_PbrMaterialChart as test_module

    @pytest.mark.test_case_id("C36530727")
    class Editor_levelLoad_Atom_ShadowTest(EditorBatchedTest):
        from Atom.tests import levelLoad_Atom_ShadowTest as test_module

    @pytest.mark.test_case_id("C36530728")
    class Editor_levelLoad_Atom_Sponza(EditorBatchedTest):
        from Atom.tests import levelLoad_Atom_Sponza as test_module
