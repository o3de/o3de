"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

C6321576: Basic Function: Customize Keyboard
https://testrail.agscollab.com/index.php?/cases/view/6321576
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
class TestCustomizeKeyboard(object):
    @pytest.mark.test_case_id("C6321576")
    def test_customize_keyboard(self, request, editor):
        expected_lines = [
            "Customize keyboard window opened successfully",
            "New shortcut works : Asset Editor opened",
            "Default shortcuts restored : Asset Editor stays closed"
        ]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "customize_keyboard.py",
            expected_lines,
            auto_test_mode=False # Customize Keyboard is a modal dialog so test mode is disabled
        )
