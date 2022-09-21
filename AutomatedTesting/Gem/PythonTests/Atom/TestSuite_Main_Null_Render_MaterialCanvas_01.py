"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import pytest

from ly_test_tools.o3de.atom_tools_test import AtomToolsBatchedTest, AtomToolsTestSuite
from ly_test_tools.launchers import launcher_helper

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_material_canvas'])
class TestMaterialCanvas(AtomToolsTestSuite):

    log_name = "material_canvas_test.log"
    executable_function = launcher_helper.create_material_canvas

    class MaterialCanvas_Atom_LaunchMaterialCanvas_1(AtomToolsBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module

    class MaterialCanvas_Atom_LaunchMaterialCanvas_2(AtomToolsBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module
