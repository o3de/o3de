"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


class Tests:
    reduced_to_one_viewport = (
        "Viewports closed normally while others remained, down to the last one",
        "A viewport refused to close even though another one remained")
    last_viewport_refused_to_close = (
        "The only viewport refused to close",
        "The only viewport closed, leaving the editor with none")
    second_viewport_opened = (
        "A second viewport opened",
        "A second viewport did not open")
    reduced_to_one_again = (
        "The extra viewports closed again, down to the last one",
        "An extra viewport refused to close even though another one remained")
    last_viewport_refused_again = (
        "The remaining viewport refused to close",
        "The remaining viewport closed, leaving the editor with none")


def MultiViewport_LastViewport_CannotBeClosed():
    """
    Summary:
    The editor must always keep at least one viewport. Closing a viewport is allowed while another one
    remains, but the last one refuses the close so the editor is never left with no viewport and no
    active world.

    Editor shutdown must still work, so this test running to completion also exercises the shutdown
    guard: a veto that did not exempt shutdown would hang the editor on teardown.

    :return: None
    """

    from PySide6 import QtCore, QtTest, QtWidgets

    import azlmbr.legacy.general as general

    import pyside_utils
    import Prefab.tests.PrefabTestUtils as prefab_test_utils
    from editor_python_test_tools.utils import Report
    from editor_python_test_tools.utils import TestHelper as helper

    # get_editor_main_window() returns a wrapInstance wrapper; if it is collected while anything found
    # underneath it is still in use, the whole C++ object tree is invalidated. Hold every wrapper.
    held_wrappers = []

    def main_window():
        editor_window = pyside_utils.get_editor_main_window()
        held_wrappers.append(editor_window)
        return editor_window.findChild(QtWidgets.QMainWindow)

    def viewport_docks():
        found = []
        for dock_widget in main_window().findChildren(QtWidgets.QDockWidget):
            if not dock_widget.windowTitle().startswith("Editor Viewport") or not dock_widget.isVisible():
                continue
            for child in dock_widget.findChildren(QtWidgets.QWidget):
                if child.property("ViewportId") is not None:
                    found.append(dock_widget)
                    break
        return found

    def viewport_count():
        return len(set(viewport_ids()))

    def viewport_ids():
        ids = []
        for dock_widget in main_window().findChildren(QtWidgets.QDockWidget):
            if not dock_widget.windowTitle().startswith("Editor Viewport") or not dock_widget.isVisible():
                continue
            for child in dock_widget.findChildren(QtWidgets.QWidget):
                viewport_id = child.property("ViewportId")
                if viewport_id is not None:
                    ids.append(viewport_id)
                    break
        return ids

    def close_one_viewport():
        # Close the dock holding the highest viewport id, so the lowest survives.
        docks_by_id = {}
        for dock_widget in main_window().findChildren(QtWidgets.QDockWidget):
            if not dock_widget.windowTitle().startswith("Editor Viewport") or not dock_widget.isVisible():
                continue
            for child in dock_widget.findChildren(QtWidgets.QWidget):
                viewport_id = child.property("ViewportId")
                if viewport_id is not None:
                    docks_by_id.setdefault(viewport_id, dock_widget)
                    break
        if not docks_by_id:
            return
        target = max(docks_by_id)
        Report.info(f"Closing viewport {target}; ids present: {sorted(docks_by_id)}")
        docks_by_id[target].close()
        general.idle_wait(1.0)
        Report.info(f"After close, ids present: {sorted(set(viewport_ids()))}")

    level_asset_path = (
        "AutomatedTesting", "Levels", "Prefab",
        "PrefabLevel_OpensLevelWithEntities", "PrefabLevel_OpensLevelWithEntities.prefab")

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

    # The editor restores its previous layout, so the starting viewport count is not fixed. Drive it
    # down to one instead of assuming it.
    def close_down_to_one():
        while viewport_count() > 1:
            before = viewport_count()
            close_one_viewport()
            after = viewport_count()
            if after >= before or after < 1:
                return False
        return viewport_count() == 1

    Report.info(f"Viewport docks at start: {len(viewport_docks())}")

    Report.result(Tests.reduced_to_one_viewport, close_down_to_one())

    close_one_viewport()
    Report.result(Tests.last_viewport_refused_to_close, viewport_count() == 1)

    open_level_in_new_viewport()
    Report.result(Tests.second_viewport_opened, helper.wait_for_condition(lambda: viewport_count() >= 2, 10.0))

    Report.result(Tests.reduced_to_one_again, close_down_to_one())

    close_one_viewport()
    Report.result(Tests.last_viewport_refused_again, viewport_count() == 1)


if __name__ == "__main__":
    from editor_python_test_tools.utils import Report
    Report.start_test(MultiViewport_LastViewport_CannotBeClosed)
