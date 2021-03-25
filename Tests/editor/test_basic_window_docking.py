"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C18668804: Basic Window Docking System Tests
https://testrail.agscollab.com/index.php?/cases/view/18668804
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 60


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
class TestBasicWindowDocking(object):
    @pytest.mark.test_case_id("C18668804")
    def test_basic_window_docking(self, request, editor):
        expected_lines = [
            "Entity Outliner is in a floating window",
            "Entity Outliner docked in top area",
            "Entity Outliner docked in right area",
            "Entity Outliner docked in bottom area",
            "Entity Outliner docked in left area"
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_window_docking.py",
            expected_lines,
            timeout=log_monitor_timeout,
        )
