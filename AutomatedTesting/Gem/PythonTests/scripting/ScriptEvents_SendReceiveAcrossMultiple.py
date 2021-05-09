"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID: T92567321
Test Case Title: Script Events: Can send and receive a script event across multiple entities successfully
URL of the test case: https://testrail.agscollab.com/index.php?/tests/view/92567321
"""


# fmt: off
class Tests():
    level_created   = ("Successfully created temporary level", "Failed to create temporary level")
    entitya_created = ("Successfully created EntityA",         "Failed to create EntityA")
    entityb_created = ("Successfully created EntityB",         "Failed to create EntityB")
    enter_game_mode = ("Successfully entered game mode",       "Failed to enter game mode")
    lines_found     = ("Successfully found expected message",  "Failed to find expected message")
    exit_game_mode  = ("Successfully exited game mode",        "Failed to exit game mode")
# fmt: on


def ScriptEvents_SendReceiveAcrossMultiple():
    """
    Summary:
     EntityA and EntityB will be created in a level. Attached to both will be a Script Canvas component. The Script Event created for the test will be sent from EntityA to EntityB.

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
    import os
    
    from editor_entity_utils import EditorEntity as Entity
    from utils import Report
    from utils import TestHelper as helper
    from utils import Tracer

    import azlmbr.legacy.general as general

    LEVEL_NAME = "tmp_level"
    WAIT_TIME = 3.0
    ASSET_PREFIX = "T92567321"
    asset_paths = {
        "event": os.path.join("TestAssets", f"{ASSET_PREFIX}.scriptevents"),
        "assetA": os.path.join("ScriptCanvas", f"{ASSET_PREFIX}A.scriptcanvas"),
        "assetB": os.path.join("ScriptCanvas", f"{ASSET_PREFIX}B.scriptcanvas"),
    }
    sc_for_entities = {
        "EntityA": asset_paths["assetA"],
        "EntityB": asset_paths["assetB"]
    }
    EXPECTED_LINES = ["Incoming Message Received"]

    def get_asset(asset_path):
        return azlmbr.asset.AssetCatalogRequestBus(azlmbr.bus.Broadcast, "GetAssetIdByPath", asset_path, azlmbr.math.Uuid(), False)

    def create_editor_entity(name, sc_asset):
        entity = Entity.create_editor_entity(name)
        sc_comp = entity.add_component("Script Canvas")
        sc_comp.set_component_property_value("Script Canvas Asset|Script Canvas Asset", get_asset(sc_asset))
        Report.critical_result(Tests.__dict__[name.lower()+"_created"], entity.id.isValid())

    def locate_expected_lines(line_list: list):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]

        return all(line in found_lines for line in line_list)

    # 1) Create temp level
    general.idle_enable(True)
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
        lines_located = helper.wait_for_condition(lambda: locate_expected_lines(EXPECTED_LINES), WAIT_TIME)
        Report.result(Tests.lines_found, lines_located)

    # 6) Exit Game Mode
    helper.exit_game_mode(Tests.exit_game_mode)


if __name__ == "__main__":
    import ImportPathHelper as imports
    imports.init()

    from utils import Report

    Report.start_test(ScriptEvents_SendReceiveAcrossMultiple)
