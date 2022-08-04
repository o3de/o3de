"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import pyside_utils
from editor_python_test_tools.utils import TestHelper as helper
from editor_python_test_tools.utils import Report, Tracer
import editor_python_test_tools.hydra_editor_utils as hydra
import scripting_utils.scripting_tools as scripting_tools
import azlmbr.legacy.general as general
import azlmbr.paths as paths
from scripting_utils.scripting_constants import (BASE_LEVEL_NAME, WAIT_TIME_3, SCRIPT_CANVAS_COMPONENT_PROPERTY_PATH)


# fmt: off
class Tests:
    entity_created     = ("New entity created",              "New entity not created")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    lines_found       = ("Expected log lines were found",  "Expected log lines were not found")
# fmt: on


EXPECTED_LINES = ["Greetings from the first script", "Greetings from the second script"]
SOURCE_FILES = [os.path.join(paths.projectroot, "ScriptCanvas", "ScriptCanvas_TwoComponents0.scriptcanvas"),
                os.path.join(paths.projectroot, "ScriptCanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")]
TEST_ENTITY_NAME = "test_entity"

class TestScriptCanvas_TwoComponents_InteractSuccessfully:
    """
    Summary:
     A test entity contains two Script Canvas components with different unique script canvas files.
     Each of these files will have a print node set to activate on graph start.

    Expected Behavior:
     When game mode is entered, two unique strings should be printed out to the console

    Test Steps:
     1) Create level
     2) Create entity with SC components
     3) Enter game mode
     4) Wait for expected lines to be found
     5) Report if expected lines were found
     6) Exit game mode

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    def __init__(self):
        self.editor_main_window = None

    @pyside_utils.wrap_async
    async def run_test(self):

        # Preconditions
        general.idle_enable(True)

        # 1) Create level
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == BASE_LEVEL_NAME, WAIT_TIME_3)
        general.close_pane("Error Report")

        # 2) Create entity with SC components
        entity = scripting_tools.create_entity_with_multiple_sc_component_asset(TEST_ENTITY_NAME, SOURCE_FILES)
        helper.wait_for_condition(lambda: entity is not None, WAIT_TIME_3)
        Report.critical_result(Tests.entity_created, entity.id.isValid())

        with Tracer() as section_tracer:

            # 3) Enter game mode
            helper.enter_game_mode(Tests.game_mode_entered)

            # 4) Wait for expected lines to be found
            lines_located = helper.wait_for_condition(
                lambda: scripting_tools.located_expected_tracer_lines(self, section_tracer, EXPECTED_LINES),
                WAIT_TIME_3)
            Report.result(Tests.lines_found, lines_located)


        # 5) Exit game mode
        helper.exit_game_mode(Tests.game_mode_exited)


test = TestScriptCanvas_TwoComponents_InteractSuccessfully()
test.run_test()
