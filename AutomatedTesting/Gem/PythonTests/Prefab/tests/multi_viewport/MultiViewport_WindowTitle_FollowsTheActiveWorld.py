"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    second_viewport_opened = (
        "A second viewport opened on its own world",
        "A second viewport did not open on its own world")
    two_worlds = (
        "The two viewports show two different worlds",
        "The two viewports ended up showing the same world")
    working_in_second_viewport = (
        "The second viewport's world became active",
        "The second viewport's world did not become active")
    title_names_second_world = (
        "The window title names the level of the viewport being worked in",
        "The window title does not name the level of the viewport being worked in")
    title_follows_back = (
        "The window title followed back to the first viewport's level",
        "The window title did not follow back to the first viewport's level")


def MultiViewport_WindowTitle_FollowsTheActiveWorld():
    """
    Summary:
    The editor window title names the level being edited. With several levels open it must name the
    level of the world the user is working in, not the one the editor happened to start with.

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

    held_wrappers = []

    def editor_window():
        window = pyside_utils.get_editor_main_window()
        held_wrappers.append(window)
        return window

    def main_window():
        return editor_window().findChild(QtWidgets.QMainWindow)

    def window_title():
        return editor_window().windowTitle()

    def viewport_panes():
        found = []
        for dock_widget in main_window().findChildren(QtWidgets.QDockWidget):
            if not dock_widget.windowTitle().startswith("Editor Viewport") or not dock_widget.isVisible():
                continue
            for child in dock_widget.findChildren(QtWidgets.QWidget):
                viewport_id = child.property("ViewportId")
                if viewport_id is not None:
                    found.append((dock_widget, child, viewport_id))
                    break
        return found

    def viewport_ids():
        return set(viewport_id for _, _, viewport_id in viewport_panes())

    def active_world():
        return editor.EditorEntityContextRequestBus(bus.Broadcast, "GetActiveWorldId")

    def world_of(viewport_id):
        return editor.EditorEntityContextRequestBus(bus.Broadcast, "GetViewportWorld", viewport_id)

    def level_name_of(viewport_id):
        # The title names the level, not its whole path.
        level_path = editor.EditorEntityContextRequestBus(bus.Broadcast, "GetWorldLevelPath", world_of(viewport_id))
        return level_path.rsplit("/", 1)[-1].rsplit(".", 1)[0]

    def work_in_viewport(viewport_id):
        # Clicking a viewport to select it is covered by
        # MultiViewport_BackgroundViewport_DoesNotStealTheActiveWorld. Here the subject is the title,
        # so drive the active world through the same request the viewport selection ends up making.
        editor.EditorEntityContextRequestBus(bus.Broadcast, "SetFocusedViewport", viewport_id)
        general.idle_wait(1.0)

    prefab_test_utils.open_base_tests_level()
    general.idle_wait(1.0)

    while len(viewport_ids()) > 1:
        viewport_panes()[len(viewport_ids()) - 1][0].close()
        general.idle_wait(1.0)

    first_viewport_id = sorted(viewport_ids())[0]

    if not general.is_pane_visible("Asset Browser"):
        pyside_utils.get_action_for_menu_path(main_window(), "Tools", "Asset Browser").trigger()
        general.idle_wait(1.0)

    asset_browser = pyside_utils.find_child_by_pattern(
        main_window(), text="Asset Browser", type=QtWidgets.QDockWidget)
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
        helper.wait_for_condition(lambda: len(viewport_ids() - {first_viewport_id}) >= 1, 10.0))

    second_viewport_id = sorted(viewport_ids() - {first_viewport_id})[0]
    Report.result(Tests.two_worlds, world_of(first_viewport_id) != world_of(second_viewport_id))

    first_level = level_name_of(first_viewport_id)
    second_level = level_name_of(second_viewport_id)
    Report.info(f"First viewport level '{first_level}', second viewport level '{second_level}'")

    work_in_viewport(second_viewport_id)
    Report.result(Tests.working_in_second_viewport, active_world() == world_of(second_viewport_id))
    Report.info(f"Title while working in the second viewport: {window_title()}")
    Report.result(Tests.title_names_second_world, second_level in window_title())

    work_in_viewport(first_viewport_id)
    Report.info(f"Title while working in the first viewport: {window_title()}")
    Report.result(Tests.title_follows_back, first_level in window_title())


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_WindowTitle_FollowsTheActiveWorld)
