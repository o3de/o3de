"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    level_created     = ("New level created",              "New level not created")
    game_mode_entered = ("Game Mode successfully entered", "Game mode failed to enter")
    game_mode_exited  = ("Game Mode successfully exited",  "Game mode failed to exited")
    found_lines       = ("Expected log lines were found",  "Expected log lines were not found")
# fmt: on


class LogLines:
    expected_lines = ["Greetings from the first script", "Greetings from the second script"]


def ScriptCanvas_TwoComponents_InteractSuccessfully():
    """
    Summary:
     A test entity contains two Script Canvas components with different unique script canvas files.
     Each of these files will have a print node set to activate on graph start.

    Expected Behavior:
     When game mode is entered, two unique strings should be printed out to the console

    Test Steps:
     1) Create level
     2) Create entity with SC components
     3) Start Tracer
     4) Enter game mode
     5) Wait for expected lines to be found
     6) Report if expected lines were found
     7) Exit game mode

    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
            Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    import os

    from utils import TestHelper as helper
    import hydra_editor_utils as hydra
    from utils import Report
    from utils import Tracer

    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.asset as asset
    import azlmbr.bus as bus

    LEVEL_NAME = "tmp_level"
    ASSET_1 = os.path.join("scriptcanvas", "ScriptCanvas_TwoComponents0.scriptcanvas")
    ASSET_2 = os.path.join("scriptcanvas", "ScriptCanvas_TwoComponents1.scriptcanvas")
    WAIT_TIME = 3.0  # SECONDS

    def get_asset(asset_path):
        return asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)

    def locate_expected_lines():
        found_lines = []
        for printInfo in section_tracer.prints:
            found_lines.append(printInfo.message.strip())

        return all(line in found_lines for line in LogLines.expected_lines)

    # 1) Create level
    general.idle_enable(True)
    result = general.create_level_no_prompt(LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Create entity with SC components
    position = math.Vector3(512.0, 512.0, 32.0)
    test_entity = hydra.Entity("test_entity")
    test_entity.create_entity(position, ["Script Canvas", "Script Canvas"])
    test_entity.get_set_test(0, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_1))
    test_entity.get_set_test(1, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_2))

    # 3) Start Tracer
    with Tracer() as section_tracer:

        # 4) Enter game mode
        helper.enter_game_mode(Tests.game_mode_entered)

        # 5) Wait for expected lines to be found
        helper.wait_for_condition(locate_expected_lines, WAIT_TIME)

        # 6) Report if expected lines were found
        Report.result(Tests.found_lines, locate_expected_lines())

    # 7) Exit game mode
    helper.exit_game_mode(Tests.game_mode_exited)


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(ScriptCanvas_TwoComponents_InteractSuccessfully)
