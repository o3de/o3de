"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This hydra/EPB script opens and closes every possible Atom level.
"""
import os
import sys

import azlmbr.legacy.general as general
import azlmbr.paths

sys.path.append(os.path.join(azlmbr.paths.devroot, 'AutomatedTesting', 'Gem', 'PythonTests'))

from automatedtesting_shared.editor_test_helper import EditorTestHelper

levels = os.listdir(os.path.join(azlmbr.paths.devroot, "AutomatedTesting", "Levels", "AtomLevels"))
helper = EditorTestHelper(log_prefix="HydraAtomLevels")


def run():
    """
    1. Open & close all valid test levels in the Editor.
    2. Every time a level is opened, verify it loads correctly and the Editor remains stable.
    """

    # Create a new level.
    level_name = "tmp_level"  # Defined in test_Atom_MainSuite.py
    heightmap_resolution = 512
    heightmap_meters_per_pixel = 1
    terrain_texture_resolution = 412
    use_terrain = False

    # Return codes are ECreateLevelResult defined in CryEdit.h
    return_code = general.create_level_no_prompt(
        f"AtomLevels/{level_name}",
        heightmap_resolution,
        heightmap_meters_per_pixel,
        terrain_texture_resolution,
        use_terrain
    )
    if return_code == 1:
        general.log(f"AtomLevels/{level_name} level already exists")
    elif return_code == 2:
        general.log("Failed to create directory")
    elif return_code == 3:
        general.log("Directory length is too long")
    elif return_code != 0:
        general.log("Unknown error, failed to create level")
    else:
        general.log(f"AtomLevels/{level_name} level created successfully")
    after_level_load()

    # Open all valid AtomLevels.
    failed_to_open = []
    levels.append(level_name)
    for level in levels:
        if general.is_idle_enabled() and (general.get_current_level_name() == level):
            general.log(f"Level {level} already open.")
        else:
            general.log(f"Opening level {level}")
            general.open_level_no_prompt(f"AtomLevels/{level}")
            helper.wait_for_condition(function=lambda: general.get_current_level_name() == level,
                                      timeout_in_seconds=4.0)
        result = (general.get_current_level_name() == level) and after_level_load()
        if result:
            general.log(f"Successfully opened {level}")
        else:
            general.log(f"{level} failed to open")
            failed_to_open.append(level)

    if failed_to_open:
        general.log(f"The following levels failed to open: {failed_to_open}")


def after_level_load():
    """Function to call after creating/opening a level to ensure it loads."""
    # Give everything a second to initialize.
    general.idle_enable(True)
    general.update_viewport()
    general.idle_wait(0.5)  # half a second is more than enough for updating the viewport.

    # Close out problematic windows, FPS meters, and anti-aliasing.
    if general.is_helpers_shown():  # Turn off the helper gizmos if visible
        general.toggle_helpers()
    if general.is_pane_visible("Error Report"):  # Close Error Report windows that block focus.
        general.close_pane("Error Report")
    if general.is_pane_visible("Error Log"):  # Close Error Log windows that block focus.
        general.close_pane("Error Log")
    general.run_console("r_displayInfo=0")
    general.run_console("r_antialiasingmode=0")

    return True


if __name__ == "__main__":
    run()
