"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    level_created     = ("New level created",              "New level not created")
    entity_created    = ("Test Entity created",            "Test Entity not created")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    found_lines       = ("Expected log lines were found",  "Expected log lines were not found")
# fmt: on


def ScriptCanvas_ChangingAssets_ComponentStable():
    """
    Summary:
     Changing the assigned Script Canvas Asset on an entity properly updates level functionality

    Expected Behavior:
     When game mode is entered, respective strings of assigned assets should be printed

    Test Steps:
     1) Create temp level
     2) Create new entity
     3) Start Tracer
     4) Set first script and evaluate
     5) Set second script and evaluate


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
    ASSET_1 = os.path.join("scriptcanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
    ASSET_2 = os.path.join("scriptcanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")
    EXP_LINE_1 = "Greetings from the first script"
    EXP_LINE_2 = "Greetings from the second script"
    WAIT_TIME = 3.0  # SECONDS

    def get_asset(asset_path):
        return asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)

    def find_expected_line(expected_line):
        found_lines = [printInfo.message.strip() for printInfo in section_tracer.prints]
        return expected_line in found_lines

    def set_asset_evaluate(test_entity, ASSET_PATH, EXP_LINE):
        # Set Script Canvas entity
        test_entity.get_set_test(0, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_PATH))

        # Enter/exit game mode
        helper.enter_game_mode(Tests.game_mode_entered)
        helper.wait_for_condition(lambda: find_expected_line(EXP_LINE), WAIT_TIME)
        Report.result(Tests.found_lines, find_expected_line(EXP_LINE))
        helper.exit_game_mode(Tests.game_mode_exited)

    # 1) Create temp level
    general.idle_enable(True)
    result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Create new entity
    position = math.Vector3(512.0, 512.0, 32.0)
    test_entity = hydra.Entity("test_entity")
    test_entity.create_entity(position, ["Script Canvas"])

    # 3) Start Tracer
    with Tracer() as section_tracer:

        # 4) Set first script and evaluate
        set_asset_evaluate(test_entity, ASSET_1, EXP_LINE_1)

        # 5) Set second script and evaluate
        set_asset_evaluate(test_entity, ASSET_2, EXP_LINE_2)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvas_ChangingAssets_ComponentStable)
