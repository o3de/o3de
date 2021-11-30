"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

#
# This is a pytest module to test the in-Editor Python API for Entity Search
#
import pytest
pytest.importorskip('ly_test_tools')

import sys
import os
sys.path.append(os.path.dirname(__file__))
from hydra_utils import launch_test_case


@pytest.mark.SUITE_sandbox
@pytest.mark.parametrize('launcher_platform', ['windows_editor'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['Simple'])
class TestEntitySearchAutomation(object):

    def test_EntitySearch(self, request, editor, level, launcher_platform):

        unexpected_lines=[]
        expected_lines = [
            "SUCCESS: Root Entities",
            "SUCCESS: No filters - return all entities",
            "SUCCESS: Filter by name - single entity",
            "SUCCESS: Filter by name - multiple entities",
            "SUCCESS: Filter by name - multiple names",
            "SUCCESS: Filter by name - wildcard 01",
            "SUCCESS: Filter by name - wildcard 02",
            "SUCCESS: Filter by name - wildcard 03",
            "SUCCESS: Filter by name - wildcard 04",
            "SUCCESS: Filter by name - wildcard 05",
            "SUCCESS: Filter by name - case sensitive 01",
            "SUCCESS: Filter by name - case sensitive 02",
            "SUCCESS: Filter by name - case sensitive 03",
            "SUCCESS: Filter by name - case sensitive 04",
            "SUCCESS: Filter by path - base 01",
            "SUCCESS: Filter by path - base 02",
            "SUCCESS: Filter by path - wildcard 01",
            "SUCCESS: Filter by path - wildcard 02",
            "SUCCESS: Filter by path - wildcard 03",
            "SUCCESS: Filter by path - wildcard 04",
            "SUCCESS: Filter by path - case sensitive 01",
            "SUCCESS: Filter by path - case sensitive 02",
            "SUCCESS: Filter by path - case sensitive 03",
            "SUCCESS: Filter by path - case sensitive 04",
            "SUCCESS: Filter by component - base 01",
            "SUCCESS: Filter by component - base 02",
            "SUCCESS: Filter by component - multiple 01",
            "SUCCESS: Filter by component - multiple 02",
            "SUCCESS: Filter with roots - base 01",
            "SUCCESS: Filter with roots - base 02",
            "SUCCESS: Filter with roots - base 03",
            "SUCCESS: Filter with roots - base 04",
            "SUCCESS: Filter with roots - base 05",
            "SUCCESS: Filter with roots - NameIsRootBased 01",
            "SUCCESS: Filter with roots - NameIsRootBased 02",
            "SUCCESS: Filter with roots - NameIsRootBased 03",
            "SUCCESS: Filter with roots - NameIsRootBased 04",
            "SUCCESS: Filter with roots - NameIsRootBased 05",
            "SUCCESS: Search with Multiple Filters",
            "SUCCESS: Filter by AABB - base 01",
            "SUCCESS: Filter by AABB - base 02",
            "SUCCESS: Filter by Component Properties - base 01",
            "SUCCESS: Filter by Component Properties - base 02",
            "SUCCESS: Filter by Component Properties - base 03"
        ]
        
        test_case_file = os.path.join(os.path.dirname(__file__), 'EntitySearchCommands_test_case.py')
        launch_test_case(editor, test_case_file, expected_lines, unexpected_lines)
