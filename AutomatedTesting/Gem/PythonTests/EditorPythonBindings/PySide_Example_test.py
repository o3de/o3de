''
"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

#
# This test shows how to use PySide2 inside an Editor Python Bindings test.
#

import pytest
import sys
import os
from .hydra_utils import launch_test_case


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestPySideExample(object):

    def test_PySideExample(self, request, editor, level, launcher_platform):

        unexpected_lines = []
        expected_lines = [
            'New entity with no parent created',
            'Environment Probe component added to entity',
            'ComboBox Values retrieved:'
            ]

        test_case_file = os.path.join(os.path.dirname(__file__), 'PySide_Example_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
