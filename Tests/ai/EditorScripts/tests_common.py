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

import sys, os
import azlmbr.legacy.general as general
import azlmbr.legacy.settings as settings

class TestHelper:
    def __init__(self, log_prefix, args = None):
        self.log_prefix = log_prefix + ':  '
        self.test_success = True
        self.args = {}
        if args:
            # Get the level name and heightmap name from command-line args
            if (len(sys.argv) == (len(args) + 1)):
                for arg_index in range(len(args)):
                    self.args[args[arg_index]] = sys.argv[arg_index + 1]
            else:
                test_success = False
                self.log('Expected command-line args:  {}'.format(args))


    # Test Setup
        # Set helpers
        # Set viewport size
        # Turn off display mode, antialiasing
        # set log prefix, log test started
        # TODO: Turn off user dialogs like Amazon login, surveys, etc...
    def setup(self):
        self.log('test started')

    def after_level_load(self):
        # Enable the Editor to start running its idle loop.
        # This is needed for Python scripts passed into the Editor startup.  Since they're executed
        # during the startup flow, they run before idle processing starts.  Without this, the engine loop 
        # won't run during idle_wait, which will prevent our test level from working.
        general.idle_enable(True)

        # Give everything a second to initialize
        general.idle_wait(1.0)

        self.original_settings = settings.get_misc_editor_settings()
        self.helpers_visible = general.is_helpers_shown()
        self.viewport_size = general.get_viewport_size()
        # Turn off the helper gizmos if visible
        if (self.helpers_visible):
            general.toggle_helpers()
            general.idle_wait(1.0)

        # Set Editor viewport to a well-defined size
        general.set_viewport_size(1280, 720)
        general.idle_wait(1.0)

        # Turn off any display info like FPS, as that will mess up our image comparisons
        # Turn off antialiasing as well
        general.run_console("r_displayInfo=0")
        general.run_console("r_antialiasingmode=0")
        general.idle_wait(1.0)



    # Test Teardown
    # Restore everything from above
    # log test results, exit editor
    def teardown(self):
        # Restore the original Editor settings
        settings.set_misc_editor_settings(self.original_settings)
        # If the helper gizmos were on at the start, restore them
        if (self.helpers_visible):
            general.toggle_helpers()
        # Set the viewport back to whatever size it was at the start
        general.set_viewport_size(self.viewport_size.x, self.viewport_size.y)
        general.idle_wait(1.0)

        self.log('test finished')

        if self.test_success == True:
            self.log('result=SUCCESS')
            general.set_result_to_success()
        else:
            self.log('result=FAILURE')
            general.set_result_to_failure()

        general.exit_no_prompt()

    def run_test(self):
        self.log('run')

    def run(self):
        self.setup()

        # Only run the actual test if we didn't have setup issues
        if self.test_success:
            self.run_test()

        self.teardown()

    def get_arg(self, arg_name):
        if arg_name in self.args:
            return self.args[arg_name]
        return ''
            

    # general logger that adds prefix?
    def log(self, log_line):
        general.log(self.log_prefix + log_line)

    # isclose:  Compares two floating-point values for "nearly-equal"
    # From https://www.python.org/dev/peps/pep-0485/#proposed-implementation :
    def isclose(self, a, b, rel_tol=1e-9, abs_tol=0.0):
        return abs(a-b) <= max(rel_tol * max(abs(a), abs(b)), abs_tol)


    # Create a new empty level
    def create_level(self, level_name, heightmap_resolution, heightmap_meters_per_pixel, terrain_texture_resolution, use_terrain):
        self.log('Creating level {}'.format(level_name))
        result = general.create_level_no_prompt(level_name, heightmap_resolution, heightmap_meters_per_pixel, terrain_texture_resolution, use_terrain)

        # Result codes are ECreateLevelResult defined in CryEdit.h
        if (result == 1):
            self.log('Temp level already exists')
        elif (result == 2):
            self.log('Failed to create directory')
        elif (result == 3):
            self.log('Directory length is too long')
        elif (result != 0):
            self.log('Unknown error, failed to create level')
        else:
            self.log('Level created successfully')
            self.after_level_load()

        return (result == 0)

    def open_level(self, level_name):
        # Open the level non-interactively
        self.log('Opening level {}'.format(level_name))
        result = general.open_level_no_prompt(level_name)
        self.after_level_load()
        if result:
            self.log('Level opened successfully')
        else:
            self.log('Unknown error, level failed to open')

        return result

    # Take Screenshot
    def take_screenshot(self, posX, posY, posZ, rotX, rotY, rotZ):
        # Set our camera position / rotation and wait for the Editor to acknowledge it
        general.set_current_view_position(posX, posY, posZ)
        general.set_current_view_rotation(rotX, rotY, rotZ)
        general.idle_wait(1.0)
        # Request a screenshot and wait for the Editor to process it
        general.run_console("r_GetScreenShot=2")
        general.idle_wait(1.0)

