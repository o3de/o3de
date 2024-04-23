"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    level_created           = ("New level created",                    "New level not created")
    entities_found          = ("Entities are found in Logging window", "Entities are not found in Logging window")
    select_multiple_targets = ("Multiple targets are selected",        "Multiple targets are not selected")
# fmt: on


GENERAL_WAIT = 0.5  # seconds


def Debugger_HappyPath_TargetMultipleEntities():
    """
    Summary:
     Multiple Entities can be targeted in the Debugger tool

    Expected Behavior:
     Multiple selected files can be checked for logging.
     Upon checking, checkboxes of the parent folders change to either full or partial check.

    Test Steps:
     1) Create temp level
     2) Create two entities with scriptcanvas components
     3) Set values for scriptcanvas
     4) Open Script Canvas window and get sc object
     5) Open Debugging(Logging) window
     6) Click on Entities tab in logging window
     7) Verify if the scriptcanvas exist under entities
     8) Verify if the entities can be selected
     9) Close Debugging window and Script Canvas window


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """
    from PySide2 import QtWidgets
    from PySide2.QtCore import Qt
    import azlmbr.legacy.general as general
    import azlmbr.math as math
    import azlmbr.asset as asset
    import azlmbr.bus as bus

    import os
    import pyside_utils
    import hydra_editor_utils as hydra
    from utils import TestHelper as helper
    from utils import Report

    TEMPLATE_NAME ="lvl_template_name"
    LEVEL_NAME = "tmp_level"
    ASSET_NAME_1 = "ScriptCanvas_TwoComponents0.scriptcanvas"
    ASSET_NAME_2 = "ScriptCanvas_TwoComponents1.scriptcanvas"
    ASSET_1 = os.path.join("scriptcanvas", ASSET_NAME_1)
    ASSET_2 = os.path.join("scriptcanvas", ASSET_NAME_2)
    WAIT_TIME = 3.0

    def get_asset(asset_path):
        return asset.AssetCatalogRequestBus(bus.Broadcast, "GetAssetIdByPath", asset_path, math.Uuid(), False)

    # 1) Create temp level
    general.idle_enable(True)
    result = general.create_level_no_prompt(TEMPLATE_NAME, LEVEL_NAME, 128, 1, 512, True)
    Report.critical_result(Tests.level_created, result == 0)
    helper.wait_for_condition(lambda: general.get_current_level_name() == LEVEL_NAME, WAIT_TIME)
    general.close_pane("Error Report")

    # 2) Create two entities with scriptcanvas components
    position = math.Vector3(512.0, 512.0, 32.0)
    test_entity_1 = hydra.Entity("test_entity_1")
    test_entity_1.create_entity(position, ["Script Canvas"])
    test_entity_2 = hydra.Entity("test_entity_2")
    test_entity_2.create_entity(position, ["Script Canvas"])

    # 3) Set values for scriptcanvas
    test_entity_1.get_set_test(0, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_1))
    test_entity_2.get_set_test(0, "Script Canvas Asset|Script Canvas Asset", get_asset(ASSET_2))

    # 4) Open Script Canvas window and get sc opbject
    general.open_pane("Script Canvas")
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")

    # 5) Open Debugging(Logging) window
    if (
        sc.findChild(QtWidgets.QDockWidget, "LoggingWindow") is None
        or not sc.findChild(QtWidgets.QDockWidget, "LoggingWindow").isVisible()
    ):
        action = pyside_utils.find_child_by_pattern(sc, {"text": "Debugging", "type": QtWidgets.QAction})
        action.trigger()
    logging_window = sc.findChild(QtWidgets.QDockWidget, "LoggingWindow")

    # 6) Click on Entities tab in logging window
    button = pyside_utils.find_child_by_pattern(logging_window, {"type": QtWidgets.QPushButton, "text": "Entities"})
    button.click()

    # 7) Verify if the scriptcanvas exist under entities
    entities = logging_window.findChild(QtWidgets.QWidget, "entitiesPage")
    tree = entities.findChild(QtWidgets.QTreeView, "pivotTreeView")
    asset_1_mi = pyside_utils.find_child_by_pattern(tree, ASSET_NAME_1.lower())
    asset_2_mi = pyside_utils.find_child_by_pattern(tree, ASSET_NAME_2.lower())
    result = asset_1_mi is not None and asset_2_mi is not None
    Report.critical_result(Tests.entities_found, result)

    # 8) Verify if the entities can be selected
    tree.expandAll()
    tree.model().setData(asset_1_mi, 2, Qt.CheckStateRole)
    tree.model().setData(asset_2_mi, 2, Qt.CheckStateRole)
    checklist = [asset_1_mi, asset_1_mi.parent(), asset_2_mi, asset_2_mi.parent()]
    result = all([index.data(Qt.CheckStateRole) == 2 for index in checklist])
    Report.critical_result(Tests.select_multiple_targets, result)

    # 9) Close Debugging window and Script Canvas window
    logging_window.close()
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Debugger_HappyPath_TargetMultipleEntities)
