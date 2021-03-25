"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This demos_testlib file is used for a collection of reusable functionality that QA will use in their scripts specific
to the setup of demo level tests.
"""
import sys

import shared.network_utils as network_utils
from shared.screenshot_utils import move_screenshots, take_screenshot_with_retries

from test_tools.shared.launcher_testlib import *

import test_tools.shared.waiter


def start_launcher(launcher):
    """
    For PC: Used to start launcher and give time to load.
    """
    launcher.launch()
    launcher.run(test_tools.launchers.phase.TimePhase(120, 120))


def load_level(launcher, remote_console, level):
    """
    Uses the remote console to use the map command to load a level and checks the console output for a successful load.
    """
    command = 'map {}'.format(level)
    load = remote_console.expect_log_line('LEVEL_LOAD_COMPLETE', 300)
    retry_console_command(remote_console, command, "Executing console command '{}'".format(command))
    assert load(), "{} level failed to load.".format(level)

    # Allow one minute to let level fully render and to test for stability
    launcher.run(test_tools.launchers.phase.TimePhase(60, 60))


def start_remote_console(launcher, remote_console, on_devkit=False):
    """
    Starts the remote console. Used in QA scripts that require the use of remote console.
    """
    if on_devkit:
        test_tools.shared.waiter.wait_for(lambda: network_utils.check_for_remote_listening_port(4600, launcher.ip),
                                          timeout=600, exc=AssertionError('Port 4600 not listening.'))
    else:
        test_tools.shared.waiter.wait_for(lambda: network_utils.check_for_listening_port(4600), timeout=300,
                                          exc=AssertionError('Port 4600 not listening.'))

    remote_console.start()

    # Allows remote console time to connect to launcher.
    launcher.run(test_tools.launchers.phase.TimePhase(60, 60))


def remote_console_take_screenshot(launcher, remote_console, level):
    """
    Uses the remote console to run the r_GetScreenshot command to take a screenshot of the current launcher and move
    the screenshot to the test results location.
    """
    screenshot_path = os.path.join(launcher.workspace.release.paths.platform_cache(), "user", "screenshots")
    take_screenshot_with_retries(remote_console, launcher, level)
    if os.path.exists(screenshot_path):
        move_screenshots(screenshot_path, '.jpg', launcher.workspace.artifact_manager.get_save_artifact_path())
