#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import sys
import time
import azlmbr.legacy.general as general
import azlmbr.legacy.settings as settings


class EditorTestHelper:
    def __init__(self, log_prefix, args=None):
        self.log_prefix = log_prefix + ":  "
        self.test_success = True
        self.args = {}
        if args:
            # Get the level name and heightmap name from command-line args

            if len(sys.argv) == (len(args) + 1):
                for arg_index in range(len(args)):
                    self.args[args[arg_index]] = sys.argv[arg_index + 1]
            else:
                test_success = False
                self.log("Expected command-line args:  {}".format(args))

    # Test Setup
    # Set helpers
    # Set viewport size
    # Turn off display mode, antialiasing
    # set log prefix, log test started
    def setup(self):
        self.log("test started")

    def after_level_load(self):
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
        # Turn off the helper gizmos if visible
        if self.helpers_visible:
            general.toggle_helpers()
            general.idle_wait(1.0)
        # Close the Error Report window so it doesn't interfere with testing hierarchies and focus
        general.close_pane("Error Report")
        # Set Editor viewport to a well-defined size
        screen_width = 1600
        screen_height = 900
        general.set_viewport_expansion_policy("FixedSize")
        general.set_viewport_size(screen_width, screen_height)
        general.update_viewport()
        general.idle_wait(1.0)
        new_viewport_size = general.get_viewport_size()
        new_viewport_width = new_viewport_size.x
        new_viewport_height = new_viewport_size.y

        if (new_viewport_width != screen_width) or (new_viewport_height != screen_height):
            self.log(
                "set_viewport_size failed - expected ({},{}), got ({},{})".format(
                    screen_width, screen_height, new_viewport_size[0], new_viewport_size[1]
                )
            )
            self.test_success = False
            success = False
        # Turn off any display info like FPS, as that will mess up our image comparisons
        # Turn off antialiasing as well
        general.run_console("r_displayInfo=0")
        general.run_console("r_antialiasingmode=0")
        general.idle_wait(1.0)
        return success

    # Test Teardown
    # Restore everything from above
    # log test results, exit editor
    def teardown(self):
        # Restore the original Editor settings
        if hasattr(self, 'original_settings'):
            settings.set_misc_editor_settings(self.original_settings)
        # If the helper gizmos were on at the start, restore them
        if hasattr(self, 'helpers_visible') and self.helpers_visible:
            general.toggle_helpers()

        # Set the viewport back to whatever size it was at the start
        # Hydra exposes members of a Vector2 via .x, .y.
        if hasattr(self, 'viewport_size'):
            general.set_viewport_size(self.viewport_size.x, self.viewport_size.y)

            general.update_viewport()
            general.idle_wait(1.0)

        self.log("test finished")

        if self.test_success:
            self.log("result=SUCCESS")
            general.set_result_to_success()
        else:
            self.log("result=FAILURE")
            general.set_result_to_failure()

        general.exit_no_prompt()

    def run_test(self):
        self.log("run")

    def run(self):
        self.setup()

        # Only run the actual test if we didn't have setup issues
        if self.test_success:
            self.run_test()

        self.teardown()

    def get_arg(self, arg_name):
        if arg_name in self.args:
            return self.args[arg_name]
        return ""

    # general logger that adds prefix?
    def log(self, log_line):
        general.log(self.log_prefix + log_line)

    # isclose:  Compares two floating-point values for "nearly-equal"
    # From https://www.python.org/dev/peps/pep-0485/#proposed-implementation :
    def isclose(self, a, b, rel_tol=1e-9, abs_tol=0.0):
        return abs(a - b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)

    # Create a new empty level
    def create_level(
        self, level_name, heightmap_resolution, heightmap_meters_per_pixel, terrain_texture_resolution, use_terrain
    ):
        self.log("Creating level {}".format(level_name))
        result = general.create_level_no_prompt(
            level_name, heightmap_resolution, heightmap_meters_per_pixel, terrain_texture_resolution, use_terrain
        )

        # Result codes are ECreateLevelResult defined in CryEdit.h
        if result == 1:
            self.log("Temp level already exists")
        elif result == 2:
            self.log("Failed to create directory")
        elif result == 3:
            self.log("Directory length is too long")
        elif result != 0:
            self.log("Unknown error, failed to create level")
        else:
            self.log("Level created successfully")
            if not self.after_level_load():
                result = -1

        return result == 0

    def open_level(self, level_name):
        # Open the level non-interactively
        self.log("Opening level {}".format(level_name))
        result = general.open_level_no_prompt(level_name)
        result = result and self.after_level_load()
        if result:
            self.log("Level opened successfully")
        else:
            self.log("Unknown error, level failed to open")

        return result

    # Take Screenshot
    def take_viewport_screenshot(self, posX, posY, posZ, rotX, rotY, rotZ):
        # Set our camera position / rotation and wait for the Editor to acknowledge it
        general.set_current_view_position(posX, posY, posZ)
        general.set_current_view_rotation(rotX, rotY, rotZ)
        general.idle_wait(1.0)
        # Request a screenshot and wait for the Editor to process it
        general.run_console("r_GetScreenShot=2")
        general.idle_wait(1.0)

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
