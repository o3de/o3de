"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    first_open_added_viewport = (
        "The first double-click opened an additional Editor Viewport",
        "The first double-click did not open an additional Editor Viewport")
    second_open_added_viewport = (
        "Double-clicking the same level again opened another viewport",
        "Double-clicking the same level again did not open another viewport")
    world_is_reused = (
        "Both viewports are held by the same world",
        "Reopening the level produced a second world for the same level")


def MultiViewport_ReopeningALevel_OpensASecondViewportOnTheSameWorld():
    """
    Summary:
    A level is never loaded into two worlds. Double-clicking a level prefab that a viewport is already showing
    opens a second viewport bound to that same world, so one world holds two viewport ids.

    :return: None
    """

    from PySide6 import QtCore, QtTest, QtWidgets

    import azlmbr.bus as bus
    import azlmbr.editor as editor
    import azlmbr.legacy.general as general

    import pyside_utils
    import Prefab.tests.PrefabTestUtils as prefab_test_utils
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    level_asset_path = (
        "AutomatedTesting", "Levels", "Prefab",
        "PrefabLevel_OpensLevelWithEntities", "PrefabLevel_OpensLevelWithEntities.prefab")

    def viewport_ids():
        # Hold the outer window alive: get_editor_main_window() returns a wrapInstance wrapper, and
        # letting it go out of scope while still walking its children invalidates the whole tree.
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        found = []
        for dock_widget in main_window.findChildren(QtWidgets.QDockWidget):
            if not dock_widget.windowTitle().startswith("Editor Viewport"):
                continue
            for child in dock_widget.findChildren(QtWidgets.QWidget):
                viewport_id = child.property("ViewportId")
                if viewport_id is not None:
                    found.append(viewport_id)
                    break
        return set(found)

    def double_click_level(tree):
        model_index = pyside_utils.find_child_by_hierarchy(tree, level_asset_path[0])
        for path_element in level_asset_path[1:]:
            model_index = pyside_utils.find_child_by_hierarchy(model_index, path_element)
        assert model_index is not None, f"Asset {'/'.join(level_asset_path)} was not found in the Asset Browser"

        tree.scrollTo(model_index)
        pyside_utils.item_view_index_mouse_click(tree, model_index)
        QtTest.QTest.mouseDClick(
            tree.viewport(), QtCore.Qt.LeftButton, QtCore.Qt.NoModifier, tree.visualRect(model_index).center())

    prefab_test_utils.open_base_tests_level()

    viewport_ids_before = viewport_ids()

    if not general.is_pane_visible("Asset Browser"):
        action = pyside_utils.get_action_for_menu_path(pyside_utils.get_editor_main_window(), "Tools", "Asset Browser")
        action.trigger()
        general.idle_wait(1.0)

    editor_window = pyside_utils.get_editor_main_window()

    main_window = editor_window.findChild(QtWidgets.QMainWindow)
    asset_browser = pyside_utils.find_child_by_pattern(main_window, text="Asset Browser", type=QtWidgets.QDockWidget)
    asset_browser.findChild(QtWidgets.QToolButton, "m_treeViewButton").click()
    general.idle_wait(1.0)

    tree = pyside_utils.find_child_by_pattern(asset_browser, "m_assetBrowserTreeViewWidget")
    tree.collapseAll()

    double_click_level(tree)

    Report.result(
        Tests.first_open_added_viewport,
        helper.wait_for_condition(lambda: len(viewport_ids() - viewport_ids_before) == 1, 10.0))

    first_viewport_id = (viewport_ids() - viewport_ids_before).pop()
    world_after_first_open = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", first_viewport_id)

    double_click_level(tree)

    Report.result(
        Tests.second_open_added_viewport,
        helper.wait_for_condition(lambda: len(viewport_ids() - viewport_ids_before) == 2, 10.0))

    second_viewport_id = (viewport_ids() - viewport_ids_before - {first_viewport_id}).pop()
    world_after_second_open = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", second_viewport_id)
    Report.result(Tests.world_is_reused, world_after_second_open == world_after_first_open)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_ReopeningALevel_OpensASecondViewportOnTheSameWorld)
