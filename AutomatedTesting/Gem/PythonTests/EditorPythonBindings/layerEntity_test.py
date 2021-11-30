"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API for Entity CRUD
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from .hydra_utils import launch_test_case


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestLayerEntityAutomation(object):

    def test_LayerEntity(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "[PASS] layer has been created",
            "[PASS] successfully changed status: 0",
            "[PASS] successfully changed status: 1",
            "[PASS] successfully changed status: 2",
            "[PASS] Get name request succeeded",
            "[PASS] Layer name changed",
            "[PASS] Layer color changed",
            "[PASS] layer child entity has been created",
            "[PASS] Query parent returned layer ID",
            "[PASS] EditorEntityInfoRequestBus GetChildren return list of decendants",
            "[PASS] comment component has been added",
            "[PASS] layer has been locked",
            "[PASS] layer children are hidden",
            "[PASS] Layer entity not found after delete request"
            ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'layerEntity_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
