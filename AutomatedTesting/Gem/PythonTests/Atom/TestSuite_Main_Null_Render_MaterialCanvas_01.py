"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
import pytest

from ly_test_tools.o3de.material_canvas_test import MaterialCanvasBatchedTest, MaterialCanvasTestSuite

logger = logging.getLogger(__name__)


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("launcher_platform", ['windows_material_canvas'])
class TestMaterialCanvas(MaterialCanvasTestSuite):

    class MaterialEditor_Atom_LaunchMaterialCanvas_1(MaterialCanvasBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module

    class MaterialEditor_Atom_LaunchMaterialCanvas_2(MaterialCanvasBatchedTest):

        from Atom.tests import MaterialCanvas_Atom_LaunchMaterialCanvas as test_module
