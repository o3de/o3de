"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.


Static tool scripts
Launch Static tool and Verify the help message
"""

import os
import pytest
import subprocess
import sys


def verify_help_message(static_tool):
    help_message = ["--help", "show this help message and exit"]
    output = subprocess.run([sys.executable, static_tool, "-h"], capture_output=True)
    assert (
        len(output.stderr) == 0 and output.returncode == 0
    ), f"Error occurred while launching {static_tool}: {output.stderr}"
    #  verify help message
    for message in help_message:
        assert message in str(output.stdout), f"Help Message: {message} is not present"


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.SUITE_smoke
class TestStaticToolsGenPakShadersWorks(object):
    def test_StaticTools_GenPakShaders_Works(self, editor):
        static_tools = [
            os.path.join(editor.workspace.paths.engine_root(), "scripts", "bundler", "gen_shaders.py"),
            os.path.join(editor.workspace.paths.engine_root(), "scripts", "bundler", "get_shader_list.py"),
            os.path.join(editor.workspace.paths.engine_root(), "scripts", "bundler", "pak_shaders.py"),
        ]

        for tool in static_tools:
            verify_help_message(tool)
