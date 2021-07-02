"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Test Case Title: Event can return a value of set type successfully
"""


# fmt: off
class Tests():
    level_created   = ("Successfully created temporary level", "Failed to create temporary level")
    entity_created  = ("Successfully created test entity",     "Failed to create test entity")
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on


def ScriptEvents_ReturnSetType_Successfully():
    """
    Summary: A temporary level is created with an Entity having ScriptCanvas component.
     ScriptEvent(T92569006_ScriptEvent.scriptevents) is created with one Method that has a return value.
     ScriptCanvas(T92569006_ScriptCanvas.scriptcanvas) is attached to Entity. Graph has Send node that sends the Method
     of the ScriptEvent and prints the returned result ( On Entity Activated -> Send node -> Print) and Receive node is
     set to return custom value ( Receive node -> Print).
     Verify that the entity containing T92569006_ScriptCanvas.scriptcanvas should print the custom value set in both
     Send and Receive nodes.

    Expected Behavior:
     After entering game mode, the graph on the entity should print an expected message to the console

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
    import os
    from editor_entity_utils import EditorEntity as Entity
    from utils import Report
    from utils import TestHelper as helper
    from utils import Tracer

    import azlmbr.legacy.general as general
    import azlmbr.asset as asset
    import azlmbr.math as math
    import azlmbr.bus as bus

    LEVEL_NAME = "tmp_level"
    WAIT_TIME = 3.0  # SECONDS
    EXPECTED_LINES = ["T92569006_ScriptEvent_Sent", "T92569006_ScriptEvent_Received"]
    SC_ASSET_PATH = os.path.join("ScriptCanvas", "T92569006_ScriptCanvas.scriptcanvas")

    def create_editor_entity(name, sc_asset):
        entity = Entity.create_editor_entity(name)
        sc_comp = entity.add_component("Script Canvas")
        asset_id = asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", sc_asset, math.Uuid(), False)
        sc_comp.set_component_property_value("Script Canvas Asset|Script Canvas Asset", asset_id)
        Report.critical_result(Tests.entity_created, entity.id.isValid())

    def locate_expected_lines(line_list: list):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]

        return all(line in found_lines for line in line_list)

    # 1) Create temp level
    general.idle_enable(True)
    result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Create test entity
    create_editor_entity("TestEntity", SC_ASSET_PATH)

    # 3) Start Tracer
    with Tracer() as section_tracer:

        # 4) Enter Game Mode
        helper.enter_game_mode(Tests.enter_game_mode)

        # 5) Read for line
        lines_located = helper.wait_for_condition(lambda: locate_expected_lines(EXPECTED_LINES), WAIT_TIME)
        Report.result(Tests.lines_found, lines_located)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptEvents_ReturnSetType_Successfully)
