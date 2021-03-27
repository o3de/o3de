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
C2627906: A simple Vegetation Layer Blender area can be created.
The specified assets plant in the specified blend area and are visible in the Viewport in
Edit Mode, Game Mode.
"""

import os
import pytest

pytest.importorskip("ly_test_tools")

import time as time

import ly_test_tools.launchers.launcher_helper as launcher_helper
import ly_remote_console.remote_console_commands as remote_console_commands
from ly_remote_console.remote_console_commands import send_command_and_expect_response as send_command_and_expect_response
import ly_test_tools.environment.waiter as waiter
import ly_test_tools.environment.file_system as file_system
import automatedtesting_shared.hydra_test_utils as hydra
import automatedtesting_shared.screenshot_utils as screenshot_utils

from automatedtesting_shared.network_utils import check_for_listening_port


test_directory = os.path.join(os.path.dirname(__file__), "EditorScripts")
remote_console_port = 4600
listener_timeout = 120


@pytest.mark.parametrize("project", ["AutomatedTesting"])
@pytest.mark.parametrize("level", ["tmp_level"])
@pytest.mark.usefixtures("automatic_process_killer")
class TestLayerBlender(object):

    @pytest.fixture
    def remote_console_instance(self, request):
        console = remote_console_commands.RemoteConsole()

        def teardown():
            if console.connected:
                console.stop()

        request.addfinalizer(teardown)

        return console

    @pytest.mark.test_case_id("C2627906")
    @pytest.mark.BAT
    @pytest.mark.SUITE_main
    @pytest.mark.parametrize("launcher_platform", ['windows_editor'])
    def test_LayerBlender_E2E_Editor(self, workspace, request, editor, project, level, launcher_platform):
        # Make sure temp level doesn't already exist
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)

        expected_lines = [
            "'Purple Spawner' created",
            "'Pink Spawner' created",
            "'Surface Entity' created",
            "Entity has a Vegetation Layer Spawner component",
            "Entity has a Vegetation Asset List component",
            "Entity has a Box Shape component",
            "Purple Spawner Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Pink Spawner Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Purple Spawner Configuration|Embedded Assets|[0]: SUCCESS",
            "Pink Spawner Configuration|Embedded Assets|[0]: SUCCESS",
            "'Blender' created",
            "Entity has a Vegetation Layer Blender component",
            "Entity has a Box Shape component",
            "Blender Configuration|Vegetation Areas: SUCCESS",
            "Blender Box Shape|Box Configuration|Dimensions: SUCCESS",
            "Camera entity created",
            "LayerBlender_E2E_Editor:  result=SUCCESS"
        ]

        hydra.launch_and_validate_results(
            request,
            test_directory,
            editor,
            "LayerBlender_E2E_Editor.py",
            expected_lines,
            cfg_args=[level]
        )

    @pytest.mark.test_case_id("C2627906")
    @pytest.mark.BAT
    @pytest.mark.SUITE_main
    @pytest.mark.xfail
    @pytest.mark.parametrize("launcher_platform", ['windows'])
    def test_LayerBlender_E2E_Launcher(self, workspace, project, launcher, level, remote_console_instance,
                                       launcher_platform):

        launcher.args.extend(["-NullRenderer"])
        launcher.start()
        assert launcher.is_alive(), "Launcher failed to start"

        # Wait for test script to quit the launcher. If wait_for returns exc, test was not successful
        waiter.wait_for(lambda: not launcher.is_alive(), timeout=300)

        # Verify launcher quit successfully and did not crash
        ret_code = launcher.get_returncode()
        assert ret_code == 0, "Test failed. See Game.log for details"

        # Cleanup our temp level
        file_system.delete([os.path.join(workspace.paths.dev(), project, "Levels", level)], True, True)
