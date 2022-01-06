"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import sys

import azlmbr.legacy.general as general

sys.path.append(os.path.join(azlmbr.paths.projectroot, "Gem", "PythonTests"))

import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.editor_test_helper import EditorTestHelper
from Atom.atom_utils.benchmark_utils import BenchmarkHelper

SCREEN_WIDTH = 1280
SCREEN_HEIGHT = 720
DEGREE_RADIAN_FACTOR = 0.0174533

helper = EditorTestHelper(log_prefix="Test_Atom_BasicLevelSetup")


def run():
    """
    1. View -> Layouts -> Restore Default Layout, sets the viewport to ratio 16:9 @ 1280 x 720
    2. Runs console command r_DisplayInfo = 0
    3. Opens AtomFeatureIntegrationBenchmark level
    4. Initializes benchmark helper with benchmark name to capture benchmark metadata.
    5. Idles for 100 frames, then collects pass timings for 100 frames.
    :return: None
    """
    def initial_viewport_setup(screen_width, screen_height):
        general.set_viewport_size(screen_width, screen_height)
        general.update_viewport()
        helper.wait_for_condition(
            function=lambda: helper.isclose(a=general.get_viewport_size().x, b=SCREEN_WIDTH, rel_tol=0.1)
            and helper.isclose(a=general.get_viewport_size().y, b=SCREEN_HEIGHT, rel_tol=0.1),
            timeout_in_seconds=4.0
        )
        result = helper.isclose(a=general.get_viewport_size().x, b=SCREEN_WIDTH, rel_tol=0.1) and helper.isclose(
            a=general.get_viewport_size().y, b=SCREEN_HEIGHT, rel_tol=0.1)
        general.log(general.get_viewport_size().x)
        general.log(general.get_viewport_size().y)
        general.log(general.get_viewport_size().z)
        general.log(f"Viewport is set to the expected size: {result}")
        general.run_console("r_DisplayInfo = 0")

    def after_level_load():
        """Function to call after creating/opening a level to ensure it loads."""
        # Give everything a second to initialize.
        general.idle_enable(True)
        general.idle_wait(1.0)
        general.update_viewport()
        general.idle_wait(0.5)  # half a second is more than enough for updating the viewport.

        # Close out problematic windows, FPS meters, and anti-aliasing.
        if general.is_helpers_shown():  # Turn off the helper gizmos if visible
            general.toggle_helpers()
            general.idle_wait(1.0)
        if general.is_pane_visible("Error Report"):  # Close Error Report windows that block focus.
            general.close_pane("Error Report")
        if general.is_pane_visible("Error Log"):  # Close Error Log windows that block focus.
            general.close_pane("Error Log")
        general.idle_wait(1.0)
        general.run_console("r_displayInfo=0")
        general.idle_wait(1.0)

        return True

    # Wait for Editor idle loop before executing Python hydra scripts.
    general.idle_enable(True)

    general.open_level_no_prompt("AtomFeatureIntegrationBenchmark")

    # Basic setup after opening level.
    after_level_load()
    initial_viewport_setup(SCREEN_WIDTH, SCREEN_HEIGHT)

    general.enter_game_mode()
    general.idle_wait(1.0)
    helper.wait_for_condition(function=lambda: general.is_in_game_mode(), timeout_in_seconds=2.0)
    benchmarker = BenchmarkHelper("AtomFeatureIntegrationBenchmark")
    benchmarker.capture_benchmark_metadata()
    general.idle_wait_frames(100)
    for i in range(1, 101):
        benchmarker.capture_pass_timestamp(i)
        benchmarker.capture_cpu_frame_time(i)
    general.exit_game_mode()
    helper.wait_for_condition(function=lambda: not general.is_in_game_mode(), timeout_in_seconds=2.0)


if __name__ == "__main__":
    run()
