"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import editor_python_test_tools.pyside_utils as pyside_utils
from editor_python_test_tools.utils import Report, Tracer
from editor_python_test_tools.utils import TestHelper as helper
import editor_python_test_tools.hydra_editor_utils as hydra
import azlmbr.scriptcanvas as scriptcanvas
import azlmbr.legacy.general as general
import azlmbr.asset as asset
import azlmbr.math as math
import azlmbr.bus as bus

# fmt: off
class Tests():
    entity_created  = ("Successfully created test entity",     "Failed to create test entity")
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on

LEVEL_NAME = "Base"
WAIT_TIME = 3.0  # SECONDS
EXPECTED_LINES = ["T92567320: Message Received"]
SC_ASSET_PATH = os.path.join("ScriptCanvas", "T92567320.scriptcanvas")

class ScriptEvents_Default_SendReceiveSuccessfully:
    """
    Summary:
     An entity exists in the level that contains a Script Canvas component. In the graph is both a Send Event
     and a Receive Event.

    Expected Behavior:
     After entering game mode the graph on the entity should print an expected message to the console

    Test Steps:
     1) Create test level
     2) Create test entity
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

        sourcehandle = scriptcanvas.SourceHandleFromPath(SC_ASSET_PATH)

        entity = hydra.Entity(name)
        entity.create_entity(math.Vector3(512.0, 512.0, 32.0), ["Script Canvas"])

        script_canvas_component = entity.components[0]
        hydra.set_component_property_value(script_canvas_component, "Configuration|Source", sourcehandle)

        Report.critical_result(Tests.entity_created, entity.id.isValid())
        return entity


    def locate_expected_lines(self, line_list: list, section_tracer):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]

        return all(line in found_lines for line in line_list)

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Create temp level
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
        general.close_pane("Error Report")

        # 2) Create test entity
        self.create_editor_entity("TestEntity", SC_ASSET_PATH)

        # 3) Start Tracer
        with Tracer() as section_tracer:

            # 4) Enter Game Mode
            helper.enter_game_mode(Tests.enter_game_mode)

            # 5) Read for line
            lines_located = helper.wait_for_condition(lambda: self.locate_expected_lines(EXPECTED_LINES, section_tracer), WAIT_TIME)
            Report.result(Tests.lines_found, lines_located)

        # 6) Exit Game Mode
        helper.exit_game_mode(Tests.exit_game_mode)


test = ScriptEvents_Default_SendReceiveSuccessfully()
test.run_test()
