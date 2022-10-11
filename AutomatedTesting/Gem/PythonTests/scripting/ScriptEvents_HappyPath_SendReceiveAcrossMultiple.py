"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report, Tracer
import pyside_utils
import azlmbr.legacy.general as general
import editor_python_test_tools.hydra_editor_utils as hydra
import azlmbr.paths as paths
import scripting_utils.scripting_tools as scripting_tools
from scripting_utils.scripting_constants import (WAIT_TIME_3, BASE_LEVEL_NAME)

# fmt: off
class Tests:
    entity_created = ("Successfully created Entity",         "Failed to create Entity")
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on


ASSET_PREFIX = "T92567321"
asset_paths = {
    "event": os.path.join(paths.projectroot, "TestAssets", f"{ASSET_PREFIX}.scriptevents"),
    "assetA": os.path.join(paths.projectroot, "ScriptCanvas", f"{ASSET_PREFIX}A.scriptcanvas"),
    "assetB": os.path.join(paths.projectroot, "ScriptCanvas", f"{ASSET_PREFIX}B.scriptcanvas"),
}
ENTITY_NAME_FILEPATH_MAP = {"EntityA": asset_paths["assetA"], "EntityB": asset_paths["assetB"]}
EXPECTED_LINES = ["Incoming Message Received"]


class ScriptEvents_HappyPath_SendReceiveAcrossMultiple:
    """
    Summary:
     EntityA and EntityB will be created in a level. Attached to both will be a Script Canvas component.
     The Script Event created for the test will be sent from EntityA to EntityB.

    Expected Behavior:
     The output of the Script Event should be printed to the console

    Test Steps:
     1) Create test level
     2) Create EntityA/EntityB (add scriptcanvas files part of entity setup)
     3) Enter Game Mode
     4) Read for line
     5) Exit Game Mode


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    def __init__(self):
        editor_window = None

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Create temp level
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == BASE_LEVEL_NAME, WAIT_TIME_3)
        general.close_pane("Error Report")

        # 2) Create EntityA/EntityB
        for key_name in ENTITY_NAME_FILEPATH_MAP.keys():
            entity = scripting_tools.create_entity_with_sc_component_asset(key_name, ENTITY_NAME_FILEPATH_MAP[key_name])
            helper.wait_for_condition(lambda: entity is not None, WAIT_TIME_3)
            Report.critical_result(Tests.entity_created, entity.id.isValid())

        with Tracer() as section_tracer:

            # 3) Enter Game Mode
            helper.enter_game_mode(Tests.enter_game_mode)

            # 4) Read for line
            lines_located = helper.wait_for_condition(
                lambda: scripting_tools.located_expected_tracer_lines(self, section_tracer, EXPECTED_LINES), WAIT_TIME_3)
            Report.result(Tests.lines_found, lines_located)

        # 5) Exit Game Mode
        helper.exit_game_mode(Tests.exit_game_mode)


test = ScriptEvents_HappyPath_SendReceiveAcrossMultiple()
test.run_test()
