"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
from utils import Report
from utils import TestHelper as helper
from utils import Tracer
import editor_python_test_tools.pyside_utils as pyside_utils
import azlmbr.legacy.general as general
import azlmbr.paths as paths

# fmt: off
class Tests():
    entitya_created = ("Successfully created EntityA",         "Failed to create EntityA")
    entityb_created = ("Successfully created EntityB",         "Failed to create EntityB")
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on

LEVEL_NAME = "Base"
WAIT_TIME = 3.0
ASSET_PREFIX = "T92567321"
asset_paths = {
    "event": os.path.join("TestAssets", f"{ASSET_PREFIX}.scriptevents"),
    "assetA": os.path.join("ScriptCanvas", f"{ASSET_PREFIX}A.scriptcanvas"),
    "assetB": os.path.join("ScriptCanvas", f"{ASSET_PREFIX}B.scriptcanvas"),
}
sc_for_entities = {"EntityA": asset_paths["assetA"], "EntityB": asset_paths["assetB"]}
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
     3) Start Tracer
     4) Enter Game Mode
     5) Read for line
     6) Exit Game Mode


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    def __init__(self):
        editor_window = None

    def create_editor_entity(self, name, sc_asset):
        entity = Entity.create_editor_entity(name)
        sc_comp = entity.add_component("Script Canvas")
        sc_comp.set_component_property_value("Script Canvas Asset|Script Canvas Asset", get_asset(sc_asset))
        Report.critical_result(Tests.__dict__[name.lower() + "_created"], entity.id.isValid())

    def locate_expected_lines(self, line_list: list, section_tracer ):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]

        return all(line in found_lines for line in line_list)

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Create temp level

        result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
        Report.critical_result(Tests.level_created, result == 0)
        helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
        general.close_pane("Error Report")

        # 2) Create EntityA/EntityB
        for key in sc_for_entities.keys():
            create_editor_entity(key, sc_for_entities[key])

        # 3) Start Tracer
        with Tracer() as section_tracer:

            # 4) Enter Game Mode
            helper.enter_game_mode(Tests.enter_game_mode)

            # 5) Read for line
            lines_located = helper.wait_for_condition(lambda: locate_expected_lines(EXPECTED_LINES, section_tracer), WAIT_TIME)
            Report.result(Tests.lines_found, lines_located)

        # 6) Exit Game Mode
        helper.exit_game_mode(Tests.exit_game_mode)


test = ScriptEvents_HappyPath_SendReceiveAcrossMultiple()
test.run_test()

