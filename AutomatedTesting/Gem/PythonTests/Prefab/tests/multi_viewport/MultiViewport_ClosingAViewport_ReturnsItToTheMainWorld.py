"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    viewport_opened = (
        "A second Editor Viewport opened on its own world",
        "A second Editor Viewport did not open on its own world")
    pane_closed = (
        "The second Editor Viewport closed",
        "The second Editor Viewport did not close")
    world_released = (
        "The closed viewport no longer holds its world",
        "The closed viewport still holds its world")
    main_world_intact = (
        "The editor level is still the active world after the second viewport closed",
        "The editor level was disturbed by closing the second viewport")


def MultiViewport_ClosingAViewport_ReturnsItToTheMainWorld():
    """
    Summary:
    A world whose last viewport closes is torn down, and the viewport id falls back to the editor's own level.
    Closing a secondary viewport must not disturb the level in the main viewport.

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

    def viewport_panes():
        editor_window = pyside_utils.get_editor_main_window()
        main_window = editor_window.findChild(QtWidgets.QMainWindow)
        found = []
        for dock_widget in main_window.findChildren(QtWidgets.QDockWidget):
            if not dock_widget.windowTitle().startswith("Editor Viewport"):
                continue
            for child in dock_widget.findChildren(QtWidgets.QWidget):
                viewport_id = child.property("ViewportId")
                if viewport_id is not None:
                    found.append((dock_widget, viewport_id))
                    break
        return found

    def viewport_ids():
        return set(viewport_id for _, viewport_id in viewport_panes())

    prefab_test_utils.open_base_tests_level()

    main_world_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetEditorEntityContextId")
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

    helper.wait_for_condition(lambda: len(viewport_ids() - viewport_ids_before) == 1, 10.0)
    new_viewport_id = (viewport_ids() - viewport_ids_before).pop()

    new_world_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", new_viewport_id)
    Report.result(Tests.viewport_opened, new_world_id != main_world_id)

    for dock_widget, viewport_id in viewport_panes():
        if viewport_id == new_viewport_id:
            dock_widget.close()

    Report.result(
        Tests.pane_closed,
        helper.wait_for_condition(lambda: viewport_ids() == viewport_ids_before, 10.0))

    world_after_close = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", new_viewport_id)
    Report.result(Tests.world_released, world_after_close == main_world_id)

    active_world_id = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetActiveWorldId")
    Report.result(Tests.main_world_intact, active_world_id == main_world_id)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_ClosingAViewport_ReturnsItToTheMainWorld)
