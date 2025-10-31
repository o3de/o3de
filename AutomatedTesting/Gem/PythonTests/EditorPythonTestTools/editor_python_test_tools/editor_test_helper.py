#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# NOTE: This code is used for tests in several feature areas. If changes are made to this file, please verify all
# dependent tests continue to run without issue.
#

import sys
import time
from typing import Sequence

# Open 3D Engine specific imports
import azlmbr.legacy.general as general
import azlmbr.legacy.settings as settings

from editor_python_test_tools.utils import Report


class EditorTestHelper:
    def __init__(self, log_prefix: str, args: Sequence[str] = None) -> None:
        self.log_prefix = log_prefix + ":  "
        self.test_success = True

        # If the idle loop has already been enabled at test init time, the Editor is already running.
        # If that's the case, we'll skip the "exit_no_prompt" at the end.
        self.editor_already_running = general.is_idle_enabled()

        self.args = {}
        if args:
            # Get the level name and heightmap name from command-line args
            if len(sys.argv) == (len(args) + 1):
                for arg_index in range(len(args)):
                    self.args[args[arg_index]] = sys.argv[arg_index + 1]
            else:
                self.test_success = False
                self.log(f"Expected command-line args:  {args}")
                self.log(f"Check that cfg_args were passed into the test class")

    # Test Setup
    # Set helpers
    # Set viewport size
    # Turn off display mode, antialiasing
    # set log prefix, log test started
    def setup(self) -> None:
        self.log("test started")

    def after_level_load(self, bypass_viewport_resize: bool = False) -> bool:
        success = True

        # Enable the Editor to start running its idle loop.
        # This is needed for Python scripts passed into the Editor startup.  Since they're executed
        # during the startup flow, they run before idle processing starts.  Without this, the engine loop
        # won't run during idle_wait, which will prevent our test level from working.
        general.idle_enable(True)

        # Give everything a second to initialize
        general.idle_wait(1.0)
        general.update_viewport()
        general.idle_wait(0.5)  # half a second is more than enough for updating the viewport.
        self.original_settings = settings.get_misc_editor_settings()
        self.helpers_visible = general.is_helpers_shown()
        self.viewport_size = general.get_viewport_size()
        self.viewport_layout = general.get_view_pane_layout()
        # Turn off the helper gizmos if visible
        if self.helpers_visible:
            general.toggle_helpers()
            general.idle_wait(1.0)
        # Close the Error Report window so it doesn't interfere with testing hierarchies and focus
        if general.is_pane_visible("Error Report"):
            general.close_pane("Error Report")
        if general.is_pane_visible("Error Log"):
            general.close_pane("Error Log")
        general.idle_wait(1.0)

        if not bypass_viewport_resize:
            # Set Editor viewport to a well-defined size
            screen_width = 1600
            screen_height = 900
            general.set_viewport_expansion_policy("FixedSize")
            general.set_viewport_size(screen_width, screen_height)
            general.update_viewport()
            general.idle_wait(1.0)
            new_viewport_size = general.get_viewport_size()
            new_viewport_width = int(new_viewport_size.x)
            new_viewport_height = int(new_viewport_size.y)

            if (new_viewport_width != screen_width) or (new_viewport_height != screen_height):
                self.log(
                    f"set_viewport_size failed - expected ({screen_width},{screen_height}), got ({new_viewport_width},{new_viewport_height})"
                )
                self.test_success = False
                success = False

        return success

    # Test Teardown
    # Restore everything from above
    # log test results, exit editor
    def teardown(self) -> None:
        # Restore the original Editor settings
        settings.set_misc_editor_settings(self.original_settings)
        # If the helper gizmos were on at the start, restore them
        if self.helpers_visible:
            general.toggle_helpers()

        # Set the viewport back to whatever size it was at the start and restore the pane layout
        general.set_viewport_size(int(self.viewport_size.x), int(self.viewport_size.y))
        general.set_viewport_expansion_policy("AutoExpand")
        # Temporarily disabling reset of view pane layout: LYN-3120
        # general.set_view_pane_layout(self.viewport_layout)
        general.update_viewport()

        self.log("test finished")

        if self.test_success:
            self.log("result=SUCCESS")
            general.set_result_to_success()
        else:
            self.log("result=FAILURE")
            general.set_result_to_failure()

        if not self.editor_already_running:
            general.exit_no_prompt()

    def run_test(self) -> None:
        self.log("run")

    def run(self) -> None:
        self.setup()

        # Only run the actual test if we didn't have setup issues
        if self.test_success:
            self.run_test()

        self.teardown()

    def get_arg(self, arg_name: str) -> str:
        if arg_name in self.args:
            return self.args[arg_name]
        return ""

    # general logger that adds prefix?
    def log(self, log_line: str) -> None:
        Report.info(self.log_prefix + log_line)

    # isclose:  Compares two floating-point values for "nearly-equal"
    def isclose(self, a: float, b: float, rel_tol: float = 1e-9, abs_tol: float = 0.0) -> bool:
        return abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)

    # Create a new empty level
    def create_level(
        self,
        template_name: str,
        level_name: str,
        heightmap_resolution: int = 1024,
        heightmap_meters_per_pixel: int = 1,
        terrain_texture_resolution: int = 4096,
        use_terrain: bool = False,
        bypass_viewport_resize: bool = False,
    ) -> bool:
        self.log(f"Creating level {level_name} from template '{template_name}'")
        result = general.create_level_no_prompt(
            template_name, level_name, heightmap_resolution, heightmap_meters_per_pixel, terrain_texture_resolution, use_terrain
        )

        # Result codes are ECreateLevelResult defined in CryEdit.h
        if result == 1:
            self.log(f"{level_name} level already exists")
        elif result == 2:
            self.log("Failed to create directory")
        elif result == 3:
            self.log("Directory length is too long")
        elif result != 0:
            self.log("Unknown error, failed to create level")
        else:
            self.log(f"{level_name} level created successfully")

        # If the editor is already running, allow "level already exists" to count as success
        if (result == 0) or (self.editor_already_running and (result == 1)):
            # For successful level creation, call the post-load step.
            if self.after_level_load(bypass_viewport_resize):
                result = 0
            else:
                result = -1

        return result == 0

    def open_level(self, level_name: str, bypass_viewport_resize: bool = False) -> bool:
        # Open the level non-interactively
        if self.editor_already_running and (general.get_current_level_name() == level_name):
            self.log(f"Level {level_name} already open")
            result = True
        else:
            self.log(f"Opening level {level_name}")
            result = general.open_level_no_prompt(level_name)

        result = result and self.after_level_load(bypass_viewport_resize)
        if result:
            self.log(f"Successfully opened {level_name}")
        else:
            self.log(f"Unknown error, {level_name} level failed to open")

        return result

    # Take Screenshot
    def take_viewport_screenshot(
        self, posX: float, posY: float, posZ: float, rotX: float, rotY: float, rotZ: float
    ) -> None:
        # Set our camera position / rotation and wait for the Editor to acknowledge it
        general.set_current_view_position(posX, posY, posZ)
        general.set_current_view_rotation(rotX, rotY, rotZ)
        general.idle_wait(1.0)
        # Request a screenshot and wait for the Editor to process it
        general.run_console("r_GetScreenShot=2")
        general.idle_wait(1.0)

    def enter_game_mode(self, success_message: str) -> None:
        """
        :param success_message: The str with the expected message for entering game mode.

        :return: None
        """
        Report.info("Entering game mode")
        general.enter_game_mode()
        general.idle_wait_frames(1)
        self.critical_result(success_message, general.is_in_game_mode())

    def exit_game_mode(self, success_message: str) -> None:
        """
        :param success_message: The str with the expected message for exiting game mode.

        :return: None
        """
        Report.info("Exiting game mode")
        general.exit_game_mode()
        general.idle_wait_frames(1)
        self.critical_result(success_message, not general.is_in_game_mode())

    def critical_result(self, success_message: str, condition: bool, fast_fail_message: str = None) -> None:
        """
        if condition is False we will fail fast

        :param success_message: messages to print based on the condition
        :param condition: success (True) or failure (False)
        :param fast_fail_message: [optional] message to include on fast fail
        """
        if not isinstance(condition, bool):
            raise TypeError("condition argument must be a bool")

        if not Report.result(success_message, condition):
            self.test_success = False
            self.fail_fast(fast_fail_message)

    def fail_fast(self, message: str = None) -> None:
        """
        A state has been reached where progressing in the test is not viable.
        raises FailFast
        :return: None
        """
        Report.info("Failing fast. Raising an exception and shutting down the editor.")
        if message:
            Report.info(f"Fail fast message: {message}")
        self.teardown()
        raise RuntimeError

    def wait_for_condition(self, function, timeout_in_seconds=1.0):
        # type: (function, float) -> bool
        """
        **** Will be replaced by a function of the same name exposed in the Engine*****
        a function to run until it returns True or timeout is reached
        the function can have no parameters and
        waiting idle__wait_* is handled here not in the function

        :param function: a function that returns a boolean indicating a desired condition is achieved
        :param timeout_in_seconds: when reached, function execution is abandoned and False is returned
        """

        with Timeout(timeout_in_seconds) as t:
            while True:
                try:
                    general.idle_wait_frames(1)
                except Exception:
                    print("WARNING: Couldn't wait for frame")

                if t.timed_out:
                    return False

                ret = function()
                if not isinstance(ret, bool):
                    raise TypeError("return value for wait_for_condition function must be a bool")
                if ret:
                    return True


class Timeout:
    # type: (float) -> None
    """
    contextual timeout
    :param seconds: float seconds to allow before timed_out is True
    """

    def __init__(self, seconds):
        self.seconds = seconds

    def __enter__(self):
        self.die_after = time.time() + self.seconds
        return self

    def __exit__(self, type, value, traceback):
        pass

    @property
    def timed_out(self):
        return time.time() > self.die_after
