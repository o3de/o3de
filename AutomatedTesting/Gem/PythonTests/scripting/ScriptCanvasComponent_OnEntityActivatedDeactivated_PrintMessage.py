"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import editor_python_test_tools.pyside_utils as pyside_utils
import editor_python_test_tools.hydra_editor_utils as hydra
from editor_python_test_tools.utils import Report, Tracer
from editor_python_test_tools.utils import TestHelper as helper
import scripting_utils.scripting_tools as scripting_tools
import azlmbr.legacy.general as general
import azlmbr.paths as paths
from scripting_utils.scripting_constants import (WAIT_TIME_3, BASE_LEVEL_NAME)

EXPECTED_LINES = ["Activator Script: Activated", "Deactivator Script: Deactivated"]
controller_dict = {
    "name": "Controller",
    "status": "active",
    "path": os.path.join(paths.projectroot, "ScriptCanvas", "OnEntityActivatedScripts", "controller.scriptcanvas"),
}
activated_dict = {
    "name": "ActivationTest",
    "status": "inactive",
    "path": os.path.join(paths.projectroot, "ScriptCanvas", "OnEntityActivatedScripts", "activator.scriptcanvas"),
}
deactivated_dict = {
    "name": "DeactivationTest",
    "status": "active",
    "path": os.path.join(paths.projectroot, "ScriptCanvas", "OnEntityActivatedScripts", "deactivator.scriptcanvas"),
}
ENTITY_TO_ACTIVATE_PATH = "Configuration|Properties|Variables|EntityToActivate|Datum|Datum|value|EntityToActivate"
ENTITY_TO_DEACTIVATE_PATH = "Configuration|Properties|Variables|EntityToDeactivate|Datum|Datum|value|EntityToDeactivate"
WAIT_ONE_SECOND = 1.0
# fmt: off
class Tests():
    controller_exists    = ("Successfully found controller entity",  "Failed to find controller entity")
    activated_exists     = ("Successfully found activated entity",   "Failed to find activated entity")
    deactivated_exists   = ("Successfully found deactivated entity", "Failed to find deactivated entity")
    start_states_correct = ("Start states set up successfully",      "Start states set up incorrectly")
    game_mode_entered    = ("Successfully entered game mode",         "Failed to enter game mode")
    lines_found          = ("Successfully found expected prints",    "Failed to find expected prints")
    game_mode_exited     = ("Successfully exited game mode", "Failed to exit game mode")
# fmt: on

class ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage():
    """
    Summary:
     Verify that the On Entity Activated/On Entity Deactivated nodes are working as expected

    Expected Behavior:
     Upon entering game mode, the Controller entity will wait 1 second and then activate the ActivationTest
     entity. The script attached to ActivationTest will print out a message on activation. The Controller
     will also deactivate the DeactivationTest entity, which should print a message.

    Test Steps:
     1) Create temp level
     2) Create all the entities we need for the test
     3) Validate the entities were created and configured
     4) Start the Tracer
     5) Enter Game Mode
     6) Wait one second for graph timers then exit game mode
     7) Validate Print message

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    def __init__(self):
        self.editor_main_window = None

    def setup_level_entities(self):

        activated_entity = scripting_tools.create_entity_with_sc_component_asset(activated_dict["name"], activated_dict["path"])
        scripting_tools.change_entity_start_status(activated_dict["name"], activated_dict["status"])
        helper.wait_for_condition(lambda: activated_entity is not None, WAIT_TIME_3)

        deactivated_entity = scripting_tools.create_entity_with_sc_component_asset(deactivated_dict["name"], deactivated_dict["path"])
        scripting_tools.change_entity_start_status(deactivated_dict["name"], deactivated_dict["status"])
        helper.wait_for_condition(lambda: deactivated_entity is not None, WAIT_TIME_3)

        self.setup_controller_entity(controller_dict)

    def setup_controller_entity(self, entity_dict: dict):
        """
        create entity using hydra and editor entity library but also drill into its exposed variables and set values.

        """
        entity = scripting_tools.create_entity_with_sc_component_asset(entity_dict["name"], entity_dict["path"])
        scripting_tools.change_entity_start_status(entity_dict["name"], entity_dict["status"])

        scripting_tools.change_entity_sc_variable_entity(entity_dict["name"], activated_dict["name"],
                                                         ENTITY_TO_ACTIVATE_PATH)
        scripting_tools.change_entity_sc_variable_entity(entity_dict["name"], deactivated_dict["name"],
                                                         ENTITY_TO_DEACTIVATE_PATH)

        helper.wait_for_condition(lambda: entity is not None, WAIT_TIME_3)

    def validate_entity_state(self, entity_tuple, test):
        """
        Function to make sure the entities created for this test were properly added to the level

        """

        entity = scripting_tools.validate_entity_exists_by_name(entity_tuple["name"], test)
        state_correct = scripting_tools.validate_entity_start_state_by_name(entity_tuple["name"], entity_tuple["status"])

        return state_correct

    def validate_test_entities(self):

        entities_valid = (self.validate_entity_state(activated_dict, Tests.activated_exists) and
                        self.validate_entity_state(deactivated_dict, Tests.deactivated_exists) and
                        self.validate_entity_state(controller_dict, Tests.controller_exists))

        Report.critical_result(Tests.start_states_correct, entities_valid)


    @pyside_utils.wrap_async
    async def run_test(self):

        # 1) Create temp level
        general.idle_enable(True)
        hydra.open_base_level()
        helper.wait_for_condition(lambda: general.get_current_level_name() == BASE_LEVEL_NAME, WAIT_TIME_3)
        general.close_pane("Error Report")

        # 2) create all the entities we need for the test
        self.setup_level_entities()

        # 3) Validate the entities were created and configured
        self.validate_test_entities()

        # 4) Start the Tracer
        with Tracer() as section_tracer:

            # 5) Enter Game Mode
            helper.enter_game_mode(Tests.game_mode_entered)

            # 6 Wait one second for graph timers then exit game mode
            general.idle_wait(WAIT_ONE_SECOND)
            helper.exit_game_mode(Tests.game_mode_exited)

            # 7) Validate Print message
            found_expected_lines = scripting_tools.located_expected_tracer_lines(self, section_tracer, EXPECTED_LINES)
            helper.wait_for_condition(lambda: found_expected_lines is not None, WAIT_TIME_3)

        Report.result(Tests.lines_found, found_expected_lines)



test = ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage()
test.run_test()
