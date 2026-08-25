"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    second_viewport_opened = (
        "A second viewport opened for the level",
        "A second viewport did not open for the level")
    two_worlds = (
        "The two viewports show two different worlds",
        "The two viewports ended up showing the same world")
    click_selects_viewport = (
        "Clicking a viewport made its world active",
        "Clicking a viewport did not make its world active")
    show_does_not_steal = (
        "Showing a background viewport left the active world alone",
        "Showing a background viewport stole the active world")
    window_activate_does_not_steal = (
        "Activating the window left the active world alone",
        "Activating the window let a background viewport steal the active world")


def MultiViewport_BackgroundViewport_DoesNotStealTheActiveWorld():
    """
    Summary:
    The active world follows the viewport the user is working in. A viewport that merely gets shown, or
    that receives a WindowActivate because the whole editor window was activated, must not take
    attribution away from the viewport that is actually selected - otherwise edits and undo are filed
    against the wrong world.

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
                    found.append((dock_widget, child, viewport_id))
                    break
        return found

    def viewport_ids():
        return set(viewport_id for _, _, viewport_id in viewport_panes())

    def pane_for(viewport_id):
        for dock_widget, widget, found_id in viewport_panes():
            if found_id == viewport_id:
                return dock_widget, widget
        return None, None

    def active_world():
        return editor.EditorEntityContextRequestBus(bus.Broadcast, "GetActiveWorldId")

    def world_of(viewport_id):
        return editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", viewport_id)

    def click_viewport(viewport_id):
        _, widget = pane_for(viewport_id)
        QtTest.QTest.mouseClick(widget, QtCore.Qt.LeftButton, QtCore.Qt.NoModifier, widget.rect().center())
        general.idle_wait(0.5)

    prefab_test_utils.open_base_tests_level()

    main_viewport_ids = viewport_ids()
    Report.info(f"Viewports before opening a level: {sorted(main_viewport_ids)}")

    # Open a second level in its own viewport, so there are two viewports on two different worlds.
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
        Tests.second_viewport_opened,
        helper.wait_for_condition(lambda: len(viewport_ids() - main_viewport_ids) == 1, 10.0))

    background_viewport_id = (viewport_ids() - main_viewport_ids).pop()
    selected_viewport_id = sorted(main_viewport_ids)[0]

    Report.result(Tests.two_worlds, world_of(selected_viewport_id) != world_of(background_viewport_id))

    # Work in the first viewport. From here on it is the one the user is using.
    click_viewport(selected_viewport_id)
    Report.result(Tests.click_selects_viewport, active_world() == world_of(selected_viewport_id))

    # Hiding and re-showing the other viewport must not re-point attribution at it.
    background_dock, background_widget = pane_for(background_viewport_id)
    background_dock.hide()
    general.idle_wait(0.5)
    background_dock.show()
    general.idle_wait(0.5)

    Report.result(Tests.show_does_not_steal, active_world() == world_of(selected_viewport_id))

    # Neither must activating the editor window, which Qt delivers to every viewport in it.
    QtCore.QCoreApplication.sendEvent(background_widget, QtCore.QEvent(QtCore.QEvent.WindowActivate))
    general.idle_wait(0.5)

    Report.result(Tests.window_activate_does_not_steal, active_world() == world_of(selected_viewport_id))


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_BackgroundViewport_DoesNotStealTheActiveWorld)
