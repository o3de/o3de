"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    main_world_active = (
        "The main viewport shows the editor level before the test starts",
        "The editor was not showing its own level before the test started")
    new_viewport_opened = (
        "Double-clicking a level prefab opened an additional Editor Viewport",
        "Double-clicking a level prefab did not open an additional Editor Viewport")
    world_is_distinct = (
        "The new viewport shows a world of its own",
        "The new viewport is still showing the editor level")
    level_path_matches = (
        "The new world holds the level that was double-clicked",
        "The new world does not hold the level that was double-clicked")


def MultiViewport_LevelFromAssetBrowser_OpensInItsOwnViewport():
    """
    Summary:
    Double-clicking a level prefab in the Asset Browser opens it in an additional Editor Viewport bound to a world
    of its own, leaving the editor's own level in the main viewport untouched.

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
    level_relative_path = "Levels/Prefab/PrefabLevel_OpensLevelWithEntities/PrefabLevel_OpensLevelWithEntities.prefab"

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

    prefab_test_utils.open_base_tests_level()

    main_world_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetEditorEntityContextId")
    active_world_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetActiveWorldId")
    Report.result(Tests.main_world_active, active_world_id == main_world_id)

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

    model_index = pyside_utils.find_child_by_hierarchy(tree, level_asset_path[0])
    for path_element in level_asset_path[1:]:
        model_index = pyside_utils.find_child_by_hierarchy(model_index, path_element)
    assert model_index is not None, f"Asset {'/'.join(level_asset_path)} was not found in the Asset Browser"

    tree.scrollTo(model_index)
    pyside_utils.item_view_index_mouse_click(tree, model_index)
    QtTest.QTest.mouseDClick(
        tree.viewport(), QtCore.Qt.LeftButton, QtCore.Qt.NoModifier, tree.visualRect(model_index).center())

    Report.result(
        Tests.new_viewport_opened,
        helper.wait_for_condition(lambda: len(viewport_ids() - viewport_ids_before) == 1, 10.0))

    new_viewport_id = (viewport_ids() - viewport_ids_before).pop()
    new_world_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", new_viewport_id)
    Report.result(Tests.world_is_distinct, new_world_id != main_world_id)

    level_path = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetWorldLevelPath", new_world_id)
    Report.info(f"New world holds level '{level_path}'")
    Report.result(Tests.level_path_matches, level_path.replace("\\", "/") == level_relative_path)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_LevelFromAssetBrowser_OpensInItsOwnViewport)
