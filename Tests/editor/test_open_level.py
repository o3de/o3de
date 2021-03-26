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
C6351276: Open a level
https://testrail.agscollab.com/index.php?/cases/view/6351276
"""

import os
import pytest

# Bail on the test if ly_test_tools doesn't exist.
pytest.importorskip("ly_test_tools")

from Tests.ly_shared import hydra_lytt_test_utils as hydra_utils
import ly_test_tools.environment.file_system as file_system

test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
log_monitor_timeout = 100


@pytest.mark.parametrize("platform", ["win_x64_vs2017"])
@pytest.mark.parametrize("configuration", ["profile"])
@pytest.mark.parametrize("project", ["SamplesProject"])
@pytest.mark.parametrize("spec", ["all"])
@pytest.mark.parametrize("level", ["Samples/Audio_Sample"])
class TestOpenLevel(object):
    @pytest.mark.test_case_id("C6351276")
    def test_open_level(self, request, editor, level):
        expected_lines = [
            "Open Level Action triggered",
            "Open a Level dialog opened",
            "An existing level opened : SUCCESS",
        ]
        unexpected_lines = ["An existing level opened : FAILED"]

        hydra_utils.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "open_level.py",
            expected_lines,
            unexpected_lines=unexpected_lines,
            cfg_args=[level],
            auto_test_mode=False,
            run_python="--runpython",
            timeout=log_monitor_timeout,
        )