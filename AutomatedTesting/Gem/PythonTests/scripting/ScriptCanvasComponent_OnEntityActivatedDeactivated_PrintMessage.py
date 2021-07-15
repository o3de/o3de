"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    level_created        = ("Successfully created temp level",       "Failed to create temp level")
    controller_exists    = ("Successfully found controller entity",  "Failed to find controller entity")
    activated_exists     = ("Successfully found activated entity",   "Failed to find activated entity")
    deactivated_exists   = ("Successfully found deactivated entity", "Failed to find deactivated entity")
    start_states_correct = ("Start states set up successfully",      "Start states set up incorrectly")
    game_mode_entered    = ("Successfully entered game mode"         "Failed to enter game mode")
    lines_found          = ("Successfully found expected prints",    "Failed to find expected prints")
    game_mode_exited     = ("Successfully exited game mode"          "Failed to exit game mode")
# fmt: on


def ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage():
    """
    Summary:
     Verify that the On Entity Activated/On Entity Deactivated nodes are working as expected

    Expected Behavior:
     Upon entering game mode, the Controller entity will wait 1 second and then activate the ActivationTest
     entity. The script attached to ActivationTest will print out a message on activation. The Controller
     will also deactivate the DeactivationTest entity, which should print a message.

    Test Steps:
     1) Create temp level
     2) Setup the level
     3) Validate the entities
     4) Start the Tracer
     5) Enter Game Mode
     6) Validate Print message
     7) Exit game mode

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os

    from editor_entity_utils import EditorEntity as Entity
    from utils import Report
    from utils import TestHelper as helper
    from utils import Tracer

    import azlmbr.legacy.general as general

    EditorEntity = str
    LEVEL_NAME = "tmp_level"
    WAIT_TIME = 3.0  # SECONDS
    EXPECTED_LINES = ["Activator Script: Activated", "Deactivator Script: Deactivated"]
    controller_dict = {
        "name": "Controller",
        "status": "active",
        "path": os.path.join("ScriptCanvas", "OnEntityActivatedScripts", "controller.scriptcanvas"),
    }
    activated_dict = {
        "name": "ActivationTest",
        "status": "inactive",
        "path": os.path.join("ScriptCanvas", "OnEntityActivatedScripts", "activator.scriptcanvas"),
    }
    deactivated_dict = {
        "name": "DeactivationTest",
        "status": "active",
        "path": os.path.join("ScriptCanvas", "OnEntityActivatedScripts", "deactivator.scriptcanvas"),
    }

    def get_asset(asset_path):
        return azlmbr.asset.AssetCatalogRequestBus(
            azlmbr.bus.Broadcast, "GetAssetIdByPath", asset_path, azlmbr.math.Uuid(), False
        )

    def setup_level():
        def create_editor_entity(
            entity_dict: dict, entity_to_activate: EditorEntity = None, entity_to_deactivate: EditorEntity = None
        ) -> EditorEntity:
            entity = Entity.create_editor_entity(entity_dict["name"])
            entity.set_start_status(entity_dict["status"])
            sc_component = entity.add_component("Script Canvas")
            sc_component.set_component_property_value(
                "Script Canvas Asset|Script Canvas Asset", get_asset(entity_dict["path"])
            )

            if entity_dict["name"] == "Controller":
                sc_component.get_property_tree()
                sc_component.set_component_property_value(
                    "Properties|Variable Fields|Variables|[0]|Name,Value|Datum|Datum|EntityToActivate",
                    entity_to_activate.id,
                )
                sc_component.set_component_property_value(
                    "Properties|Variable Fields|Variables|[1]|Name,Value|Datum|Datum|EntityToDeactivate",
                    entity_to_deactivate.id,
                )
            return entity

        activated = create_editor_entity(activated_dict)
        deactivated = create_editor_entity(deactivated_dict)
        create_editor_entity(controller_dict, activated, deactivated)

    def validate_entity_exist(entity_name: str, test_tuple: tuple):
        """
        Validate the entity with the given name exists in the level
        :return: entity: editor entity object
        """
        entity = Entity.find_editor_entity(entity_name)
        Report.critical_result(test_tuple, entity.id.IsValid())
        return entity

    def validate_start_state(entity: EditorEntity, expected_state: str):
        """
        Validate that the starting state of the entity is correct, if it isn't then attempt to rectify and recheck.
        :return: bool: Whether state is set as expected
        """
        state_options = {
            "active": azlmbr.globals.property.EditorEntityStartStatus_StartActive,
            "inactive": azlmbr.globals.property.EditorEntityStartStatus_StartInactive,
            "editor": azlmbr.globals.property.EditorEntityStartStatus_EditorOnly,
        }
        if expected_state.lower() not in state_options.keys():
            raise ValueError(f"{expected_state} is an invalid option; valid options: active, inactive, or editor.")

        state = entity.get_start_status()
        if state != state_options[expected_state]:
            # If state fails to set, set_start_status will assert
            entity.set_start_status(expected_state)
        return True

    def validate_entities_in_level():
        controller = validate_entity_exist(controller_dict["name"], Tests.controller_exists)
        state1_correct = validate_start_state(controller, controller_dict["status"])

        act_tester = validate_entity_exist(activated_dict["name"], Tests.activated_exists)
        state2_correct = validate_start_state(act_tester, activated_dict["status"])

        deac_tester = validate_entity_exist(deactivated_dict["name"], Tests.deactivated_exists)
        state3_correct = validate_start_state(deac_tester, deactivated_dict["status"])

        all_states_correct = state1_correct and state2_correct and state3_correct
        Report.critical_result(Tests.start_states_correct, all_states_correct)

    def locate_expected_lines(line_list: list):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]
        return all(line in found_lines for line in line_list)

    # 1) Create temp level
    general.idle_enable(True)
    result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Setup the level
    setup_level()

    # 3) Validate the entities
    validate_entities_in_level()

    # 4) Start the Tracer
    with Tracer() as section_tracer:

        # 5) Enter Game Mode
        helper.enter_game_mode(Tests.game_mode_entered)

        # 6) Validate Print message
        helper.wait_for_condition(lambda: locate_expected_lines(EXPECTED_LINES), WAIT_TIME)

    Report.result(Tests.lines_found, locate_expected_lines(EXPECTED_LINES))

    # 7) Exit game mode
    helper.exit_game_mode(Tests.game_mode_exited)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvasComponent_OnEntityActivatedDeactivated_PrintMessage)
