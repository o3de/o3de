"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

"""
C697735: Custom layouts can be saved
C697736: Custom/Default layouts can be loaded
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 100


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
class TestLoadLayout(object):
    @pytest.mark.test_case_id("C697736", "C697735")
    def test_EditorLayout_CustomLayoutSavesAndLoads(self, request, editor):
        expected_lines = [
            "load_layout:  test started",
            "Custom layout setup complete",
            "load_layout:  result=SUCCESS",
            "QAction not found for menu item 'custom_layout_test'"
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "editor_layout_save_and_load.py",
            expected_lines,
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )
