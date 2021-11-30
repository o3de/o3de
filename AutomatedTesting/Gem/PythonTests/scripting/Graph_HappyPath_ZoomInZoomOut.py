"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""


# fmt: off
class Tests():
    zoom_in  = ("Zoom In action working as expected",  "Zoom In action not working as expected")
    zoom_out = ("Zoom Out action working as expected", "Zoom Out action not working as expected")
# fmt: on


GENERAL_WAIT = 0.5  # seconds


def Graph_HappyPath_ZoomInZoomOut():
    """
    Summary:
     The graph can be zoomed in and zoomed out.

    Expected Behavior:
     The graph is zoomed in when when we click View->ZoomIn
     The graph is zoomed out when when we click View->ZoomOut

    Test Steps:
     1) Open Script Canvas window (Tools > Script Canvas)
     2) Get the SC window object
     3) Create new graph
     4) Get initial graph transform values
     5) Trigger Zoom In and verify if the graph transform scale is increased
     6) Trigger Zoom Out and verify if the graph transform scale is decreased
     7) Close Script Canvas window


    Note:
     - This test file must be called from the Open 3D Engine Editor command terminal
     - Any passed and failed tests are written to the Editor.log file.
        Parsing the file or running a log_monitor are required to observe the test results.

    :return: None
    """

    # Helper imports
    import ImportPathHelper as imports

    imports.init()

    from PySide2 import QtWidgets
    import azlmbr.legacy.general as general

    import pyside_utils
    from utils import TestHelper as helper
    from utils import Report

    # 1) Open Script Canvas window (Tools > Script Canvas)
    general.idle_enable(True)
    general.open_pane("Script Canvas")
    helper.wait_for_condition(lambda: general.is_pane_visible("Script Canvas"), 5.0)

    # 2) Get the SC window object
    editor_window = pyside_utils.get_editor_main_window()
    sc = editor_window.findChild(QtWidgets.QDockWidget, "Script Canvas")
    sc_main = sc.findChild(QtWidgets.QMainWindow)

    # 3) Create new graph
    create_new_graph = pyside_utils.find_child_by_pattern(
        sc_main, {"objectName": "action_New_Script", "type": QtWidgets.QAction}
    )
    create_new_graph.trigger()

    # 4) Get initial graph transform values
    graphics_view = sc_main.findChild(QtWidgets.QGraphicsView)
    # NOTE: transform m11 and m22 are horizontal and vertical scales of graph
    # they increase when zoomed in and decreased when zoomed out
    curr_m11, curr_m22 = graphics_view.transform().m11(), graphics_view.transform().m22()

    # 5) Trigger Zoom In and verify if the graph transform scale is increased
    zin = pyside_utils.find_child_by_pattern(sc_main, {"objectName": "action_ZoomIn", "type": QtWidgets.QAction})
    zin.trigger()
    result = helper.wait_for_condition(
        lambda: curr_m11 < graphics_view.transform().m11() and curr_m22 < graphics_view.transform().m22(), GENERAL_WAIT
    )
    Report.result(Tests.zoom_in, result)

    # 6) Trigger Zoom Out and verify if the graph transform scale is decreased
    curr_m11, curr_m22 = graphics_view.transform().m11(), graphics_view.transform().m22()
    zout = pyside_utils.find_child_by_pattern(sc_main, {"objectName": "action_ZoomOut", "type": QtWidgets.QAction})
    zout.trigger()
    result = helper.wait_for_condition(
        lambda: curr_m11 > graphics_view.transform().m11() and curr_m22 > graphics_view.transform().m22(), GENERAL_WAIT
    )
    Report.result(Tests.zoom_out, result)

    # 7) Close Script Canvas window
    general.close_pane("Script Canvas")


if __name__ == "__main__":
    import ImportPathHelper as imports

    imports.init()
    from utils import Report

    Report.start_test(Graph_HappyPath_ZoomInZoomOut)
