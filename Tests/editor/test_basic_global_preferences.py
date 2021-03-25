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
C6321491: Basic Function: Global Preferences
https://testrail.agscollab.com/index.php?/cases/view/6321491
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 80


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
class TestBasicGlobalPreferences(object):
    expected_lines = [
        "Global Preferences action triggered",
        "Toolbar Icon size set to Large",
        "Toolbar Icon size set to Default",
        "Toolbar Icon size changed in Global Preferences",
    ]

    @pytest.mark.test_case_id("C6321491")
    def test_basic_global_preferences(self, request, editor):
        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "basic_global_preferences.py",
            self.expected_lines,
            auto_test_mode=False,
            timeout=log_monitor_timeout,
        )
