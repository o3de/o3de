"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Test case ID: T92563191
Test Case Title: Two Entities can use the same Graph asset successfully at RunTime
URL of the test case: https://testrail.agscollab.com/index.php?/tests/view/92563191
"""


# fmt: off
class Tests():
    level_created     = ("New level created",              "New level not created")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    found_lines       = ("Expected log lines were found",  "Expected log lines were not found")
# fmt: on


def ScriptCanvas_TwoEntities():
    """
    Summary:
     Two Entities can use the same Graph asset successfully at RunTime. The script canvas asset
     attached to the enties will print the respective entity names.

    Expected Behavior:
     When game mode is entered, respective strings of different entities should be printed.

    Test Steps:
     1) Create temp level
     2) Create two new entities with different names
     3) Set ScriptCanvas asset to both the entities
     4) Enter/Exit game mode and verify log lines


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    import os

    from utils import TestHelper as helper
    from utils import Tracer
    import hydra_editor_utils as hydra
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.asset as asset
    import azlmbr.bus as bus

    LEVEL_NAME = "tmp_level"
    ASSET_PATH = os.path.join("scriptcanvas", "T92563191_test.scriptcanvas")
    EXPECTED_LINES = ["Entity Name: test_entity_1", "Entity Name: test_entity_2"]
    WAIT_TIME = 0.5  # SECONDS

    def get_asset(asset_path):
        return asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)

    # 1) Create temp level
    general.idle_enable(True)
    result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Create two new entities with different names
    position = math.Vector3(512.0, 512.0, 32.0)
    test_entity_1 = hydra.Entity("test_entity_1")
    test_entity_1.create_entity(position, ["Script Canvas"])

    test_entity_2 = hydra.Entity("test_entity_2")
    test_entity_2.create_entity(position, ["Script Canvas"])

    # 3) Set ScriptCanvas asset to both the entities
    test_entity_1.get_set_test(0, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_PATH))
    test_entity_2.get_set_test(0, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_PATH))

    # 4) Enter/Exit game mode and verify log lines
    with Tracer() as section_tracer:

        helper.enter_game_mode(Tests.game_mode_entered)
        # wait for WAIT_TIME to let the script print strings
        general.idle_wait(WAIT_TIME)
        helper.exit_game_mode(Tests.game_mode_exited)

    found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]
    result = all(line in found_lines for line in EXPECTED_LINES)

    Report.result(Tests.found_lines, result)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvas_TwoEntities)
