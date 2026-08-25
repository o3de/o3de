"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    second_viewport_opened = (
        "A second viewport opened",
        "A second viewport did not open")
    one_viewport_ui_for_the_editor = (
        "Exactly one viewport UI exists no matter how many viewports are open",
        "The viewport UI is per viewport rather than editor wide")
    viewport_ui_survives = (
        "The viewport UI outlived the viewport holding the lowest id",
        "The viewport UI was destroyed along with the viewport holding the lowest id")
    viewport_ui_follows_survivor = (
        "The viewport UI is anchored over a surviving viewport",
        "The viewport UI is not anchored over any surviving viewport")


def MultiViewport_ClosingTheFirstViewport_KeepsTheViewportUi():
    """
    Summary:
    The viewport UI (transform mode cluster, snapping, component mode switcher) is editor wide chrome
    that renders over whichever viewport is selected, so exactly one of it must exist however many
    viewports are open. When it belonged to a viewport instead, closing that viewport destroyed it and
    every viewport UI request addressed to DefaultViewportId silently became a no-op.

    :return: None
    """

    from PySide6 import QtCore, QtTest, QtWidgets

    import azlmbr.legacy.general as general

    import pyside_utils
    import Prefab.tests.PrefabTestUtils as prefab_test_utils
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    level_asset_path = (
        "AutomatedTesting", "Levels", "Prefab",
        "PrefabLevel_OpensLevelWithEntities", "PrefabLevel_OpensLevelWithEntities.prefab")

    # get_editor_main_window() returns a wrapInstance wrapper; if it is collected while anything found
    # underneath it is still in use, the whole C++ object tree is invalidated. Hold every wrapper.
    held_wrappers = []

    def main_window():
        editor_window = pyside_utils.get_editor_main_window()
        held_wrappers.append(editor_window)
        return editor_window.findChild(QtWidgets.QMainWindow)

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

    def viewport_count():
        return len(viewport_panes())

    def viewport_ui_windows():
        # The viewport UI overlay carries Qt::Tool, so it is always a top level window. Scanning only
        # top level widgets avoids counting the same window twice through its parent.
        found = []
        for widget in QtWidgets.QApplication.topLevelWidgets():
            held_wrappers.append(widget)
            if widget.objectName() == "ViewportUiWindow":
                found.append(widget)
        return found

    def viewport_ui_origins():
        return [window.geometry().topLeft() for window in viewport_ui_windows()]

    def render_overlay_origins():
        origins = []
        for dock_widget, _, _ in viewport_panes():
            overlay = dock_widget.findChild(QtWidgets.QWidget, "renderOverlay")
            if overlay is not None:
                origins.append(overlay.mapToGlobal(QtCore.QPoint(0, 0)))
        return origins

    def open_level_in_new_viewport():
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

    prefab_test_utils.open_base_tests_level()
    general.idle_wait(1.0)

    # The editor restores its previous layout, so drive the viewport count rather than assuming it.
    while viewport_count() > 1:
        viewport_panes()[viewport_count() - 1][0].close()
        general.idle_wait(1.0)

    open_level_in_new_viewport()
    Report.result(Tests.second_viewport_opened, helper.wait_for_condition(lambda: viewport_count() >= 2, 10.0))

    Report.info(f"{viewport_count()} viewports, {len(viewport_ui_windows())} viewport UI windows")
    Report.result(Tests.one_viewport_ui_for_the_editor, len(viewport_ui_windows()) == 1)

    # Close whichever viewport owns the lowest id - that is the one the viewport UI used to belong to.
    panes = sorted(viewport_panes(), key=lambda pane: pane[2])
    lowest_id = panes[0][2]
    Report.info(f"Closing the viewport holding id {lowest_id}")
    panes[0][0].close()
    helper.wait_for_condition(lambda: lowest_id not in [pane[2] for pane in viewport_panes()], 10.0)
    general.idle_wait(1.0)

    Report.info(f"{viewport_count()} viewports left, {len(viewport_ui_windows())} viewport UI windows")
    Report.result(Tests.viewport_ui_survives, len(viewport_ui_windows()) == 1)

    overlay_origins = render_overlay_origins()
    ui_origins = viewport_ui_origins()
    Report.info(f"Viewport UI at {ui_origins}, surviving overlays at {overlay_origins}")
    Report.result(
        Tests.viewport_ui_follows_survivor,
        len(ui_origins) > 0 and any(origin in overlay_origins for origin in ui_origins))


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_ClosingTheFirstViewport_KeepsTheViewportUi)
