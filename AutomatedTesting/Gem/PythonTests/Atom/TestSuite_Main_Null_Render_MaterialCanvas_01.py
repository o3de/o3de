"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import pytest

from ly_test_tools import WINDOWS,LINUX
from ly_test_tools.o3de.atom_tools_test import AtomToolsBatchedTest, AtomToolsTestSuite

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_atom_tools'])
class TestMaterialCanvas(AtomToolsTestSuite):

    log_name = "material_canvas_test.log"
    atom_tools_executable_name = "MaterialCanvas"

    @pytest.mark.skipif(WINDOWS, reason="https://github.com/o3de/o3de/issues/17010")
    class MaterialCanvas_Atom_LaunchMaterialCanvas_1(AtomToolsBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module

    @pytest.mark.skipif(WINDOWS, reason="https://github.com/o3de/o3de/issues/17010")
    class MaterialCanvas_Atom_LaunchMaterialCanvas_2(AtomToolsBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module

    @pytest.mark.skipif(WINDOWS, reason="https://github.com/o3de/o3de/issues/17010")
    @pytest.mark.skipif(LINUX, reason="https://github.com/o3de/o3de/issues/14565")
    class MaterialCanvas_Atom_BasicTests(AtomToolsBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_BasicTests as test_module
